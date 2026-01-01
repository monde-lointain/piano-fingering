# Module Specification: MusicXML

## Responsibilities

- **Parser**: Parse MusicXML files → `Piece` data structure
- **Writer**: Write fingered `Piece` → MusicXML file
- Extract notes, chords, rests, measure boundaries, staff assignments
- Preserve all non-fingering musical information (dynamics, lyrics, metadata)
- Validate XML structure and report errors with line numbers
- Support MusicXML 3.0+ (FR-1.2)

---

## Data Ownership

### Primary Data Structures

```cpp
// Modified keyboard distance (0-14 per octave, including imaginary black keys)
struct Pitch {
    int value;  // 0-14 (C=0, C#=1, D=2, ..., B=13, imaginary=6,14)

    bool IsBlackKey() const { return value % 2 == 1; }
    bool IsWhiteKey() const { return value % 2 == 0; }
};

// Single note in a musical piece
struct Note {
    Pitch pitch;
    int octave;              // 0-10 (MIDI octave numbering)
    int duration;            // In ticks (resolution-dependent)
    bool is_rest;            // True if rest, false if sounding note
    Hand hand;               // LEFT or RIGHT (derived from staff assignment)
    int voice;               // Voice number (1-4) for polyphonic passages

    // Absolute keyboard position (for distance calculations)
    int AbsolutePosition() const { return octave * 15 + pitch.value; }
};

// Vertical time segment containing simultaneously played notes
class Slice {
public:
    void AddNote(const Note& note) { notes_.push_back(note); }
    const std::vector<Note>& GetNotes() const { return notes_; }
    size_t Size() const { return notes_.size(); }

    // Hard constraint: max 5 notes per hand per slice
    static constexpr size_t kMaxNotesPerHand = 5;

private:
    std::vector<Note> notes_;
};

// Musical measure (bar)
struct Measure {
    int number;
    int time_signature_numerator;    // e.g., 4 in 4/4
    int time_signature_denominator;  // e.g., 4 in 4/4
    std::vector<Slice> slices;
};

// Complete musical piece
class Piece {
public:
    void AddMeasure(const Measure& measure) { measures_.push_back(measure); }

    const std::vector<Measure>& GetMeasures() const { return measures_; }
    size_t GetMeasureCount() const { return measures_.size(); }

    // Count total slices across all measures
    size_t GetSliceCount() const;

    // Count total notes (excluding rests)
    size_t GetNoteCount() const;

    // Metadata
    std::string title;
    std::string composer;

private:
    std::vector<Measure> measures_;
};
```

---

## Communication

### Incoming Dependencies (Afferent)

| Caller | Input |
|--------|-------|
| `cli` | `ParseMusicXML(file_path)` |
| `cli` | `WriteMusicXML(file_path, piece, solution)` |

### Outgoing Dependencies (Efferent)

**None** (pure I/O module)

**External Library**:
- `tinyxml2.h` (DOM-based XML parsing)

---

## Coupling

### Afferent Coupling (Incoming)
- `cli` module
- `engine` module (reads `Piece` data)

**Count**: 2 (low)

### Efferent Coupling (Outgoing)
- None (only depends on `tinyxml2` library and `common/types.h`)

**Count**: 0 (minimal)

---

## API Specification

```cpp
// In src/musicxml/parser.h
namespace musicxml {

// Parse MusicXML file into Piece data structure
// Throws: ParseError if XML is malformed or file cannot be opened
Piece ParseMusicXML(const std::string& file_path);

// Write fingered Piece back to MusicXML file
// Throws: WriteError if file cannot be written
void WriteMusicXML(const std::string& file_path,
                   const Piece& piece,
                   const engine::Solution& solution,
                   bool force_overwrite = false);

// Exception types
class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg, int line_number = -1)
        : std::runtime_error(msg), line_number_(line_number) {}
    int GetLineNumber() const { return line_number_; }
private:
    int line_number_;
};

class WriteError : public std::runtime_error {
public:
    explicit WriteError(const std::string& msg) : std::runtime_error(msg) {}
};

}  // namespace musicxml
```

---

## Parsing Logic

### High-Level Algorithm

```cpp
Piece ParseMusicXML(const std::string& file_path) {
    // 1. Load XML document
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(file_path.c_str()) != tinyxml2::XML_SUCCESS) {
        throw ParseError("Cannot open file", 0);
    }

    // 2. Extract metadata
    Piece piece;
    piece.title = ExtractText(doc, "//work/work-title");
    piece.composer = ExtractText(doc, "//identification/creator[@type='composer']");

    // 3. Iterate through measures
    for (auto* measure_elem : IterateElements(doc, "//part/measure")) {
        Measure measure;
        measure.number = measure_elem->IntAttribute("number");

        // 4. Group notes by timestamp → Slices
        auto slices = GroupNotesIntoSlices(measure_elem);
        measure.slices = slices;

        piece.AddMeasure(measure);
    }

    return piece;
}
```

### Note Extraction (FR-1.3)

```xml
<note>
  <pitch>
    <step>C</step>
    <alter>1</alter>  <!-- Sharp -->
    <octave>4</octave>
  </pitch>
  <duration>4</duration>
  <voice>1</voice>
  <staff>1</staff>  <!-- Right hand -->
</note>
```

**Conversion to `Note` struct**:

```cpp
Note ParseNoteElement(tinyxml2::XMLElement* note_elem) {
    Note note;

    // Extract pitch
    auto* pitch_elem = note_elem->FirstChildElement("pitch");
    if (pitch_elem) {
        std::string step = pitch_elem->FirstChildElement("step")->GetText();  // C-B
        int alter = GetOptionalInt(pitch_elem, "alter", 0);  // -1=flat, 0=natural, 1=sharp
        note.octave = GetInt(pitch_elem, "octave");

        note.pitch = ConvertToModifiedKeyboard(step, alter);
    }

    // Extract metadata
    note.duration = GetInt(note_elem, "duration");
    note.voice = GetOptionalInt(note_elem, "voice", 1);
    int staff = GetOptionalInt(note_elem, "staff", 1);
    note.hand = (staff == 1) ? Hand::RIGHT : Hand::LEFT;

    // Check for rest
    note.is_rest = (note_elem->FirstChildElement("rest") != nullptr);

    return note;
}
```

### Modified Keyboard Distance Conversion (Problem Description Figure 2)

```
C  C#  D  D#  E  X  F  F#  G  G#  A  A#  B  X
0  1   2  3   4  5  6  7   8  9   10 11  12 13  (imaginary at 5, 13)
```

```cpp
Pitch ConvertToModifiedKeyboard(const std::string& step, int alter) {
    // Base pitch classes
    static const std::map<char, int> kBasePitches = {
        {'C', 0}, {'D', 2}, {'E', 4}, {'F', 6}, {'G', 8}, {'A', 10}, {'B', 12}
    };

    int base = kBasePitches.at(step[0]);
    int value = base + alter;  // Apply sharps/flats

    // Normalize to 0-14 range
    if (value < 0) value += 15;
    if (value >= 15) value -= 15;

    return Pitch{value};
}
```

### Slice Grouping

Notes at the same timestamp belong to the same slice:

```cpp
std::vector<Slice> GroupNotesIntoSlices(tinyxml2::XMLElement* measure_elem) {
    std::map<int, Slice> timestamp_map;  // timestamp → Slice

    int current_time = 0;
    for (auto* note_elem : IterateElements(measure_elem, "note")) {
        Note note = ParseNoteElement(note_elem);

        // Check for <chord> tag (indicates simultaneous note)
        bool is_chord = (note_elem->FirstChildElement("chord") != nullptr);
        if (!is_chord) {
            current_time += note.duration;  // Advance time
        }

        timestamp_map[current_time].AddNote(note);
    }

    // Convert map to vector (sorted by timestamp)
    std::vector<Slice> slices;
    for (auto& [_, slice] : timestamp_map) {
        slices.push_back(std::move(slice));
    }
    return slices;
}
```

---

## Writing Fingered MusicXML

### Insertion of Fingering Elements (FR-4.2, FR-4.3)

```xml
<note>
  <pitch>
    <step>C</step>
    <octave>4</octave>
  </pitch>
  <duration>4</duration>
  <notations>
    <technical>
      <fingering>1</fingering>  <!-- Thumb -->
    </technical>
  </notations>
</note>
```

**Implementation**:

```cpp
void WriteMusicXML(const std::string& file_path,
                   const Piece& piece,
                   const engine::Solution& solution,
                   bool force_overwrite) {
    // 1. Check if output file exists (FR-4.8)
    if (!force_overwrite && FileExists(file_path)) {
        throw WriteError("Output file already exists. Use --force to overwrite.");
    }

    // 2. Load original XML (to preserve all elements)
    tinyxml2::XMLDocument doc;
    doc.LoadFile(original_input_file.c_str());

    // 3. Iterate through notes and inject <fingering> elements
    size_t note_index = 0;
    for (auto* note_elem : IterateElements(doc, "//note")) {
        if (note_elem->FirstChildElement("rest")) {
            continue;  // Skip rests
        }

        // Get fingering from solution
        Finger finger = solution.GetFingering(note_index++);

        // Insert <fingering> element
        auto* notations = GetOrCreateElement(note_elem, "notations");
        auto* technical = GetOrCreateElement(notations, "technical");
        auto* fingering = doc.NewElement("fingering");
        fingering->SetText(static_cast<int>(finger));
        technical->InsertEndChild(fingering);
    }

    // 4. Write to file (FR-4.10: UTF-8 encoding)
    if (doc.SaveFile(file_path.c_str()) != tinyxml2::XML_SUCCESS) {
        throw WriteError("Cannot write file: " + std::string(doc.ErrorStr()));
    }
}
```

---

## Testing Strategy

### Unit Tests

| Test Case | Scenario | Expected Outcome |
|-----------|----------|------------------|
| `ParseValidFile` | Well-formed MusicXML | `Piece` with correct note count |
| `ParseMalformedXML` | Invalid XML syntax | Throws `ParseError` with line number |
| `ParseMissingFile` | File does not exist | Throws `ParseError` |
| `ExtractMetadata` | Parse title + composer | Metadata fields populated |
| `ExtractNotes` | Parse <note> elements | Correct pitch, octave, duration |
| `ExtractChords` | Notes with <chord> tag | Grouped into same slice |
| `ExtractStaff` | staff="1" vs staff="2" | Hand = RIGHT vs LEFT |
| `DefaultHandAssignment` | No staff attribute | Warning + default mapping |
| `ConvertPitch` | C#4 → Pitch{1} | Correct modified keyboard distance |
| `ConvertImaginaryKey` | E4 → F4 span | Accounts for imaginary black key |
| `GroupSlices` | Measure with 3 chords | 3 slices created |
| `WriteRoundTrip` | Parse → Write → Parse | All elements preserved |
| `WriteFingering` | Add fingering annotations | <fingering> elements present |
| `WriteOverwriteProtection` | File exists, no --force | Throws `WriteError` |

**Implementation**:

```cpp
TEST(MusicXMLTest, ParseValidFile) {
    auto piece = musicxml::ParseMusicXML("testdata/c_major_scale.xml");
    EXPECT_EQ(piece.GetNoteCount(), 15);  // 15 notes in C major scale
}

TEST(MusicXMLTest, ConvertPitch) {
    Pitch c_sharp = ConvertToModifiedKeyboard("C", 1);
    EXPECT_EQ(c_sharp.value, 1);
    EXPECT_TRUE(c_sharp.IsBlackKey());
}

TEST(MusicXMLTest, WriteRoundTrip) {
    auto original = musicxml::ParseMusicXML("testdata/input.xml");
    engine::Solution dummy_solution;  // Empty fingering

    musicxml::WriteMusicXML("/tmp/output.xml", original, dummy_solution, true);
    auto reloaded = musicxml::ParseMusicXML("/tmp/output.xml");

    EXPECT_EQ(original.GetNoteCount(), reloaded.GetNoteCount());
    EXPECT_EQ(original.title, reloaded.title);
}
```

### Integration Tests

| Test Case | Scenario | Verification |
|-----------|----------|--------------|
| `GoldenSetParsing` | Parse all 8 Golden Set pieces | No exceptions, note counts match expected |
| `MuseScoreImport` | Write fingered XML, import to MuseScore | Fingerings visible, no errors |
| `FinaleImport` | Write fingered XML, import to Finale | Fingerings visible, no errors |
| `SibeliusImport` | Write fingered XML, import to Sibelius | Fingerings visible, no errors |
| `PreserveElements` | Parse, optimize, write | Dynamics, lyrics, metadata intact |

---

## Error Handling

### File I/O Errors (FR-1.5)

```cpp
try {
    auto piece = musicxml::ParseMusicXML(input_file);
} catch (const musicxml::ParseError& e) {
    if (e.GetLineNumber() > 0) {
        std::cerr << "Error: Invalid MusicXML at line " << e.GetLineNumber()
                  << ": " << e.what() << '\n';
        return ExitCode::XMLParseError;
    } else {
        std::cerr << "Error: Cannot open input file '" << input_file
                  << "': " << e.what() << '\n';
        return ExitCode::InputFileError;
    }
}
```

### Validation Warnings (FR-1.8, FR-1.10)

```cpp
// Warn if staff assignment is missing
if (!note_elem->FirstChildElement("staff")) {
    std::cerr << "Warning: Assuming Staff 1 is Right Hand.\n";
}

// Warn if chord size exceeds limit (FR-1.10)
if (slice.Size() > Slice::kMaxNotesPerHand) {
    std::cerr << "Warning: Chord size (" << slice.Size()
              << ") exceeds limit (5). Processing anyway.\n";
}
```

---

## Implementation Notes

### TinyXML-2 API Usage

```cpp
// Load document
tinyxml2::XMLDocument doc;
doc.LoadFile("input.xml");

// Query elements
auto* root = doc.RootElement();
auto* element = root->FirstChildElement("measure");

// Iterate siblings
for (auto* elem = element; elem != nullptr; elem = elem->NextSiblingElement("measure")) {
    // Process each <measure>
}

// Extract text
const char* text = element->GetText();  // nullptr if no text
std::string value = text ? text : "";

// Extract attributes
int number = element->IntAttribute("number");
```

### Preservation of Original XML (FR-4.4)

**Strategy**: Load original XML, inject `<fingering>` elements, write back.

**Why not regenerate XML?**
- Dynamics, articulations, lyrics have complex schemas
- Easier to preserve by modifying in-place than reconstructing

**Trade-off**:
- ✅ Guaranteed preservation of all elements
- ❌ Must maintain reference to original file path

---

## File Structure

```
src/musicxml/
├── parser.h           // Public API (ParseMusicXML, WriteMusicXML)
├── parser.cpp         // Implementation
├── pitch_converter.cpp  // Modified keyboard distance conversion
├── slice_grouper.cpp    // Timestamp-based slice grouping
└── writer.cpp         // Fingering injection logic
```

---

## Dependencies

| Dependency | Purpose |
|------------|---------|
| `tinyxml2` | DOM-based XML parsing (external library) |
| `common/types.h` | `Hand`, `Finger`, `Pitch` enums |
| C++ stdlib | `<vector>`, `<string>`, `<map>`, `<stdexcept>` |

---

## Success Criteria

- ✅ Parse all 8 Golden Set pieces without crashes
- ✅ Round-trip (parse → write → parse) preserves all elements
- ✅ Fingering annotations importable to MuseScore, Finale, Sibelius
- ✅ Malformed XML reports line number in error message
- ✅ Missing staff assignment triggers warning, applies default
- ✅ Modified keyboard distance conversion matches Figure 2
- ✅ Chords (simultaneous notes) grouped into single slice
