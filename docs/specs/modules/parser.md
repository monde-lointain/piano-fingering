# Module: MusicXML Parser

> **Status:** ✅ Implemented

## Responsibilities

1. **Read MusicXML files** from filesystem using pugixml
2. **Extract musical elements**: Notes, chords, rests, measure boundaries
3. **Build domain objects**: Transform XML → `Piece` structure
4. **Preserve original DOM**: Store full XML tree for later passthrough
5. **Handle errors gracefully**: Invalid XML, missing elements, I/O failures

---

## Data Ownership

### Input

| Data | Type | Source |
|------|------|--------|
| MusicXML file path | `std::filesystem::path` | CLI argument |

### Output (Transferred to Caller)

| Data | Type | Description |
|------|------|-------------|
| `Piece` | Domain object | Parsed musical structure (ownership transferred) |
| `pugi::xml_document` | XML DOM | Original tree for passthrough (ownership transferred) |

### Internal State

| Data | Type | Lifetime |
|------|------|----------|
| `xml_doc_` | `pugi::xml_document` | Temporary during parsing, then moved to caller |
| `current_staff_` | `int` | Parsing context (1 or 2) |
| `current_measure_` | `int` | Measure number being processed |

---

## Communication

### Inbound API

```cpp
class MusicXMLParser {
public:
  struct ParseResult {
    Piece piece;                      // Parsed domain object
    std::unique_ptr<pugi::xml_document> original_xml;  // For passthrough
  };

  // Parse file and extract Piece + DOM
  static ParseResult parse(const std::filesystem::path& xml_path);

private:
  // Internal helpers
  static std::vector<Note> extract_notes_from_measure(const pugi::xml_node& measure, int staff);
  static Slice build_slice_from_chord(const std::vector<pugi::xml_node>& chord_notes);
  static Pitch xml_step_to_pitch(const std::string& step, int alter);
};
```

### Outbound Dependencies

- **Domain**: Constructs `Piece`, `Measure`, `Slice`, `Note`, `Pitch`
- **pugixml**: XML parsing library (external)

---

## Coupling Analysis

### Afferent Coupling

- **CLI/Orchestrator**: Calls `parse()` to load input file

### Efferent Coupling

- **Domain** (constructs objects)
- **pugixml** (external, header-only)

**Instability:** Medium (adapter layer - depends on domain + external lib)

---

## Testing Strategy

### Unit Tests

#### 1. Single Note Extraction

**Input XML:**
```xml
<note>
  <pitch>
    <step>C</step>
    <octave>4</octave>
  </pitch>
  <duration>480</duration>
  <voice>1</voice>
  <staff>1</staff>
</note>
```

**Test:**
```cpp
TEST(ParserTest, SingleNoteExtraction) {
  pugi::xml_document doc;
  doc.load_string(/* XML above */);
  auto note_node = doc.child("note");

  Note note = MusicXMLParser::extract_note(note_node);

  EXPECT_EQ(note.pitch().value(), 0);  // C = 0 in modified system
  EXPECT_EQ(note.octave(), 4);
  EXPECT_EQ(note.duration(), 480);
  EXPECT_EQ(note.staff(), 1);
}
```

#### 2. Chord Extraction

**Input XML:**
```xml
<note>
  <pitch><step>C</step><octave>4</octave></pitch>
  <duration>480</duration>
  <staff>1</staff>
</note>
<note>
  <chord/>  <!-- Indicates simultaneity with previous note -->
  <pitch><step>E</step><octave>4</octave></pitch>
  <duration>480</duration>
  <staff>1</staff>
</note>
```

**Test:**
```cpp
TEST(ParserTest, ChordExtraction) {
  // Parse XML with 2 simultaneous notes
  Slice slice = /* parse logic */;

  EXPECT_EQ(slice.notes().size(), 2);
  EXPECT_EQ(slice.notes()[0].pitch().value(), 0);  // C
  EXPECT_EQ(slice.notes()[1].pitch().value(), 4);  // E
}
```

#### 3. Rest Handling

**Input XML:**
```xml
<note>
  <rest/>
  <duration>480</duration>
  <staff>1</staff>
</note>
```

**Test:**
```cpp
TEST(ParserTest, RestHandling) {
  Note note = /* parse */;
  EXPECT_TRUE(note.is_rest());
}
```

#### 4. Staff Assignment Defaults (FR-1.7)

**Input XML (no explicit staff):**
```xml
<part id="P1">
  <measure number="1">
    <note>
      <pitch><step>C</step><octave>5</octave></pitch>
      <duration>480</duration>
    </note>
  </measure>
</part>
```

**Test:**
```cpp
TEST(ParserTest, DefaultStaffAssignment) {
  ParseResult result = MusicXMLParser::parse("single_part.xml");

  // First part defaults to staff=1 (right hand)
  EXPECT_EQ(result.piece.right_hand().size(), 1);
  EXPECT_EQ(result.piece.left_hand().size(), 0);
}
```

#### 5. Altered Notes (Sharps/Flats)

**Input XML:**
```xml
<pitch>
  <step>C</step>
  <alter>1</alter>  <!-- Sharp -->
  <octave>4</octave>
</pitch>
```

**Test:**
```cpp
TEST(ParserTest, SharpNotes) {
  Pitch p = xml_step_to_pitch("C", 1);  // C# = pitch 1 in modified system
  EXPECT_EQ(p.value(), 1);
}
```

### Integration Tests

#### 1. Parse All Golden Set Files

```cpp
TEST(ParserIntegrationTest, ParseGoldenSet) {
  const std::vector<std::string> golden_set = {
    "czerny_op821_1.musicxml",
    "czerny_op821_37.musicxml",
    // ... all 8 pieces
  };

  for (const auto& filename : golden_set) {
    fs::path path = "tests/baseline/" + filename;
    EXPECT_NO_THROW({
      auto result = MusicXMLParser::parse(path);
      EXPECT_GT(result.piece.right_hand().size(), 0);  // Has measures
    });
  }
}
```

#### 2. Malformed XML Error Handling

**Input:** Invalid XML (unclosed tags)
```xml
<note>
  <pitch>
    <step>C</step>
  <!-- Missing </pitch> and </note> -->
```

**Test:**
```cpp
TEST(ParserIntegrationTest, MalformedXMLThrows) {
  EXPECT_THROW(
    MusicXMLParser::parse("malformed.xml"),
    ParsingError
  );
}
```

**Expected Error Message:**
```
piano-fingering: Error: Parser: Invalid MusicXML at line 3: Unexpected end of file
```

#### 3. File Not Found

```cpp
TEST(ParserIntegrationTest, FileNotFoundThrows) {
  EXPECT_THROW(
    MusicXMLParser::parse("nonexistent.xml"),
    FileNotFoundError
  );
}
```

**Expected Exit Code:** 2 (per SRS FR-1.5)

---

## Design Constraints

1. **DOM Preservation**: Must keep original XML tree intact for Generator to reuse
2. **Single-Pass Parsing**: No need to reparse - build domain objects in one traversal
3. **Graceful Degradation**: Warn on unsupported elements (e.g., lyrics, dynamics) but continue
4. **MusicXML 3.x Compatibility**: Support both compressed (.mxl) and uncompressed (.musicxml) formats

---

## File Structure

```
include/parser/
  musicxml_parser.h      // Public API
  pitch_mapping.h        // Step+Alter → Modified Pitch conversion
src/parser/
  musicxml_parser.cpp    // Implementation
  pitch_mapping.cpp
```

---

## Critical Implementation Details

### Pitch Conversion (Step + Alter → Modified Pitch)

MusicXML uses `<step>` (A-G) + `<alter>` (-2 to +2 for double-flat to double-sharp):

```cpp
Pitch xml_step_to_pitch(const std::string& step, int alter) {
  // Base pitches (C=0 in our modified system)
  static const std::unordered_map<std::string, int> BASE_PITCHES = {
    {"C", 0}, {"D", 2}, {"E", 4}, {"F", 6}, {"G", 7}, {"A", 9}, {"B", 11}
  };

  int base = BASE_PITCHES.at(step);
  int modified = (base + alter) % 14;  // Wrap around octave
  if (modified < 0) modified += 14;

  return Pitch(modified);
}
```

**Example:**
- `C# (C + alter=1)` → 0 + 1 = **1** (correct, C# is pitch 1)
- `Db (D + alter=-1)` → 2 + (-1) = **1** (enharmonic equivalent)
- `E (E + alter=0)` → 4 + 0 = **4** (correct)

### Staff Assignment Logic

```cpp
int determine_staff(const pugi::xml_node& note, int default_staff) {
  auto staff_node = note.child("staff");
  if (staff_node) {
    return staff_node.text().as_int();
  }

  // FR-1.7: Default to staff 1 if not specified
  static bool warned = false;
  if (!warned) {
    std::cerr << "Warning: Assuming Staff 1 is Right Hand.\n";
    warned = true;
  }
  return default_staff;
}
```

### Chord Detection

MusicXML marks chord notes with `<chord/>` element:

```cpp
std::vector<Slice> extract_slices_from_measure(const pugi::xml_node& measure) {
  std::vector<Note> current_chord;
  std::vector<Slice> slices;

  for (auto note_node : measure.children("note")) {
    if (note_node.child("chord")) {
      // Part of current chord
      current_chord.push_back(extract_note(note_node));
    } else {
      // New time slice - finalize previous chord if exists
      if (!current_chord.empty()) {
        slices.push_back(Slice(std::move(current_chord)));
        current_chord.clear();
      }
      current_chord.push_back(extract_note(note_node));
    }
  }

  // Finalize last chord
  if (!current_chord.empty()) {
    slices.push_back(Slice(std::move(current_chord)));
  }

  return slices;
}
```

---

## Dependencies

### External Libraries

- **pugixml** (header-only, MIT license)
  - Fast, lightweight XML parser
  - XPath support for complex queries
  - DOM manipulation for preservation

### Internal Dependencies

- **Domain**: All entity types (Pitch, Note, Slice, Measure, Piece)

---

## Error Handling

| Error Condition | Exception Type | Exit Code | Message Format |
|----------------|---------------|-----------|----------------|
| File not found | `FileNotFoundError` | 2 | `Error: Cannot open input file '<path>': <reason>` |
| Malformed XML | `ParsingError` | 3 | `Error: Invalid MusicXML at line <N>: <parser_message>` |
| Missing required elements | `ParsingError` | 3 | `Error: Missing required element: <element_name>` |
| Invalid pitch data | `ParsingError` | 3 | `Error: Invalid pitch at measure <M>: <details>` |

---

## Future Extensibility

- **Compressed MusicXML (.mxl)**: Unzip and parse `META-INF/container.xml`
- **Multi-part scores**: Handle more than 2 staves (e.g., piano + vocal)
- **Backup/forward elements**: Handle irregular timing (grace notes, triplets)
