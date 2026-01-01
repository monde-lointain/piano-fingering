# System Overview

## Context Diagram

```
┌─────────────────────────┐
│   User (CLI Invocation) │
└────────────┬────────────┘
             │
             ▼
┌────────────────────────────────────────────┐
│  Piano Fingering Generator (Single Binary) │
│                                            │
│  ┌──────────────────────────────────────┐ │
│  │  CLI (Orchestrator)                  │ │
│  └────┬────────────────┬────────────────┘ │
│       │                │                   │
│  ┌────▼─────┐    ┌────▼────────┐         │
│  │ Parser   │    │ Config      │         │
│  │(MusicXML)│    │ Manager     │         │
│  └────┬─────┘    └─────┬───────┘         │
│       │                │                   │
│       │           ┌────▼────────┐         │
│       │           │ Evaluator   │         │
│       │           │(15 Rules)   │         │
│       │           └─────┬───────┘         │
│       │                 │                  │
│       │            ┌────▼────────┐        │
│       │            │ Optimizer   │        │
│       │            │(Beam + ILS) │        │
│       │            └─────┬───────┘        │
│       │                  │                 │
│  ┌────▼──────────────────▼───┐           │
│  │  Generator (MusicXML Out)  │           │
│  └────────────────────────────┘           │
│                                            │
└───────────┬────────────────────────────────┘
            │
            ▼
   ┌────────────────────┐
   │ Fingered MusicXML  │
   │   Output File      │
   └────────────────────┘
```

## System Characteristics

This system is architected as a **single-quantum modular monolith** with the following defining characteristics:

### Top 3 Architecture Characteristics (Ranked)

| Rank | Characteristic | Driving Requirement | Impact on Design |
|------|---------------|---------------------|------------------|
| **1** | **Performance** | Process 2000 notes in <5 seconds on baseline hardware (4-core i5) with <512MB memory | Thread pool for parallel ILS trajectories, precomputed cost deltas, beam search pruning |
| **2** | **Correctness/Determinism** | Same seed produces identical output across platforms; 100% hard constraint satisfaction | Seeded RNG, value semantics in domain model, no global state |
| **3** | **Portability** | Single static binary for Windows, macOS, Linux without runtime dependencies | Header-only XML library, C++20 standard library only, CMake build system |

### Non-Critical Characteristics

The following are **not** primary drivers:

- **Scalability**: Single-user CLI tool, no concurrent requests
- **Availability**: Runs-to-completion, no long-running services
- **Security**: Minimal surface (file I/O only), no network/auth
- **Extensibility**: Fixed 15-rule algorithm from published research

## Quantum Breakdown

This system consists of **one deployable quantum**:

| Quantum | Deployment Unit | Data Owned | Rationale |
|---------|----------------|------------|-----------|
| **Piano Fingering Generator** | Single static binary (`piano-fingering`) | No persistent database - all state is transient (input file → in-memory processing → output file) | SRS mandates single-file distribution; no microservices needed for offline CLI tool |

### Internal Modules (within quantum)

| Module | Coupling Type | Primary Responsibility |
|--------|--------------|------------------------|
| **Domain** | Efferent = 0 | Core value types (Note, Slice, Piece) |
| **Config** | Efferent = 0 | Distance matrices, rule weights, presets |
| **Parser** | Efferent = Domain | MusicXML → Domain objects |
| **Evaluator** | Efferent = Domain, Config | Apply 15 rules, compute score |
| **Optimizer** | Efferent = Domain, Config, Evaluator | Beam Search DP + ILS |
| **Generator** | Efferent = Domain | Domain objects → MusicXML |
| **CLI** | Efferent = All | Orchestration, UX, exit codes |

## Deployment Model

```
┌──────────────────────────────────┐
│   Target Environment             │
│   (Windows/macOS/Linux Desktop)  │
│                                  │
│   ┌──────────────────────────┐  │
│   │  piano-fingering         │  │
│   │  (static binary)         │  │
│   │                          │  │
│   │  ├─ Domain (linked)      │  │
│   │  ├─ Config (linked)      │  │
│   │  ├─ Parser (linked)      │  │
│   │  ├─ Evaluator (linked)   │  │
│   │  ├─ Optimizer (linked)   │  │
│   │  ├─ Generator (linked)   │  │
│   │  └─ CLI (linked)         │  │
│   └──────────────────────────┘  │
│                                  │
│   No external dependencies       │
│   No configuration files needed  │
│   (presets embedded in binary)   │
└──────────────────────────────────┘
```

## Key Architectural Constraints

| Constraint | Source | Implication |
|-----------|--------|-------------|
| Static linking required | SRS 2.4.4 | All dependencies must be header-only or source-embedded |
| C++20 standard | SRS 2.4.4 | Cannot use platform-specific APIs beyond stdlib |
| No network I/O | SRS 5.4 | Offline-only processing |
| Deterministic output | SRS 6.4.4 | RNG must be seeded, no timing-dependent behavior |
| Memory budget <512MB | SRS 6.1.3 | No global memoization, bounded beam width |

## Data Flow

```
[Input MusicXML File]
        │
        ▼
    ┌───────┐
    │Parser │ ──→ [Piece Domain Object]
    └───────┘              │
                           ▼
                    ┌──────────────┐
                    │  Optimizer   │
                    │  (per hand)  │
                    └──────┬───────┘
                           │
            ┌──────────────┼──────────────┐
            ▼              ▼              ▼
       [Left Hand]    [Evaluator]   [Right Hand]
       Fingering      (Rule Engine)  Fingering
            │              │              │
            └──────────────┼──────────────┘
                           ▼
                    ┌──────────────┐
                    │  Generator   │
                    └──────┬───────┘
                           │
                           ▼
              [Fingered MusicXML Output]
```

## Critical Success Factors

1. **Performance Targets Met**: <5s for 2000 notes on baseline hardware
2. **Golden Set Validation**: All 8 test pieces produce Z scores within 5% of baseline
3. **Cross-Platform Parity**: Identical output (byte-for-byte) with same seed on Win/Mac/Linux
4. **80% Code Coverage**: Unit + integration tests achieve target
5. **No Runtime Failures**: 99% success rate on diverse MusicXML corpus
