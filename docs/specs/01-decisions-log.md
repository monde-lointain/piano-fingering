# Architectural Decision Records (ADRs)

## ADR-001: Modular Monolith over Plugin Architecture

**Status:** ✅ Accepted

**Date:** 2026-01-01

### Context

The system requires:
- High-performance optimization (<2s for 500 notes)
- Single static binary distribution (SRS 2.4.4)
- Potential for algorithm extensibility (different rule weights, future research variations)

Two options considered:
1. **Modular Monolith**: Compile-time modules with direct function calls
2. **Plugin Architecture**: Runtime-loaded shared libraries for algorithm components

### Decision

Implement as a **modular monolith** with clear compile-time module boundaries.

### Consequences

#### Positive
- ✅ **Zero serialization overhead**: Direct function calls enable sub-microsecond rule evaluations critical for performance targets
- ✅ **Deterministic behavior**: No ABI compatibility issues across platforms
- ✅ **Single binary**: Meets SRS requirement for static linking
- ✅ **Simpler build**: No dynamic library path management

#### Negative
- ❌ **Recompilation required**: Algorithm changes require full rebuild (acceptable - research algorithm is stable)
- ❌ **No hot-swap**: Cannot switch algorithms at runtime (not a requirement)
- ❌ **Tighter coupling**: Modules linked at compile time (mitigated by interface segregation)

---

## ADR-002: pugixml over TinyXML-2

**Status:** ✅ Accepted

**Date:** 2026-01-01

### Context

Need DOM-based XML parser for:
- Reading MusicXML input (FR-1.2)
- Preserving all non-fingering elements (FR-1.4)
- Writing fingered MusicXML output (FR-4.1)

Candidates:
- **TinyXML-2**: Referenced in SRS, zlib license
- **pugixml**: Header-only alternative, MIT license

### Decision

Use **pugixml** configured in header-only mode.

### Consequences

#### Positive
- ✅ **Performance**: Benchmarks show 2-3x faster parsing than TinyXML-2
- ✅ **XPath support**: Simplifies element queries (`doc.select_nodes("//note[@staff='1']")`)
- ✅ **Header-only**: No build complexity, integrates cleanly with static linking

#### Negative
- ❌ **Larger API surface**: More features than needed (mitigated by using minimal subset)
- ❌ **Deviation from SRS reference**: SRS mentions TinyXML-2 (acceptable - "or equivalent" clause)

---

## ADR-003: Fixed-Size Thread Pool over Explicit Thread Spawn

**Status:** ✅ Accepted

**Date:** 2026-01-01

### Context

ILS optimization requires:
- 1000+ iterations (balanced mode) or 5000+ (quality mode)
- Parallel trajectory exploration for speedup (FR-3.10)
- Memory budget constraint (<512MB per SRS 6.1.3)

Options:
1. **Explicit spawn**: Create new thread per trajectory
2. **Fixed thread pool**: Reuse `hardware_concurrency()` workers

### Decision

Implement **fixed-size thread pool** with work-stealing queue.

### Consequences

#### Positive
- ✅ **Bounded resources**: O(cores) threads instead of O(iterations)
- ✅ **Eliminates spawn overhead**: Thread creation costs amortized
- ✅ **Better CPU utilization**: Scheduler has fewer context switches

#### Negative
- ❌ **Implementation complexity**: Requires thread-safe queue + synchronization primitives
- ❌ **Load balancing needed**: Uneven trajectory lengths could starve workers (mitigated by work-stealing)

---

## ADR-004: Embedded Presets over External Config Files

**Status:** ✅ Accepted

**Date:** 2026-01-01

### Context

System provides 3 hand-size presets (Small/Medium/Large) per FR-2.1:
- Distance matrices (10 finger pairs × 6 thresholds)
- Rule weights (15 penalty values)

Options:
1. **Embedded**: Compile presets into binary as `constexpr` strings
2. **External files**: Ship JSON files in `~/.config/piano-fingering/`

### Decision

**Embed presets** in binary; support external custom config via `--config` flag.

### Consequences

#### Positive
- ✅ **Single-file distribution**: Binary works without installation (SRS requirement)
- ✅ **No missing config errors**: Presets always available
- ✅ **Cross-platform consistency**: No file path resolution issues

#### Negative
- ❌ **Presets immutable**: Changing default values requires recompilation
- ❌ **Binary size increase**: ~2KB for 3 presets (negligible - <0.1% of total)

---

## ADR-005: Observer Pattern for Progress Reporting

**Status:** ✅ Accepted

**Date:** 2026-01-01

### Context

Optimizer must report progress to CLI (FR-6.1, FR-6.2) without:
- Coupling algorithm logic to I/O
- Blocking optimization for UI updates

Options:
1. **Polling**: CLI queries optimizer state
2. **Observer/Callback**: Optimizer invokes registered listener

### Decision

Define `IProgressObserver` interface; CLI implements concrete observer.

### Consequences

#### Positive
- ✅ **Decoupling**: Optimizer has no knowledge of stderr/stdout
- ✅ **Testability**: Mock observers enable unit testing without I/O
- ✅ **Flexibility**: Support multiple observers (e.g., file logger + CLI)

#### Negative
- ❌ **Indirection overhead**: Virtual function call per update (~10ns - negligible given 1s update interval)
- ❌ **Thread safety**: Requires mutex if observer called from worker threads (mitigated by main-thread-only updates)

---

## ADR-006: Delta Evaluation over Global Hash Memoization

**Status:** ✅ Accepted

**Date:** 2026-01-01

### Context

Score evaluation is bottleneck (called millions of times in ILS):
- Full evaluation: O(N × M²) for N slices with M notes each
- Need caching strategy to avoid recomputation

Options:
1. **Global hashmap**: Memoize `hash(state) → score`
2. **Delta evaluation**: Precompute intra-slice costs, use O(1) transition deltas

### Decision

Implement **delta evaluation** with precomputed slice costs.

### Consequences

#### Positive
- ✅ **O(1) incremental updates**: Swapping one finger recalculates only affected transitions
- ✅ **Memory bounded**: O(N) storage vs. O(B^N) for beam state cache
- ✅ **Cache locality**: Precomputed arrays accessed sequentially

#### Negative
- ❌ **Complex implementation**: Requires tracking dirty regions in ILS
- ❌ **Limited to local changes**: Full beam search still requires complete evaluation (acceptable - beam is one-pass forward)

---

## ADR-007: Value Semantics for Domain Objects

**Status:** ✅ Accepted

**Date:** 2026-01-01

### Context

Domain objects (Note, Slice, Measure, Piece) are:
- Passed between modules frequently
- Immutable after construction (no setters)
- Small enough for stack allocation

Options:
1. **Value semantics**: Pass by value, rely on move elision
2. **Reference semantics**: Pass `shared_ptr<const Object>`

### Decision

Use **value semantics** with `const` correctness.

### Consequences

#### Positive
- ✅ **No heap allocations**: Stack-allocated temporaries
- ✅ **Thread-safe by default**: No shared mutable state
- ✅ **Deterministic lifetime**: RAII cleanup, no reference cycles

#### Negative
- ❌ **Copy overhead for large pieces**: Mitigated by move semantics + RVO
- ❌ **Cannot share partial state**: Optimizer duplicates piece per trajectory (acceptable - memory budget allows)

---

## ADR-008: Beam Width = 100 (Configurable via Internal Constant)

**Status:** ✅ Accepted

**Date:** 2026-01-01

### Context

Beam search requires fixed width parameter:
- SRS 6.1 implies balanced quality/speed
- Analysis doc recommends 50-200 range

Options:
1. **Fixed 100**: Hardcoded constant
2. **User-configurable**: `--beam-width` flag

### Decision

Hardcode `BEAM_WIDTH = 100` as `constexpr`; no CLI flag.

### Consequences

#### Positive
- ✅ **Simpler UX**: Users don't need to understand beam search internals
- ✅ **Prevents footguns**: Too-small width degrades quality; too-large violates memory budget
- ✅ **Matches analysis**: 100 is optimal for typical pieces

#### Negative
- ❌ **No user tunability**: Power users cannot experiment (acceptable - research parameter, not user-facing)
- ❌ **Recompilation to change**: Could expose as `#define` in config header if needed

---

## Summary of Trade-Offs

| Decision | Primary Benefit | Primary Cost | Accepted Because |
|----------|----------------|--------------|------------------|
| Modular Monolith | Performance | Recompilation | Algorithm is stable |
| pugixml | Parse speed | API surface | Performance critical |
| Thread Pool | Bounded resources | Complexity | Memory budget strict |
| Embedded Presets | Single binary | Immutability | Distribution simplicity |
| Observer Pattern | Testability | Indirection | Cost negligible |
| Delta Evaluation | O(1) updates | Implementation complexity | ILS is bottleneck |
| Value Semantics | Thread safety | Copy overhead | Move elision effective |
| Fixed Beam Width | UX simplicity | No tunability | Research parameter |
