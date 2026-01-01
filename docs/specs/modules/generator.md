# Module: MusicXML Generator

## Responsibilities

1. **Insert fingering annotations** into preserved XML DOM
2. **Preserve all original elements**: Dynamics, articulations, lyrics, metadata
3. **Generate valid MusicXML 3.x output** conforming to W3C schema
4. **Write UTF-8 encoded file** to specified output path
5. **Handle filesystem errors** gracefully (permissions, disk space)

---

## Data Ownership

### Input (Transferred from Caller)

| Data | Type | Source |
|------|------|--------|
| `xml_doc_` | `pugi::xml_document` | From Parser (original DOM) |
| `fingering_` | `Fingering` | From Optimizer |
| `piece_` | `Piece` | From Parser (for note mapping) |

### Output

| Data | Type | Description |
|------|------|-------------|
| Output file | MusicXML (.musicxml) | UTF-8 encoded XML with `<fingering>` elements |

---

## Communication

### Inbound API

```cpp
class MusicXMLGenerator {
public:
  // Generate fingered MusicXML output
  static void generate(
    const std::filesystem::path& output_path,
    pugi::xml_document& xml_doc,
    const Piece& piece,
    const Fingering& left_hand_fingering,
    const Fingering& right_hand_fingering,
    bool force_overwrite = false
  );

private:
  // Insert <fingering> elements into DOM
  static void insert_fingerings(
    pugi::xml_node& part,
    const std::vector<Measure>& measures,
    const Fingering& fingering
  );

  // Create <technical> + <fingering> nodes
  static void add_fingering_to_note(pugi::xml_node& note, Finger finger);
};
```

### Outbound Dependencies

- **Domain**: Reads `Piece`, `Measure`, `Slice`, `Fingering`
- **pugixml**: XML DOM manipulation

---

## Coupling Analysis

### Afferent Coupling

- **CLI/Orchestrator**: Calls `generate()` after optimization completes

### Efferent Coupling

- **Domain** (read-only access)
- **pugixml** (external, XML writing)

**Instability:** Low (simple adapter, no complex logic)

---

## Testing Strategy

### Unit Tests

#### 1. Fingering Element Insertion

```cpp
TEST(GeneratorTest, InsertFingeringElement) {
  pugi::xml_document doc;
  auto note = doc.append_child("note");
  note.append_child("pitch").append_child("step").text() = "C";
  note.append_child("duration").text() = "480";

  MusicXMLGenerator::add_fingering_to_note(note, Finger::THUMB);

  // Verify structure: <note> -> <notations> -> <technical> -> <fingering>1</fingering>
  auto notations = note.child("notations");
  ASSERT_TRUE(notations);

  auto technical = notations.child("technical");
  ASSERT_TRUE(technical);

  auto fingering = technical.child("fingering");
  ASSERT_TRUE(fingering);
  EXPECT_EQ(fingering.text().as_int(), 1);  // Thumb = 1
}
```

#### 2. Preserve Existing Notations

```cpp
TEST(GeneratorTest, PreserveExistingArticulations) {
  pugi::xml_document doc;
  auto note = doc.append_child("note");
  auto notations = note.append_child("notations");
  notations.append_child("articulations").append_child("staccato");

  MusicXMLGenerator::add_fingering_to_note(note, Finger::INDEX);

  // Verify both staccato and fingering exist
  EXPECT_TRUE(notations.child("articulations").child("staccato"));
  EXPECT_TRUE(notations.child("technical").child("fingering"));
}
```

### Integration Tests

#### 1. Round-Trip Test (Parse → Generate → Parse)

```cpp
TEST(GeneratorIntegrationTest, RoundTripPreservesAllElements) {
  // Parse original
  auto parse_result = MusicXMLParser::parse("tests/baseline/czerny_op821_1.musicxml");

  // Generate dummy fingering
  Fingering dummy(/* num_notes */ 100);
  for (size_t i = 0; i < 100; ++i) {
    dummy.assign(i, Finger::THUMB);
  }

  // Generate output
  fs::path output = fs::temp_directory_path() / "roundtrip.musicxml";
  MusicXMLGenerator::generate(output, *parse_result.original_xml, parse_result.piece, dummy, dummy);

  // Re-parse output
  auto reparsed = MusicXMLParser::parse(output);

  // Verify all measures preserved
  EXPECT_EQ(reparsed.piece.right_hand().size(), parse_result.piece.right_hand().size());

  // Verify fingering added
  // (Check that <fingering> elements exist in DOM)
  pugi::xml_document& doc = *reparsed.original_xml;
  size_t fingering_count = doc.select_nodes("//fingering").size();
  EXPECT_GT(fingering_count, 0);
}
```

#### 2. MuseScore Import Test (Manual)

**Procedure:**
1. Generate fingered MusicXML: `piano-fingering tests/baseline/czerny_op821_1.musicxml output.musicxml`
2. Open `output.musicxml` in MuseScore 4
3. Verify:
   - All notes present with correct pitches
   - Fingering numbers visible above noteheads
   - Dynamics, slurs, articulations intact

**Acceptance Criterion:** MuseScore imports without errors, fingering visible.

#### 3. File Overwrite Protection

```cpp
TEST(GeneratorIntegrationTest, OverwriteProtectionThrows) {
  fs::path output = "existing_file.musicxml";
  std::ofstream(output) << "dummy content";  // Create existing file

  pugi::xml_document doc;
  Piece piece;
  Fingering fingering;

  // Without --force flag
  EXPECT_THROW(
    MusicXMLGenerator::generate(output, doc, piece, fingering, fingering, /* force */ false),
    FileExistsError
  );

  // With --force flag
  EXPECT_NO_THROW(
    MusicXMLGenerator::generate(output, doc, piece, fingering, fingering, /* force */ true)
  );
}
```

---

## Design Constraints

1. **No XML schema validation**: Assume Parser produced valid structure
2. **UTF-8 encoding mandatory**: Specified in SRS FR-4.10
3. **Idempotent insertion**: Multiple calls to `add_fingering_to_note()` should not duplicate elements
4. **Preserve original formatting**: Maintain indentation and whitespace from input

---

## File Structure

```
include/generator/
  musicxml_generator.h       // Public API
src/generator/
  musicxml_generator.cpp     // Implementation
```

---

## Critical Implementation Details

### Fingering Element Insertion

Per MusicXML 3.1 spec, fingering goes inside `<technical>` within `<notations>`:

```xml
<note>
  <pitch>
    <step>C</step>
    <octave>4</octave>
  </pitch>
  <duration>480</duration>
  <voice>1</voice>
  <notations>
    <technical>
      <fingering>3</fingering>  <!-- Inserted here -->
    </technical>
  </notations>
</note>
```

**Implementation:**
```cpp
void MusicXMLGenerator::add_fingering_to_note(pugi::xml_node& note, Finger finger) {
  // Get or create <notations> node
  auto notations = note.child("notations");
  if (!notations) {
    notations = note.append_child("notations");
  }

  // Get or create <technical> node
  auto technical = notations.child("technical");
  if (!technical) {
    technical = notations.append_child("technical");
  }

  // Remove existing <fingering> if present (idempotent)
  auto existing = technical.child("fingering");
  if (existing) {
    technical.remove_child(existing);
  }

  // Insert new <fingering>
  auto fingering_node = technical.append_child("fingering");
  fingering_node.text() = static_cast<int>(finger);
}
```

### Note-to-Fingering Mapping

Challenge: Match notes in DOM to fingering indices.

**Strategy:**
1. Traverse DOM notes in document order
2. Maintain counter for left/right hand separately
3. Use staff attribute to determine hand
4. Skip rests (no fingering assignment)

```cpp
void MusicXMLGenerator::insert_fingerings(
  pugi::xml_node& part,
  const std::vector<Measure>& measures,
  const Fingering& fingering
) {
  size_t fingering_idx = 0;

  for (auto measure : part.children("measure")) {
    for (auto note : measure.children("note")) {
      // Skip rests
      if (note.child("rest")) continue;

      // Check if part of chord (same fingering index as previous)
      bool is_chord = note.child("chord");
      if (!is_chord && fingering_idx > 0) {
        ++fingering_idx;
      }

      // Validate index in bounds
      if (fingering_idx >= fingering.size()) {
        throw std::runtime_error("Fingering index out of bounds");
      }

      auto finger = fingering.get(fingering_idx);
      if (finger.has_value()) {
        add_fingering_to_note(note, *finger);
      }
    }
  }
}
```

### UTF-8 Output

```cpp
void MusicXMLGenerator::generate(
  const fs::path& output_path,
  pugi::xml_document& xml_doc,
  const Piece& piece,
  const Fingering& left_hand_fingering,
  const Fingering& right_hand_fingering,
  bool force_overwrite
) {
  // Check file existence
  if (!force_overwrite && fs::exists(output_path)) {
    throw FileExistsError("Output file '" + output_path.string() + "' already exists. Use --force to overwrite.");
  }

  // Insert fingerings into DOM
  auto parts = xml_doc.select_nodes("//part");
  if (parts.size() >= 1) {
    insert_fingerings(parts[0].node(), piece.right_hand(), right_hand_fingering);
  }
  if (parts.size() >= 2) {
    insert_fingerings(parts[1].node(), piece.left_hand(), left_hand_fingering);
  }

  // Write to file with UTF-8 encoding
  if (!xml_doc.save_file(output_path.c_str(), "  ", pugi::format_default, pugi::encoding_utf8)) {
    throw FileWriteError("Cannot write output file '" + output_path.string() + "': " + std::strerror(errno));
  }
}
```

---

## Dependencies

- **Domain**: Reads `Piece`, `Measure`, `Fingering`
- **pugixml**: XML DOM manipulation and file writing

---

## Error Handling

| Error Condition | Exception Type | Exit Code | Message Format |
|----------------|---------------|-----------|----------------|
| Output file exists (no --force) | `FileExistsError` | 5 | `Error: Output file '<path>' already exists. Use --force to overwrite.` |
| Cannot write to file (permissions) | `FileWriteError` | 6 | `Error: Cannot write output file '<path>': <reason>` |
| Disk space full | `FileWriteError` | 6 | `Error: Cannot write output file '<path>': No space left on device` |
| Fingering index out of bounds | `std::runtime_error` | 7 | `Internal error: Fingering size mismatch` |

---

## Future Extensibility

- **Compressed MusicXML (.mxl)**: Zip output with `META-INF/container.xml`
- **Batch processing**: Generate multiple outputs from single optimization run
- **Annotation options**: User choice of fingering placement (above/below noteheads)
