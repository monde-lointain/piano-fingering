# Module Specification: Engine

## Responsibilities

- Implement Beam Search Dynamic Programming (constructive phase)
- Implement Iterated Local Search (improvement phase)
- Evaluate fingering difficulty using 15 rules
- Enforce hard constraint (no duplicate fingers in chords)
- Support 3 optimization modes (fast, balanced, quality)
- Provide deterministic results with fixed seed
- Report progress via injected `ProgressReporter` interface

---

## Data Ownership

### Primary Data Structures

```cpp
// Fingering assignment for a single slice
struct State {
    std::vector<Finger> fingers;  // One finger per note in slice (size 1-5)

    // Validate hard constraint: no duplicate fingers
    bool IsValid() const;
};

// Beam search entry (DP state)
struct BeamEntry {
    State state;
    float cumulative_cost;
    size_t prev_index;  // Backpointer for solution reconstruction
};

// Complete fingering solution for one hand
class Solution {
public:
    void SetFingering(size_t slice_index, const State& state);
    Finger GetFingering(size_t note_index) const;
    float GetTotalCost() const { return total_cost_; }

private:
    std::vector<State> slice_fingerings_;  // One State per slice
    float total_cost_;
};

// Optimization options
enum class OptimizationMode {
    Fast,      // Greedy constructive only (no ILS)
    Balanced,  // Beam DP + 1000 ILS iterations
    Quality    // Beam DP + 5000 ILS iterations
};
```

---

## Communication

### Incoming Dependencies (Afferent)

| Caller | Input |
|--------|-------|
| `cli` | `Optimize(piece, config, mode, seed, reporter)` |

### Outgoing Dependencies (Efferent)

| Module | Read Access |
|--------|-------------|
| `musicxml` | `Piece` (read-only) |
| `config` | `Config` (read-only) |

**Note**: Engine never modifies `Piece` or `Config` (const correctness).

---

## Coupling

### Afferent Coupling (Incoming)
- `cli` module

**Count**: 1 (low)

### Efferent Coupling (Outgoing)
- `musicxml` module (read `Piece`)
- `config` module (read `Config`)
- `common/types.h` (`Hand`, `Finger`)

**Count**: 2 (low)

---

## API Specification

```cpp
// In src/engine/optimizer.h
namespace engine {

// Progress reporting interface (injected by CLI)
class ProgressReporter {
public:
    virtual ~ProgressReporter() = default;
    virtual void ReportProgress(int current, int total) = 0;
};

// Main optimization entry point
// Returns: Solution with finger assignments for all notes
Solution Optimize(const Piece& piece,
                  const Config& config,
                  OptimizationMode mode,
                  uint32_t seed,
                  ProgressReporter* reporter = nullptr);

}  // namespace engine
```

---

## Sub-Module: Cost Evaluation

### Responsibilities

- Implement 15 fingering rules (Table 2 in Problem Description)
- Compute inter-slice transition cost
- Compute intra-slice (chord) cost
- Compute triplet cost (Rules 3, 4, 12)

### API

```cpp
// In src/engine/cost/evaluator.h
namespace engine::cost {

class CostEvaluator {
public:
    explicit CostEvaluator(const Config& config) : config_(config) {}

    // Compute cost of transition between two consecutive slices
    float ComputeInterSliceCost(const State& prev, const State& curr,
                                const Slice& prev_slice, const Slice& curr_slice) const;

    // Compute cost within a single chord (Rule 14)
    float ComputeIntraSliceCost(const State& state, const Slice& slice) const;

    // Compute triplet cost (Rules 3, 4, 12)
    float ComputeTripletCost(const State& s1, const State& s2, const State& s3,
                             const Slice& slice1, const Slice& slice2, const Slice& slice3) const;

private:
    const Config& config_;

    // Individual rule implementations
    float ApplyRule1(int distance, Finger f1, Finger f2) const;  // Below MinComf or above MaxComf
    float ApplyRule2(int distance, Finger f1, Finger f2) const;  // Below MinRel or above MaxRel
    // ... (15 rules total)
    float ApplyRule15(Finger f_prev, Finger f_curr, const Note& n_prev, const Note& n_curr) const;
};

}  // namespace engine::cost
```

### Rule Implementation Examples

**Rule 1**: Distance below MinComf or above MaxComf (+2 per unit)

```cpp
float CostEvaluator::ApplyRule1(int distance, Finger f1, Finger f2) const {
    const auto& bounds = config_.GetDistanceMatrix().Get(f1, f2);

    float penalty = 0.0f;
    if (distance < bounds.MinComf) {
        penalty += (bounds.MinComf - distance) * 2.0f;
    } else if (distance > bounds.MaxComf) {
        penalty += (distance - bounds.MaxComf) * 2.0f;
    }

    return penalty * config_.GetRuleWeights().weights[0];  // Rule 1 weight
}
```

**Rule 13**: Distance below MinPrac or above MaxPrac (+10 per unit)

```cpp
float CostEvaluator::ApplyRule13(int distance, Finger f1, Finger f2) const {
    const auto& bounds = config_.GetDistanceMatrix().Get(f1, f2);

    float penalty = 0.0f;
    if (distance < bounds.MinPrac) {
        penalty += (bounds.MinPrac - distance) * 10.0f;
    } else if (distance > bounds.MaxPrac) {
        penalty += (distance - bounds.MaxPrac) * 10.0f;
    }

    return penalty * config_.GetRuleWeights().weights[12];  // Rule 13 weight
}
```

**Rule 14**: Apply rules 1, 2, 13 within chord (doubled scores)

```cpp
float CostEvaluator::ComputeIntraSliceCost(const State& state, const Slice& slice) const {
    float total_cost = 0.0f;
    const auto& notes = slice.GetNotes();

    // For each pair of notes in the chord
    for (size_t i = 0; i < notes.size(); ++i) {
        for (size_t j = i + 1; j < notes.size(); ++j) {
            int distance = notes[j].AbsolutePosition() - notes[i].AbsolutePosition();
            Finger f1 = state.fingers[i];
            Finger f2 = state.fingers[j];

            // Apply rules with doubled penalties
            total_cost += 2.0f * ApplyRule1(distance, f1, f2);
            total_cost += 2.0f * ApplyRule2(distance, f1, f2);
            total_cost += ApplyRule13(distance, f1, f2);  // Not doubled
        }
    }

    return total_cost;
}
```

### Testing Strategy (Cost Module)

| Test Case | Scenario | Expected Outcome |
|-----------|----------|------------------|
| `Rule1_BelowMinComf` | Distance = -10, MinComf = -6 | Penalty = (6 - (-10)) * 2 = 32 |
| `Rule1_AboveMaxComf` | Distance = 12, MaxComf = 8 | Penalty = (12 - 8) * 2 = 8 |
| `Rule13_Violation` | Distance = -12, MinPrac = -8 | Penalty = ((-8) - (-12)) * 10 = 40 |
| `Rule14_ChordCost` | Chord with 2 notes, distance outside comfort | Cost = 2 × (Rule1 + Rule2) + Rule13 |
| `TripletCost` | Three consecutive slices | Rules 3, 4, 12 applied |

---

## Sub-Module: Beam Search DP

### Responsibilities

- Construct initial solution using dynamic programming
- Maintain beam of top-K states at each slice
- Prune low-cost states to beam width (100)
- Backtrack to reconstruct best solution

### Algorithm

```cpp
Solution BeamSearchDP(const std::vector<Slice>& slices,
                      const Config& config,
                      const CostEvaluator& evaluator) {
    constexpr size_t BEAM_WIDTH = 100;
    const size_t N = slices.size();

    // dp[i] = top-K states at slice i
    std::vector<std::vector<BeamEntry>> dp(N);

    // Initialize: generate all valid states for slice 0
    dp[0] = GenerateInitialStates(slices[0]);

    // Forward pass: DP with beam search
    for (size_t i = 1; i < N; ++i) {
        std::vector<BeamEntry> candidates;
        candidates.reserve(dp[i - 1].size() * GetStateCount(slices[i]));

        for (const auto& prev_entry : dp[i - 1]) {
            for (const auto& curr_state : GenerateValidStates(slices[i])) {
                float trans_cost = evaluator.ComputeInterSliceCost(
                    prev_entry.state, curr_state, slices[i - 1], slices[i]);

                float intra_cost = evaluator.ComputeIntraSliceCost(curr_state, slices[i]);

                float triplet_cost = 0.0f;
                if (i >= 2) {
                    const auto& prevprev_state = FindState(dp[i - 2], prev_entry.prev_index);
                    triplet_cost = evaluator.ComputeTripletCost(
                        prevprev_state, prev_entry.state, curr_state,
                        slices[i - 2], slices[i - 1], slices[i]);
                }

                BeamEntry new_entry{
                    .state = curr_state,
                    .cumulative_cost = prev_entry.cumulative_cost + trans_cost + intra_cost + triplet_cost,
                    .prev_index = GetEntryIndex(dp[i - 1], prev_entry)
                };

                candidates.push_back(new_entry);
            }
        }

        // Prune to beam width
        std::partial_sort(candidates.begin(),
                          candidates.begin() + std::min(BEAM_WIDTH, candidates.size()),
                          candidates.end(),
                          [](const auto& a, const auto& b) { return a.cumulative_cost < b.cumulative_cost; });

        dp[i].assign(candidates.begin(),
                     candidates.begin() + std::min(BEAM_WIDTH, candidates.size()));
    }

    // Backtrack to reconstruct solution
    return Backtrack(dp, N);
}
```

### State Generation

```cpp
// Generate all valid finger assignments for a slice
std::vector<State> GenerateValidStates(const Slice& slice) {
    const size_t M = slice.Size();
    if (M > 5) {
        throw std::runtime_error("Chord size exceeds 5 notes");
    }

    std::vector<State> states;

    // Enumerate all M-permutations of {1, 2, 3, 4, 5}
    // Example for M=2: {1,2}, {1,3}, {1,4}, {1,5}, {2,1}, {2,3}, ..., {5,4}
    std::vector<Finger> all_fingers = {Finger::Thumb, Finger::Index, Finger::Middle, Finger::Ring, Finger::Pinky};

    // Use std::next_permutation or recursive enumeration
    EnumeratePermutations(all_fingers, M, [&](const std::vector<Finger>& perm) {
        State state;
        state.fingers = perm;
        if (state.IsValid()) {  // Check hard constraint
            states.push_back(state);
        }
    });

    return states;
}

// Hard constraint: no duplicate fingers
bool State::IsValid() const {
    std::set<Finger> used;
    for (Finger f : fingers) {
        if (used.count(f)) {
            return false;  // Duplicate found
        }
        used.insert(f);
    }
    return true;
}
```

### Complexity

| Component | Complexity |
|-----------|------------|
| State generation | O(5! / (5-M)!) ≈ O(120) for M=5 |
| DP forward pass | O(N × B² × M²) |
| Backtrack | O(N) |
| **Total** | O(N × B² × M²) = O(N × 100² × 5²) ≈ O(250,000 × N) |

For N=1000 slices: ~250 million operations (highly tractable).

### Testing Strategy (DP Module)

| Test Case | Scenario | Expected Outcome |
|-----------|----------|------------------|
| `BeamPruning` | 200 candidate states → prune to 100 | Top 100 by cost retained |
| `Backtrack` | DP complete, backtrack from best state | Solution reconstructed correctly |
| `HardConstraintEnforced` | Chord with 5 notes | No duplicate fingers in any state |
| `MonophonicPiece` | 1 note per slice | Exact DP (no pruning needed) |
| `CMajorScale` | Known optimal fingering | Matches expected Z-score |

---

## Sub-Module: Iterated Local Search (ILS)

### Responsibilities

- Improve solution via local search
- Perturb solution to escape local optima
- Run multiple independent trajectories in parallel
- Return best solution across all trajectories

### Algorithm

```cpp
Solution ILS(const Solution& initial_solution,
             const std::vector<Slice>& slices,
             const Config& config,
             const CostEvaluator& evaluator,
             size_t max_iterations) {
    Solution best_solution = initial_solution;
    float best_cost = initial_solution.GetTotalCost();

    Solution current_solution = initial_solution;

    for (size_t iter = 0; iter < max_iterations; ++iter) {
        // Local search (first-improvement)
        bool improved = true;
        while (improved) {
            improved = false;

            // Neighborhood 1: Single finger swap
            for (size_t i = 0; i < slices.size(); ++i) {
                for (size_t j = 0; j < slices[i].Size(); ++j) {
                    for (Finger new_finger : {Finger::Thumb, Finger::Index, Finger::Middle, Finger::Ring, Finger::Pinky}) {
                        Solution neighbor = current_solution;
                        neighbor.slice_fingerings_[i].fingers[j] = new_finger;

                        if (!neighbor.slice_fingerings_[i].IsValid()) {
                            continue;  // Violates hard constraint
                        }

                        float new_cost = EvaluateSolution(neighbor, slices, evaluator);
                        if (new_cost < current_solution.GetTotalCost()) {
                            current_solution = neighbor;
                            improved = true;
                            break;  // First improvement
                        }
                    }
                    if (improved) break;
                }
                if (improved) break;
            }
        }

        // Update best
        if (current_solution.GetTotalCost() < best_cost) {
            best_solution = current_solution;
            best_cost = current_solution.GetTotalCost();
        }

        // Perturbation (escape local optimum)
        current_solution = Perturb(best_solution, 3);  // Randomly change 3 fingers
    }

    return best_solution;
}
```

### Perturbation

```cpp
Solution Perturb(const Solution& solution, size_t strength, std::mt19937& rng) {
    Solution perturbed = solution;

    std::uniform_int_distribution<size_t> slice_dist(0, solution.slice_fingerings_.size() - 1);
    std::uniform_int_distribution<int> finger_dist(1, 5);

    for (size_t k = 0; k < strength; ++k) {
        size_t slice_idx = slice_dist(rng);
        size_t note_idx = slice_dist(rng) % perturbed.slice_fingerings_[slice_idx].fingers.size();

        Finger new_finger = static_cast<Finger>(finger_dist(rng));
        perturbed.slice_fingerings_[slice_idx].fingers[note_idx] = new_finger;

        // Retry if hard constraint violated
        while (!perturbed.slice_fingerings_[slice_idx].IsValid()) {
            new_finger = static_cast<Finger>(finger_dist(rng));
            perturbed.slice_fingerings_[slice_idx].fingers[note_idx] = new_finger;
        }
    }

    return perturbed;
}
```

### Parallelization

```cpp
Solution ParallelILS(const Solution& initial_solution,
                     const std::vector<Slice>& slices,
                     const Config& config,
                     const CostEvaluator& evaluator,
                     size_t total_iterations,
                     uint32_t seed) {
    unsigned int num_threads = std::min(
        std::thread::hardware_concurrency(),
        static_cast<unsigned int>(total_iterations)
    );

    size_t iterations_per_thread = total_iterations / num_threads;

    std::vector<std::future<Solution>> futures;
    for (unsigned int t = 0; t < num_threads; ++t) {
        futures.push_back(std::async(std::launch::async, [&, t, seed]() {
            std::mt19937 rng(seed + t);  // Different seed per thread
            return ILS(initial_solution, slices, config, evaluator, iterations_per_thread);
        }));
    }

    // Collect results and return best
    Solution best_solution = initial_solution;
    float best_cost = initial_solution.GetTotalCost();

    for (auto& future : futures) {
        Solution solution = future.get();
        if (solution.GetTotalCost() < best_cost) {
            best_solution = solution;
            best_cost = solution.GetTotalCost();
        }
    }

    return best_solution;
}
```

### Testing Strategy (ILS Module)

| Test Case | Scenario | Expected Outcome |
|-----------|----------|------------------|
| `LocalSearchImproves` | Suboptimal initial solution | Cost decreases after local search |
| `PerturbationEscapes` | Stuck in local optimum | Perturbation enables further improvement |
| `Determinism` | Same seed, same input | Identical solution across runs |
| `ParallelEquivalence` | 1 thread vs 4 threads | 4 threads finds equal or better solution |
| `HardConstraintMaintained` | After perturbation | No duplicate fingers |

---

## Orchestration

```cpp
Solution Optimize(const Piece& piece,
                  const Config& config,
                  OptimizationMode mode,
                  uint32_t seed,
                  ProgressReporter* reporter) {
    // Extract slices for each hand separately
    auto [left_slices, right_slices] = SplitByHand(piece);

    // Create cost evaluator
    CostEvaluator evaluator(config);

    // Parallel optimization (2× parallelism: left + right hand)
    auto optimize_hand = [&](const std::vector<Slice>& slices, uint32_t hand_seed) -> Solution {
        // Phase 1: Beam Search DP
        Solution initial = BeamSearchDP(slices, config, evaluator);

        if (mode == OptimizationMode::Fast) {
            return initial;  // No ILS
        }

        // Phase 2: ILS
        size_t ils_iterations = (mode == OptimizationMode::Balanced) ? 1000 : 5000;
        return ParallelILS(initial, slices, config, evaluator, ils_iterations, hand_seed);
    };

    auto left_future = std::async(std::launch::async, optimize_hand, left_slices, seed);
    auto right_future = std::async(std::launch::async, optimize_hand, right_slices, seed + 1);

    Solution left_solution = left_future.get();
    Solution right_solution = right_future.get();

    // Merge solutions (interleave left and right hand fingerings)
    return MergeSolutions(left_solution, right_solution, piece);
}
```

---

## Testing Strategy (Integration)

| Test Case | Scenario | Expected Outcome |
|-----------|----------|------------------|
| `CMajorScale_FastMode` | Greedy DP only | Valid solution, <0.5s |
| `CMajorScale_BalancedMode` | DP + 1000 ILS | Better Z-score than Fast, <2s |
| `CMajorScale_QualityMode` | DP + 5000 ILS | Best Z-score, <5s |
| `GoldenSet_AllModes` | All 10 pieces × 3 modes | 100% success, no crashes |
| `HardConstraint_NeverViolated` | 1000 random pieces | 0 duplicate-finger violations |
| `Determinism` | Seed=12345, 10 runs | Identical Z-scores |
| `ProgressReporting` | 100-measure piece | Progress updates ≤1s apart |

---

## File Structure

```
src/engine/
├── optimizer.h        // Public API (Optimize function)
├── optimizer.cpp      // Orchestration (Beam DP + ILS)
├── cost/
│   ├── evaluator.h    // CostEvaluator class
│   ├── evaluator.cpp
│   ├── rule_01.cpp    // Rule 1 implementation
│   ├── rule_02.cpp    // Rule 2 implementation
│   └── ...            // Rules 3-15
├── dp/
│   ├── beam_search.h  // Beam DP algorithm
│   ├── beam_search.cpp
│   └── state_gen.cpp  // State generation logic
└── ils/
    ├── local_search.h  // ILS algorithm
    ├── local_search.cpp
    └── perturbation.cpp // Perturbation logic
```

---

## Dependencies

| Dependency | Purpose |
|------------|---------|
| `musicxml` | Read `Piece`, `Slice`, `Note` |
| `config` | Read `Config`, `DistanceMatrix`, `RuleWeights` |
| `common/types.h` | `Hand`, `Finger` enums |
| C++ stdlib | `<vector>`, `<thread>`, `<future>`, `<random>`, `<algorithm>` |

---

## Success Criteria

- ✅ Beam width=100 maintained at each DP step
- ✅ Hard constraint never violated (100% success rate)
- ✅ Deterministic output with fixed seed
- ✅ Parallel ILS scales to `hardware_concurrency()` threads
- ✅ Fast mode <0.5s, Balanced <2s, Quality <5s on baseline hw
- ✅ Golden Set Z-scores within 5% of baseline
- ✅ Memory <512MB for 2000-note piece
