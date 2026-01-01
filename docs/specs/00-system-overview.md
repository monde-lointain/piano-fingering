# System Overview

## High-Level Context

**Piano Fingering Generator** is a single-binary CLI tool that automatically generates optimal finger assignments for piano sheet music.

**Problem Domain**: NP-hard combinatorial optimization problem with bounded treewidth (≤10), making it polynomial-time tractable via dynamic programming.

---

## Context Diagram

```
┌──────────────────┐
│  User (Pianist)  │
└────────┬─────────┘
         │ invokes CLI
         ▼
┌─────────────────────────────────────────────────────┐
│          Piano Fingering Generator (C++)            │
│  ┌───────────────────────────────────────────────┐  │
│  │ CLI Module                                    │  │
│  │  - Arg parsing, validation                    │  │
│  │  - Orchestration, exit codes                  │  │
│  └───────────────┬───────────────────────────────┘  │
│                  │                                   │
│       ┌──────────┼──────────────┬──────────────┐    │
│       ▼          ▼              ▼              ▼    │
│  ┌────────┐ ┌────────┐ ┌─────────────┐ ┌─────────┐ │
│  │ Config │ │MusicXML│ │   Engine    │ │ Output  │ │
│  │ Module │ │ Parser │ │  (Optimizer)│ │ Writer  │ │
│  └────────┘ └────────┘ └─────────────┘ └─────────┘ │
│      │          │              │              │     │
│      │          │      ┌───────┴───────┐      │     │
│      │          │      │  Cost Module  │      │     │
│      │          │      │  DP Module    │      │     │
│      │          │      │  ILS Module   │      │     │
│      │          │      └───────────────┘      │     │
└──────┼──────────┼──────────────────────────────┼────┘
       │          │                              │
       ▼          ▼                              ▼
  ┌────────┐ ┌──────────┐                  ┌──────────┐
  │Presets │ │ Input    │                  │ Output   │
  │ JSON   │ │MusicXML  │                  │MusicXML  │
  └────────┘ └──────────┘                  └──────────┘
```

---

## Architecture Characteristics (Ranked)

### 1. Performance ★★★★★
**Evidence**: 500 notes <2s, 2000 notes <5s, 512MB RAM, 50% CPU utilization (SRS §6.1)

**Design Impact**:
- Cache-friendly data structures (`std::vector` with `reserve()`)
- Avoid heap allocations in hot paths (cost evaluation loop)
- Parallel ILS with `std::thread::hardware_concurrency()` threads
- Beam width=100 to balance exploration vs. speed

**Measurement**: Wall-clock time via `time` command; RSS via `/usr/bin/time -v`

---

### 2. Portability ★★★★☆
**Evidence**: Win10+/macOS12+/Ubuntu22.04+, x86-64/ARM64, deterministic output with fixed seed (SRS §2.3, §6.4.4)

**Design Impact**:
- C++20 standard library only (no platform APIs)
- `std::thread` for parallelism (no pthreads/WinAPI)
- `std::mt19937` for reproducible RNG
- TinyXML-2 + nlohmann/json (header-only, cross-platform)

**Verification**: CI builds on all 3 platforms; byte-identical output for same seed

---

### 3. Correctness ★★★★★
**Evidence**: 99% success rate, 100% hard constraint compliance, ≤5% regression (SRS §6.4.1, §6.4.6)

**Design Impact**:
- Strong typing (`enum class Hand`, `Finger`, `Pitch`)
- Validation layer in config module (MinPrac < MinComf < MinRel < MaxRel < MaxComf < MaxPrac)
- Golden Set regression tests (8 pieces with baseline Z-scores)
- Hard constraint check: no duplicate fingers in same chord

**Testing**: Google Test unit tests (80% coverage); integration tests on Golden Set

---

## Architectural Style

**Selected**: **Modular Monolith** (Layered Architecture)

**Justification**:
- Single statically-linked binary (SRS §2.4.4 mandate)
- No network/persistence → no IPC overhead benefit
- Performance-critical algorithm needs zero abstraction cost
- Simple deployment for end users

**Alternatives Rejected**:
- Microservices: Overkill; adds IPC overhead; violates single-binary requirement
- Plugin architecture: No extensibility requirement; unnecessary complexity

---

## Module Dependency Graph

```
           ┌─────────┐
           │   CLI   │ (Orchestrator)
           └────┬────┘
                │
     ┏━━━━━━━━━━┻━━━━━━━━━━┓
     ▼          ▼           ▼
┌─────────┐ ┌────────┐ ┌─────────┐
│ Config  │ │MusicXML│ │ Engine  │
└─────────┘ └────────┘ └────┬────┘
                             │
                    ┌────────┼────────┐
                    ▼        ▼        ▼
                ┌──────┐ ┌────┐ ┌──────┐
                │ Cost │ │ DP │ │ ILS  │
                └──────┘ └────┘ └──────┘

Legend:
  → Dependency (reads data from)
  No cycles (DAG enforced)
```

**Key Properties**:
- **DAG**: No circular dependencies
- **Low coupling**: Engine only reads `Piece` + `Config`
- **High cohesion**: Each module owns its data structures

---

## Data Flow

```
1. CLI parses args
   ↓
2. Config module loads preset/JSON → DistanceMatrix + RuleWeights
   ↓
3. MusicXML parser reads input → Piece (Notes, Slices, Measures)
   ↓
4. Engine runs:
   4a. Beam Search DP (construct initial solution)
   4b. ILS (improve via local search + perturbation)
   ↓
5. Output writer merges Solution + Piece → fingered MusicXML
   ↓
6. CLI returns exit code 0
```

---

## Critical Quality Attributes

| Attribute | Target | Verification Method |
|-----------|--------|---------------------|
| **Response Time** | 2000 notes in <5s | Baseline hw: 4-core i5-8250U @ 1.6GHz |
| **Memory Footprint** | <512MB RSS | `/usr/bin/time -v` on 2000-note piece |
| **Reliability** | 99% success rate | Test corpus of 100 valid MusicXML files |
| **Determinism** | Identical output for same seed | Cross-platform byte-diff test |
| **Maintainability** | 80% code coverage | gcov; Google Test suite |

---

## Technology Stack

| Layer | Technology | Rationale |
|-------|-----------|-----------|
| **Language** | C++20 | Performance + modern stdlib (SRS §2.4.4) |
| **Build** | CMake 3.20+ | Cross-platform portability |
| **XML Parsing** | TinyXML-2 | Header-only, zlib license |
| **JSON Parsing** | nlohmann/json | Header-only, robust validation |
| **Threading** | `std::thread` | POSIX + Win32 portability |
| **Testing** | Google Test | Industry standard C++ testing |
| **Static Analysis** | clang-tidy | Enforce C++ Core Guidelines |

---

## Non-Goals (Out of Scope)

- GUI (text-only CLI)
- Real-time MIDI input
- Automatic hand assignment (L/R distribution)
- Cross-hand collision detection
- Notation software plugins
- Network services / telemetry
- Database persistence

---

## Success Criteria

1. ✅ Process Golden Set (8 pieces) without crashes
2. ✅ <2s for 500 notes, <5s for 2000 notes (balanced mode)
3. ✅ Output importable to MuseScore, Finale, Sibelius
4. ✅ 100% hard constraint compliance (no duplicate fingers in chords)
5. ✅ Deterministic output (same seed → same fingering)
6. ✅ 80%+ code coverage via unit tests
7. ✅ Clean build with `-Wall -Wextra -Wpedantic` on GCC/Clang
