# Module: Optimizer

## Responsibilities

1. **Beam Search Dynamic Programming (Phase 1)**: Generate high-quality initial fingering
2. **Iterated Local Search (Phase 2)**: Improve solution via local search + perturbation
3. **Thread Pool Management**: Parallelize ILS trajectories across CPU cores
4. **Progress Reporting**: Notify observer of measure completion and phase transitions
5. **Deterministic Execution**: Seeded RNG for reproducible results

---

## Data Ownership

### Beam Search State

| Data | Type | Description |
|------|------|-------------|
| `BeamState` | `struct {Fingering partial, double cost, size_t prev_state_idx}` | Partial solution at slice i |
| `beam_` | `std::vector<std::vector<BeamState>>` | beam_[slice_idx] = top-K states |
| `beam_width_` | `size_t` | Max states kept per slice (default 100) |

### ILS State

| Data | Type | Description |
|------|------|-------------|
| `ILSTrajectory` | `struct {Fingering current, Fingering best, double best_cost}` | Single trajectory state |
| `trajectories_` | `std::vector<ILSTrajectory>` | Parallel trajectories (one per thread) |
| `perturbation_strength_` | `size_t` | Notes modified per perturbation (default 3) |

### Thread Pool

| Data | Type | Description |
|------|------|-------------|
| `workers_` | `std::vector<std::thread>` | Worker threads |
| `task_queue_` | `std::queue<std::function<void()>>` | Pending tasks |
| `mutex_` | `std::mutex` | Protects task queue |
| `cv_` | `std::condition_variable` | Wakes idle workers |

---

## Communication

### Inbound API

```cpp
class Optimizer {
public:
  struct Result {
    Fingering fingering;
    double score;
    size_t iterations_performed;
  };

  explicit Optimizer(const Config& cfg, IProgressObserver* observer = nullptr);

  // Optimize a single hand
  Result optimize(const Piece& piece, Hand hand, unsigned int seed);

private:
  // Phase 1: Beam Search DP
  Fingering beam_search(const Piece& piece, Hand hand);

  // Phase 2: Iterated Local Search
  void ils_improve(Fingering& fingering, const Piece& piece, Hand hand, size_t iterations);

  // Helpers
  std::vector<Fingering> generate_valid_states(const Slice& slice) const;
  void prune_beam(std::vector<BeamState>& beam) const;
  Fingering perturb(const Fingering& f, size_t strength, std::mt19937& rng) const;

  Config config_;
  IProgressObserver* observer_;
  ScoreEvaluator evaluator_;
  ThreadPool thread_pool_;
  std::mt19937 rng_;
};
```

### Outbound Dependencies

- **Domain**: Reads `Piece`, constructs `Fingering`
- **Config**: Reads beam width, ILS iterations, perturbation strength
- **Evaluator**: Calls `evaluate()` and `evaluate_delta()`
- **Observer**: Reports progress via `on_measure_complete()`, `on_phase_change()`

---

## Coupling Analysis

### Afferent Coupling

- **CLI/Orchestrator**: Calls `optimize()` for left and right hands

### Efferent Coupling

- **Domain** (constructs solutions)
- **Config** (algorithm parameters)
- **Evaluator** (score computation)
- **Observer** (progress reporting)

**Instability:** Medium (core algorithm, but depends on multiple modules)

---

## Testing Strategy

### Unit Tests

#### 1. Beam Pruning

```cpp
TEST(OptimizerTest, BeamPruningKeepsTopK) {
  Config cfg = Config::load_preset("Medium");
  cfg.beam_width = 5;
  Optimizer opt(cfg);

  std::vector<BeamState> beam;
  for (int i = 0; i < 10; ++i) {
    beam.push_back({/* fingering */, static_cast<double>(i), 0});  // Costs 0-9
  }

  opt.prune_beam(beam);

  EXPECT_EQ(beam.size(), 5);  // Kept only top 5
  EXPECT_EQ(beam[0].cost, 0);  // Best cost first
  EXPECT_EQ(beam[4].cost, 4);  // Fifth-best cost last
}
```

#### 2. Valid State Generation (Hard Constraint)

```cpp
TEST(OptimizerTest, ValidStatesNoDuplicateFingers) {
  Config cfg = Config::load_preset("Medium");
  Optimizer opt(cfg);

  // Chord with 3 simultaneous notes
  Slice chord({
    Note(Pitch(0), 4, 480, false, 1, 1),
    Note(Pitch(4), 4, 480, false, 1, 1),
    Note(Pitch(7), 4, 480, false, 1, 1)
  });

  auto valid_states = opt.generate_valid_states(chord);

  for (const auto& fingering : valid_states) {
    std::set<Finger> used_fingers;
    for (size_t i = 0; i < fingering.size(); ++i) {
      auto f = fingering.get(i);
      ASSERT_TRUE(f.has_value());
      EXPECT_TRUE(used_fingers.insert(*f).second);  // No duplicates
    }
  }
}
```

#### 3. Perturbation Operator

```cpp
TEST(OptimizerTest, PerturbationModifiesExactlyNNotes) {
  Config cfg = Config::load_preset("Medium");
  cfg.perturbation_strength = 3;
  Optimizer opt(cfg);

  Fingering original(10);  // 10-note fingering
  for (size_t i = 0; i < 10; ++i) {
    original.assign(i, Finger::THUMB);
  }

  std::mt19937 rng(12345);
  Fingering perturbed = opt.perturb(original, 3, rng);

  size_t differences = 0;
  for (size_t i = 0; i < 10; ++i) {
    if (original.get(i) != perturbed.get(i)) {
      ++differences;
    }
  }

  EXPECT_EQ(differences, 3);
}
```

#### 4. Determinism with Fixed Seed

```cpp
TEST(OptimizerTest, SameSeedProducesSameResult) {
  Config cfg = Config::load_preset("Medium");
  Optimizer opt1(cfg), opt2(cfg);

  auto piece = /* load test piece */;

  auto result1 = opt1.optimize(piece, Hand::RIGHT, /* seed */ 12345);
  auto result2 = opt2.optimize(piece, Hand::RIGHT, /* seed */ 12345);

  EXPECT_EQ(result1.score, result2.score);
  EXPECT_EQ(result1.fingering, result2.fingering);  // Exact match
}
```

### Integration Tests

#### 1. Golden Set Z-Score Regression

```cpp
TEST(OptimizerIntegrationTest, GoldenSetScoresWithinTolerance) {
  Config cfg = Config::load_preset("Medium");
  Optimizer opt(cfg);

  auto baseline = load_baseline_scores("tests/baseline/baseline_scores.json");

  for (const auto& [filename, expected_score] : baseline) {
    auto piece = MusicXMLParser::parse("tests/baseline/" + filename).piece;

    auto result = opt.optimize(piece, Hand::RIGHT, /* seed */ 0);

    // Allow 5% deviation per SRS CORRECT-1.2
    double tolerance = expected_score * 0.05;
    EXPECT_NEAR(result.score, expected_score, tolerance)
      << "Failed for " << filename;
  }
}
```

#### 2. Performance Benchmarks

```cpp
TEST(OptimizerIntegrationTest, ProcessingTime500Notes) {
  Config cfg = Config::load_preset("Medium");
  cfg.mode = OptimizationMode::BALANCED;
  Optimizer opt(cfg);

  auto piece = /* generate or load 500-note piece */;

  auto start = std::chrono::high_resolution_clock::now();
  auto result = opt.optimize(piece, Hand::RIGHT, 0);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

  EXPECT_LT(duration.count(), 2);  // SRS PERF-1.1: <2 seconds
}
```

---

## Design Constraints

1. **Memory Budget**: Beam width limited to prevent exceeding 512MB (SRS PERF-3.1)
2. **Thread Safety**: ILS trajectories must not share mutable state
3. **Determinism**: RNG must be seeded consistently across platforms
4. **No Blocking I/O**: Progress updates must be async or non-blocking

---

## File Structure

```
include/optimizer/
  optimizer.h              // Public API
  beam_search.h            // Phase 1 algorithm
  ils.h                    // Phase 2 algorithm
  thread_pool.h            // Worker queue
  state_generation.h       // Valid fingering generation
src/optimizer/
  optimizer.cpp            // Orchestration (beam + ILS)
  beam_search.cpp
  ils.cpp
  thread_pool.cpp
  state_generation.cpp
```

---

## Critical Implementation Details

### Beam Search DP (Phase 1)

```cpp
Fingering Optimizer::beam_search(const Piece& piece, Hand hand) {
  const auto& measures = (hand == Hand::RIGHT) ? piece.right_hand() : piece.left_hand();

  // Flatten measures into slices
  std::vector<Slice> slices;
  for (const auto& measure : measures) {
    slices.insert(slices.end(), measure.slices().begin(), measure.slices().end());
  }

  size_t N = slices.size();

  // beam_[i] = top-K states at slice i
  std::vector<std::vector<BeamState>> beam(N);

  // Initialize first slice
  for (const auto& fingering : generate_valid_states(slices[0])) {
    double cost = evaluator_.evaluate_intra_slice(slices[0], fingering);
    beam[0].push_back({fingering, cost, /* prev_idx */ 0});
  }
  prune_beam(beam[0]);

  // Forward pass
  for (size_t i = 1; i < N; ++i) {
    for (size_t prev_idx = 0; prev_idx < beam[i-1].size(); ++prev_idx) {
      const auto& prev_state = beam[i-1][prev_idx];

      for (const auto& curr_fingering : generate_valid_states(slices[i])) {
        double transition_cost = evaluator_.evaluate_transition(
          slices[i-1], prev_state.partial,
          slices[i], curr_fingering
        );
        double intra_cost = evaluator_.evaluate_intra_slice(slices[i], curr_fingering);

        double total_cost = prev_state.cost + transition_cost + intra_cost;

        beam[i].push_back({curr_fingering, total_cost, prev_idx});
      }
    }

    prune_beam(beam[i]);

    // Progress update
    if (observer_) {
      observer_->on_measure_complete(i + 1, N);
    }
  }

  // Backtrack to reconstruct best path
  return backtrack(beam, N);
}
```

### Iterated Local Search (Phase 2)

```cpp
void Optimizer::ils_improve(Fingering& fingering, const Piece& piece, Hand hand, size_t iterations) {
  double best_cost = evaluator_.evaluate(piece, fingering, hand);
  Fingering best_solution = fingering;

  for (size_t iter = 0; iter < iterations; ++iter) {
    // Local search (first-improvement)
    bool improved = true;
    while (improved) {
      improved = false;

      for (size_t note_idx = 0; note_idx < fingering.size(); ++note_idx) {
        for (Finger new_finger : {Finger::THUMB, Finger::INDEX, /* ... */}) {
          if (new_finger == fingering.get(note_idx)) continue;

          Fingering candidate = fingering;
          candidate.assign(note_idx, new_finger);

          // Check hard constraint
          if (evaluator_.violates_hard_constraint(/* slice */, candidate)) {
            continue;
          }

          // Delta evaluation using SliceLocation
          ScoreEvaluator::SliceLocation location = compute_location(note_idx);  // Helper to compute location
          double delta = evaluator_.evaluate_delta(piece, fingering, candidate, location, hand);
          double new_cost = best_cost + delta;

          if (new_cost < best_cost) {
            fingering = candidate;
            best_cost = new_cost;
            improved = true;
            break;
          }
        }
        if (improved) break;
      }
    }

    // Update global best
    if (best_cost < evaluator_.evaluate(piece, best_solution, hand)) {
      best_solution = fingering;
    }

    // Perturbation
    fingering = perturb(best_solution, config_.perturbation_strength, rng_);
  }

  fingering = best_solution;
}
```

### Thread Pool for Parallel ILS

```cpp
class ThreadPool {
public:
  explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency())
    : stop_(false) {
    for (size_t i = 0; i < num_threads; ++i) {
      workers_.emplace_back([this] {
        while (true) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty()) return;

            task = std::move(tasks_.front());
            tasks_.pop();
          }
          task();
        }
      });
    }
  }

  template<class F>
  auto enqueue(F&& f) -> std::future<std::invoke_result_t<F>> {
    using return_type = std::invoke_result_t<F>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
    std::future<return_type> result = task->get_future();

    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (stop_) throw std::runtime_error("enqueue on stopped ThreadPool");
      tasks_.emplace([task]() { (*task)(); });
    }

    cv_.notify_one();
    return result;
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      stop_ = true;
    }
    cv_.notify_all();
    for (std::thread& worker : workers_) {
      worker.join();
    }
  }

private:
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool stop_;
};
```

### Parallel ILS Trajectories

```cpp
Result Optimizer::optimize(const Piece& piece, Hand hand, unsigned int seed) {
  rng_.seed(seed);

  // Phase 1: Beam Search
  if (observer_) observer_->on_phase_change(Phase::BEAM_SEARCH);
  Fingering initial = beam_search(piece, hand);

  // Phase 2: Parallel ILS
  if (observer_) observer_->on_phase_change(Phase::ILS);

  size_t num_trajectories = thread_pool_.size();
  std::vector<std::future<Fingering>> futures;

  for (size_t i = 0; i < num_trajectories; ++i) {
    futures.push_back(thread_pool_.enqueue([this, initial, &piece, hand, i]() {
      Fingering trajectory = initial;
      std::mt19937 local_rng(rng_() + i);  // Seeded uniquely per trajectory

      ils_improve(trajectory, piece, hand, config_.ils_iterations);
      return trajectory;
    }));
  }

  // Collect results
  Fingering best = initial;
  double best_cost = evaluator_.evaluate(piece, best, hand);

  for (auto& future : futures) {
    Fingering candidate = future.get();
    double cost = evaluator_.evaluate(piece, candidate, hand);
    if (cost < best_cost) {
      best = candidate;
      best_cost = cost;
    }
  }

  if (observer_) observer_->on_optimization_complete(best_cost);

  return {best, best_cost, config_.ils_iterations * num_trajectories};
}
```

---

## Dependencies

- **Domain**: Constructs `Fingering`, reads `Piece`
- **Config**: Algorithm parameters (beam width, ILS iterations)
- **Evaluator**: Score computation
- **Observer**: Progress reporting (optional)
- **C++ stdlib**: `<thread>`, `<mutex>`, `<condition_variable>`, `<random>`

---

## Performance Optimizations

1. **Parallel ILS trajectories**: Linear speedup with cores
2. **Delta evaluation in ILS**: O(S) amortized instead of O(N) full re-evaluation per swap
3. **Beam pruning with partial_sort**: O(K log K) instead of full sort
4. **Precomputed state generation**: Cache valid fingerings per chord size
5. **Evaluator caching**: Each trajectory maintains evaluator cache for efficient deltas

---

## Future Extensibility

- **GPU acceleration**: Beam search state evaluation on CUDA
- **Adaptive beam width**: Increase width for complex passages
- **Tabu search**: Avoid revisiting recent solutions in ILS
