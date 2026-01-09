# Module: Score Evaluator

## Responsibilities

1. **Implement 15 fingering rules** from Parncutt et al. (1997) with cascading penalties
2. **Compute difficulty scores** for complete fingerings
3. **Provide delta evaluation** for incremental updates during ILS
4. **Distinguish monophonic vs. polyphonic** rule application
5. **Enforce hard constraints** (same finger cannot play two simultaneous notes)

---

## Data Ownership

### Precomputed Tables

| Data | Type | Purpose |
|------|------|---------|
| `finger_pair_distances_` | `std::array<std::array<int, 10>, 5*5>` | Cache computed distances for all finger pairs |
| `black_key_lookup_` | `std::array<bool, 14>` | Fast `is_black_key(pitch)` without modulo |

### Runtime State (per evaluation)

| Data | Type | Description |
|------|------|-------------|
| `config_` | `const Config*` | Distance matrix + rule weights |
| `cache_` | `mutable std::unique_ptr<CacheData>` | PIMPL storing note vectors for delta evaluation |

**CacheData** (implementation detail):
- `std::vector<NoteInfo> notes` - Cached note vector from last evaluate()
- `size_t fingerings_size` - Validation that cached notes match current fingerings size
- `Hand hand` - Validation that cached notes match current hand

---

## Communication

### Inbound API

```cpp
class ScoreEvaluator {
public:
  struct SliceLocation {
    size_t measure_idx;
    size_t slice_idx;
    size_t note_idx_in_slice;
    size_t fingering_idx;
  };

  explicit ScoreEvaluator(const Config& cfg);

  ~ScoreEvaluator();  // Needed for unique_ptr<CacheData>

  // Move-only for PIMPL pattern
  ScoreEvaluator(ScoreEvaluator&&) noexcept;
  ScoreEvaluator& operator=(ScoreEvaluator&&) noexcept;

  // Full evaluation (used in Beam Search initial pass)
  // Also populates internal cache for efficient delta evaluation
  double evaluate(
    const Piece& piece,
    const std::vector<Fingering>& fingerings,
    Hand hand
  ) const;

  // Delta evaluation (used in ILS for incremental updates)
  // Reuses cached note vectors from prior evaluate() call for O(S) amortized performance
  double evaluate_delta(
    const Piece& piece,
    const std::vector<Fingering>& current_fingerings,
    const std::vector<Fingering>& proposed_fingerings,
    const SliceLocation& changed_location,
    Hand hand
  ) const;

private:
  const Config* config_;

  struct CacheData;  // PIMPL - stores note vectors for delta evaluation
  mutable std::unique_ptr<CacheData> cache_;
};
```

### Outbound Dependencies

- **Domain**: Reads `Piece`, `Slice`, `Note`, `Finger`
- **Config**: Reads `DistanceMatrix`, `RuleWeights`

---

## Coupling Analysis

### Afferent Coupling

- **Optimizer**: Calls `evaluate()` and `evaluate_delta()` millions of times

### Efferent Coupling

- **Domain** (read-only access)
- **Config** (read-only access)

**Instability:** Low (pure computation module, no side effects)

---

## Testing Strategy

### Unit Tests (One Test Per Rule Minimum)

#### Rule 1: Below MinComf or Above MaxComf

```cpp
TEST(EvaluatorTest, Rule1_ComfortViolation) {
  Config cfg = Config::load_preset("Medium");
  ScoreEvaluator eval(cfg);

  // Create two notes with distance violating MaxComf for fingers 1-2
  // MaxComf(1-2) = 8, so distance 10 violates by 2 units
  Note n1(Pitch(0), 4, 480, false, 1, 1);  // C4
  Note n2(Pitch(10), 4, 480, false, 1, 1); // A#4 (distance = 10)

  Finger f1 = Finger::THUMB;
  Finger f2 = Finger::INDEX;

  // Rule 1: +2 per unit beyond MaxComf = 2 units * 2 = +4
  // But cascading! Also triggers Rule 2 (+1) and possibly Rule 13
  double penalty = eval.rule1_comfort_distance(n1, f1, n2, f2);

  EXPECT_EQ(penalty, 2 * 2);  // 2 units * +2 penalty = 4 (Rule 1 only)
}
```

#### Rule 2: Below MinRel or Above MaxRel (Base Cascading)

```cpp
TEST(EvaluatorTest, Rule2_RelaxedViolation) {
  Config cfg = Config::load_preset("Medium");
  ScoreEvaluator eval(cfg);

  // MaxRel(1-2) = 5, distance 6 violates by 1 unit
  Note n1(Pitch(0), 4, 480, false, 1, 1);  // C4
  Note n2(Pitch(6), 4, 480, false, 1, 1);  // F#4 (distance = 6)

  double penalty = eval.rule2_relaxed_distance(n1, Finger::THUMB, n2, Finger::INDEX);

  EXPECT_EQ(penalty, 1);  // 1 unit * +1 penalty
}
```

#### Rule 13: Below MinPrac or Above MaxPrac (Severe Cascading)

```cpp
TEST(EvaluatorTest, Rule13_PracticalViolation) {
  Config cfg = Config::load_preset("Medium");
  ScoreEvaluator eval(cfg);

  // MaxPrac(1-2) = 10, distance 12 violates by 2 units
  Note n1(Pitch(0), 4, 480, false, 1, 1);  // C4
  Note n2(Pitch(12), 4, 480, false, 1, 1); // [C-imaginary]4 (distance = 12)

  // Cascading: Rule 2 (+1/unit) + Rule 1 (+2/unit) + Rule 13 (+10/unit) = +13/unit
  // Total: 2 units * 13 = 26
  double penalty = eval.evaluate_distance_with_cascading(n1, Finger::THUMB, n2, Finger::INDEX);

  EXPECT_EQ(penalty, 2 * (1 + 2 + 10));  // 26
}
```

#### Rule 5: Fourth Finger Usage

```cpp
TEST(EvaluatorTest, Rule5_FourthFingerPenalty) {
  Config cfg = Config::load_preset("Medium");
  ScoreEvaluator eval(cfg);

  Note n1(Pitch(0), 4, 480, false, 1, 1);
  Note n2(Pitch(2), 4, 480, false, 1, 1);

  Fingering f;
  f.assign(0, Finger::RING);  // Finger 4 (ring finger)

  // Rule 5: +1 per use of fourth finger
  double penalty = eval.rule5_fourth_finger_usage(n1, Finger::RING);
  EXPECT_EQ(penalty, 1.0);
}
```

#### Rule 8: Thumb on Black Key

```cpp
TEST(EvaluatorTest, Rule8_ThumbOnBlack) {
  Config cfg = Config::load_preset("Medium");
  ScoreEvaluator eval(cfg);

  Note black_key(Pitch(1), 4, 480, false, 1, 1);  // C# (pitch 1 is black)

  // Rule 8: +0.5 for thumb on black
  double penalty = eval.rule8_thumb_on_black(black_key, Finger::THUMB, /* before */ std::nullopt, /* after */ std::nullopt);
  EXPECT_EQ(penalty, 0.5);
}
```

#### Rule 14: Chord Constraints with Doubled Penalties

```cpp
TEST(EvaluatorTest, Rule14_ChordDoublesRules1And2) {
  Config cfg = Config::load_preset("Medium");
  ScoreEvaluator eval(cfg);

  // Chord: C4 (thumb) + A#4 (index) - distance 10 violates MaxComf(1-2)=8 by 2 units
  Slice chord({
    Note(Pitch(0), 4, 480, false, 1, 1),
    Note(Pitch(10), 4, 480, false, 1, 1)
  });

  Fingering f;
  f.assign(0, Finger::THUMB);
  f.assign(1, Finger::INDEX);

  // Within chord: Rule 1 penalty is DOUBLED: (2 units * 2 penalty) * 2 = 8
  double penalty = eval.rule14_chord_constraints(chord, f);
  EXPECT_EQ(penalty, 8);  // Doubled Rule 1 penalty
}
```

#### Rule 15: Same Pitch, Different Finger

```cpp
TEST(EvaluatorTest, Rule15_SamePitchDifferentFinger) {
  Config cfg = Config::load_preset("Medium");
  ScoreEvaluator eval(cfg);

  Slice s1({Note(Pitch(0), 4, 480, false, 1, 1)});  // C4
  Slice s2({Note(Pitch(0), 4, 480, false, 1, 1)});  // C4 again

  Fingering f1, f2;
  f1.assign(0, Finger::THUMB);
  f2.assign(0, Finger::INDEX);  // Different finger for same pitch

  double penalty = eval.rule15_same_pitch_different_finger(s1, s2, f1, f2);
  EXPECT_EQ(penalty, 1.0);
}
```

### Unit Tests: Cascading Penalty Accumulation

```cpp
TEST(EvaluatorTest, CascadingPenalties_FullStack) {
  Config cfg = Config::load_preset("Medium");
  ScoreEvaluator eval(cfg);

  // Design scenario: distance 3 units below MinPrac
  // Triggers all three cascading rules:
  // - Rule 2 (base): 3 * 1 = 3
  // - Rule 1 (additional): 3 * 2 = 6
  // - Rule 13 (additional): 3 * 10 = 30
  // Total: 39

  // Use finger pair 1-2 with MinPrac=-8
  // Need distance of -11 to be 3 units below MinPrac
  Note n1(Pitch(0), 4, 480, false, 1, 1);   // C4
  Note n2(Pitch(3), 4, 480, false, 1, 1);   // D#4 (distance = 3)
  // Wait, that's positive distance. Need negative distance.
  // Negative distance = n2 pitch < n1 pitch
  Note n1_high(Pitch(0), 5, 480, false, 1, 1);  // C5
  Note n2_low(Pitch(3), 4, 480, false, 1, 1);   // D#4 (n2 is 14-3=11 semitones below)

  // Actually, use keyboard distance formula:
  // distance = (n2.pitch + n2.octave*14) - (n1.pitch + n1.octave*14)
  // For thumb-index (1-2), MinPrac=-8, need distance <= -11

  int dist = (3 + 4*14) - (0 + 4*14);  // 3 - 0 = 3 (positive, wrong direction)

  // Correct setup: n2 lower than n1
  Note n1(Pitch(11), 4, 480, false, 1, 1);  // B4
  Note n2(Pitch(0), 4, 480, false, 1, 1);   // C4
  // distance = (0 + 4*14) - (11 + 4*14) = -11

  double penalty = eval.evaluate_distance_with_cascading(n1, Finger::THUMB, n2, Finger::INDEX);

  // 3 units below MinPrac(-8) → violation = 3
  // Cascading: (1 + 2 + 10) * 3 = 39
  EXPECT_EQ(penalty, 39);
}
```

### Integration Test: Python Scorer Parity

```cpp
TEST(EvaluatorIntegrationTest, MatchPythonScorerOnGoldenSet) {
  Config cfg = Config::load_preset("Medium");
  ScoreEvaluator eval(cfg);

  // Load baseline scores from tests/baseline/baseline_scores.json
  auto baseline = load_baseline_scores("tests/baseline/baseline_scores.json");

  for (const auto& [filename, expected_score] : baseline) {
    auto parse_result = MusicXMLParser::parse("tests/baseline/" + filename);
    Fingering fingering = /* load from Python output or run optimizer */;

    double computed_score = eval.evaluate(parse_result.piece, fingering, Hand::RIGHT);

    // Allow 0.1% tolerance for floating-point rounding
    EXPECT_NEAR(computed_score, expected_score, expected_score * 0.001);
  }
}
```

---

## Design Constraints

1. **Logical const-correctness**: Evaluator is logically const; internal cache is mutable for performance
2. **NOT thread-safe**: Mutable cache requires separate evaluator instance per thread
   - **Parallel usage pattern**: Create one `ScoreEvaluator` per thread in ILS trajectories
   - Call `evaluate()` once to warm cache, then `evaluate_delta()` achieves O(1) cache hits
   - Memory cost: ~50KB per thread for 2000-note piece
3. **Performance-critical**: Profile-guided optimization needed (hot path in ILS)
4. **Cascading rules MUST accumulate**: Rules 1, 2, 13 are additive, not max
5. **Cache invalidation**: evaluate() must be called before evaluate_delta() for correct cache state
6. **Move-only**: Uses PIMPL pattern with unique_ptr, not copyable

---

## File Structure

```
include/evaluator/
  score_evaluator.h        // Public API with SliceLocation and PIMPL
  rules.h                  // Individual rule functions (free functions)
src/evaluator/
  score_evaluator.cpp      // Orchestration, caching, delta evaluation
  rules.cpp                // Rule implementations (15 functions + cascading helpers)
```

**Note:** Cascading logic is inline in rules.cpp, not separate file

---

## Critical Implementation Details

### Cascading Distance Evaluation (Rules 1, 2, 13)

```cpp
double ScoreEvaluator::evaluate_distance_with_cascading(
  const Note& n1, Finger f1,
  const Note& n2, Finger f2
) const {
  int distance = compute_distance(n1, n2);
  FingerPair pair(f1, f2);

  const auto& dm = (/* hand == RIGHT */) ? config_.right_hand : config_.left_hand;
  const auto& thresholds = dm.get_pair(pair);

  double penalty = 0.0;

  // Check from innermost to outermost threshold
  if (distance < thresholds.MinRel || distance > thresholds.MaxRel) {
    int violation_units = std::max(
      thresholds.MinRel - distance,
      distance - thresholds.MaxRel
    );
    penalty += violation_units * config_.weights[1];  // Rule 2: +1/unit

    if (distance < thresholds.MinComf || distance > thresholds.MaxComf) {
      int comfort_violation = std::max(
        thresholds.MinComf - distance,
        distance - thresholds.MaxComf
      );
      penalty += comfort_violation * config_.weights[0];  // Rule 1: +2/unit additional

      if (distance < thresholds.MinPrac || distance > thresholds.MaxPrac) {
        int prac_violation = std::max(
          thresholds.MinPrac - distance,
          distance - thresholds.MaxPrac
        );
        penalty += prac_violation * config_.weights[12];  // Rule 13: +10/unit additional
      }
    }
  }

  return penalty;
}
```

### Delta Evaluation for ILS

```cpp
double ScoreEvaluator::evaluate_delta(
  const Piece& piece,
  const std::vector<Fingering>& current_fingerings,
  const std::vector<Fingering>& proposed_fingerings,
  const SliceLocation& changed_location,
  Hand hand
) const {
  // Reuse cached old_notes from prior evaluate() call
  // Build new_notes vector for proposed fingerings
  // Only recompute rules affected by the changed note at changed_location

  // Rules affected:
  // - Inter-slice (1,2,5-13,15): Previous and next slices
  // - Intra-slice (14): Current slice if it's a chord
  // - Triplet (3,4,12): If part of a triplet

  double delta = 0.0;

  // Use SliceLocation to directly address changed note (O(1) access)
  // Remove old penalties involving changed note
  delta -= compute_local_penalties(old_notes, changed_location);

  // Add new penalties involving changed note
  delta += compute_local_penalties(new_notes, changed_location);

  return delta;
}
```

**Complexity:**
- O(S) per call where S = total slices (for building note vectors)
- O(S) amortized when preceded by evaluate() call (reuses cached old_notes)
- Effectively O(1) per-note work for local penalty computation
- 10-100x faster than full O(N) re-evaluation for typical pieces

---

## Dependencies

- **Domain**: Read-only access to `Piece`, `Note`, `Slice`, `Finger`
- **Config**: Read-only access to `DistanceMatrix`, `RuleWeights`

---

## Performance Optimizations

1. **Precompute black key lookup table**: Avoid modulo in hot loop
2. **Inline small rule functions**: Enable compiler optimization
3. **Cache distance matrix access**: Store references, not repeated lookups
4. **SIMD for batch evaluation**: Evaluate multiple candidates in parallel (future)

---

## Future Extensibility

- **User-defined rules**: Plugin system for custom penalty functions
- **Rule weights per hand**: Different penalties for left vs. right
- **Context-aware penalties**: Tempo, dynamics affect difficulty scoring
