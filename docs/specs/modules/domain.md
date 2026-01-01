# Module: Domain Model

## Responsibilities

Provide immutable value types representing musical and fingering concepts:

1. **Core Value Types**: `Pitch`, `Finger`, `Hand` - strongly-typed enums
2. **Note Representation**: `Note` - single sounding pitch with timing
3. **Temporal Grouping**: `Slice` - vertical time segment containing simultaneous notes
4. **Structural Units**: `Measure`, `Piece` - hierarchical composition
5. **Fingering Assignment**: `Fingering` - finger-to-note mapping

This module defines the **ubiquitous language** of the domain - all other modules depend on these types.

---

## Data Ownership

### Primary Entities

| Entity | Owned Data | Invariants |
|--------|-----------|-----------|
| `Pitch` | Modified keyboard distance (0-14 per octave) | Immutable, validated on construction |
| `Finger` | Finger number (1-5) | 1=Thumb, 5=Pinky |
| `Hand` | Enumeration (LEFT, RIGHT) | Binary choice |
| `Note` | `{Pitch pitch, int octave, int duration, bool is_rest, int staff, int voice}` | Duration > 0, staff ∈ {1,2}, voice ∈ {1,2,3,4} |
| `Slice` | `std::vector<Note> notes` | Max 5 notes per hand |
| `Measure` | `std::vector<Slice> slices, int number, TimeSig time_sig` | At least 1 slice |
| `Piece` | `std::vector<Measure> left_hand, std::vector<Measure> right_hand, Metadata metadata` | Parallel measure counts |
| `Fingering` | `std::vector<std::optional<Finger>> assignments` | 1:1 correspondence with notes |

### Derived Data

- **Measure Duration**: Sum of slice durations
- **Piece Length**: Total note count = sum(left_hand.notes) + sum(right_hand.notes)

---

## Communication

This is a **leaf module** - it has no outgoing dependencies.

**Inbound Communication:**
- Parser creates domain objects from MusicXML
- Optimizer reads/writes Fingering assignments
- Evaluator reads Note/Fingering data
- Generator reads domain objects to produce XML

**API Surface:**
```cpp
// Construction (immutable after creation)
Pitch::Pitch(int value);  // throws if value not in [0, 14]
Note::Note(Pitch p, int octave, int duration, bool is_rest, int staff, int voice);
Slice::Slice(std::vector<Note> notes);  // validates max 5 notes
Measure::Measure(int number, std::vector<Slice> slices, TimeSig sig);
Piece::Piece(std::vector<Measure> left, std::vector<Measure> right, Metadata meta);

// Accessors (all const)
Pitch Note::pitch() const noexcept;
int Note::octave() const noexcept;
const std::vector<Note>& Slice::notes() const noexcept;
const std::vector<Measure>& Piece::left_hand() const noexcept;

// Utilities
int Pitch::distance_to(Pitch other) const noexcept;  // Keyboard distance
bool Note::is_black_key() const noexcept;  // pitch % 2 == 1
```

---

## Coupling Analysis

### Afferent Coupling (High - Good for Leaf Module)
Depended upon by:
- **Parser** (constructs domain objects)
- **Optimizer** (reads/writes Fingering)
- **Evaluator** (reads Note data for rule evaluation)
- **Generator** (reads Piece for XML output)
- **CLI** (passes domain objects between modules)

### Efferent Coupling (Zero - Ideal)
Depends on:
- C++ Standard Library only (`<vector>`, `<optional>`, `<string>`)

**Stability Metric:** Instability = Efferent / (Afferent + Efferent) = 0 / (4 + 0) = **0.0** (perfectly stable)

---

## Testing Strategy

### Unit Tests

#### Value Type Invariants
1. **Pitch Construction**
   - Valid range [0, 14] accepted
   - Out-of-range throws `std::invalid_argument`
   - Distance calculation: `Pitch(5).distance_to(Pitch(10))` → 5

2. **Note Validation**
   - Valid construction with all fields
   - Octave range [0, 10] enforced
   - Duration must be positive
   - Staff must be 1 or 2
   - `is_black_key()` correctness (1,3,6,8,10,13 are black)

3. **Slice Validation**
   - Empty slice allowed (rest measure)
   - Max 5 notes enforced (throws if exceeded)
   - Notes sorted by pitch on construction

4. **Measure Construction**
   - Measure number uniqueness not enforced (parser's responsibility)
   - Time signature validation (numerator > 0, denominator power of 2)

5. **Piece Construction**
   - Left and right hand measure counts need not match
   - Metadata preserved (title, composer)

#### Example Test Cases
```cpp
TEST(PitchTest, ValidConstruction) {
  EXPECT_NO_THROW(Pitch(0));
  EXPECT_NO_THROW(Pitch(14));
  EXPECT_THROW(Pitch(15), std::invalid_argument);
}

TEST(NoteTest, BlackKeyDetection) {
  Note C4(Pitch(1), 4, 480, false, 1, 1);  // C is white (1 is white)
  Note CSharp4(Pitch(2), 4, 480, false, 1, 1);  // C# is black (2 is black? No - pitch 1=C, pitch 2=C#)
  // Actually: pitch 0=C, 1=C#, 2=D, 3=D#, 4=E, 5=F(imaginary), 6=F#, ...
  // Black keys: odd pitches (1,3,6,8,10,13)
  EXPECT_FALSE(Note(Pitch(0), 4, 480, false, 1, 1).is_black_key());  // C
  EXPECT_TRUE(Note(Pitch(1), 4, 480, false, 1, 1).is_black_key());   // C#
}

TEST(SliceTest, MaxNotesEnforced) {
  std::vector<Note> six_notes(6, Note(Pitch(0), 4, 480, false, 1, 1));
  EXPECT_THROW(Slice(six_notes), std::invalid_argument);
}
```

### Integration Tests
**N/A** - No I/O or cross-module dependencies to test in isolation.

---

## Design Constraints

1. **Immutability**: All domain objects are `const`-correct. No setters. Use builder pattern if needed.
2. **Value Semantics**: Pass by value or `const&`. No `shared_ptr` unless proven necessary by profiling.
3. **No Heap Allocations**: Prefer stack-allocated small objects. `std::vector` for collections is acceptable (uses heap but manages lifetime).
4. **Thread-Safe**: Immutability guarantees thread-safety for read operations.

---

## File Structure

```
include/domain/
  types.h           // Pitch, Finger, Hand enums + utility functions
  note.h            // Note, Slice, Measure classes
  piece.h           // Piece class + Metadata struct
  fingering.h       // Fingering class
```

---

## Critical Implementation Details

### Modified Keyboard Distance (Pitch)

Per problem description, includes imaginary black keys:
```
C=0, C#=1, D=2, D#=3, E=4, [F-imaginary]=5, F#=6, G=7, G#=8, A=9, A#=10, B=11, [C-imaginary]=12, (repeat next octave at +14)
```

**Black key detection:**
```cpp
bool is_black_key(Pitch p) {
  int mod = p.value() % 14;
  return mod == 1 || mod == 3 || mod == 6 || mod == 8 || mod == 10 || mod == 13;
}
```

### Fingering Representation

```cpp
class Fingering {
  std::vector<std::optional<Finger>> assignments_;  // Index matches note index in Slice
public:
  explicit Fingering(size_t num_notes) : assignments_(num_notes, std::nullopt) {}

  void assign(size_t note_idx, Finger f);
  std::optional<Finger> get(size_t note_idx) const;

  // Validation
  bool is_complete() const;  // All notes have assignments
  bool violates_hard_constraint() const;  // Check for duplicate fingers in same slice
};
```

---

## Dependencies

- **External**: None (C++ stdlib only)
- **Internal**: None (leaf module)

---

## Future Extensibility

Potential additions (out of scope for v1.0):
- **Cross-hand constraints**: Notes where left hand crosses above right
- **Pedaling annotations**: Damper pedal markings
- **Articulation context**: Legato vs. staccato affects fingering difficulty
