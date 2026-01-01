# Module Specification: Config

## Responsibilities

- Load built-in presets (Small, Medium, Large hand sizes)
- Parse custom JSON configuration files
- Validate distance matrix constraints (MinPrac < MinComf < MinRel < MaxRel < MaxComf < MaxPrac)
- Validate rule weights (all non-negative)
- Merge custom config with preset (overlay semantics)
- Provide immutable `Config` struct to engine module

---

## Data Ownership

### Primary Data Structures

```cpp
// Distance bounds for a single finger pair (e.g., 1-2)
struct FingerPairDistance {
    int MinPrac;   // Minimum practical distance
    int MinComf;   // Minimum comfortable distance
    int MinRel;    // Minimum relaxed distance
    int MaxRel;    // Maximum relaxed distance
    int MaxComf;   // Maximum comfortable distance
    int MaxPrac;   // Maximum practical distance
};

// Distance matrix for all 10 finger pairs (1-2, 1-3, ..., 4-5)
class DistanceMatrix {
public:
    // Get distance bounds for a specific finger pair
    const FingerPairDistance& Get(Finger f1, Finger f2) const;

    // Validate ordering constraints
    // Returns: true if valid, false otherwise
    bool Validate(std::string* error_msg = nullptr) const;

private:
    // 10 finger pairs: 1-2, 1-3, 1-4, 1-5, 2-3, 2-4, 2-5, 3-4, 3-5, 4-5
    std::array<FingerPairDistance, 10> pairs_;
};

// Penalty weights for all 15 fingering rules
struct RuleWeights {
    std::array<float, 15> weights;

    // Validate all weights are non-negative
    bool Validate(std::string* error_msg = nullptr) const;
};

// Complete configuration (immutable after construction)
class Config {
public:
    Config(const DistanceMatrix& matrix, const RuleWeights& weights)
        : distance_matrix_(matrix), rule_weights_(weights) {}

    const DistanceMatrix& GetDistanceMatrix() const { return distance_matrix_; }
    const RuleWeights& GetRuleWeights() const { return rule_weights_; }

private:
    DistanceMatrix distance_matrix_;
    RuleWeights rule_weights_;
};
```

### Preset Enumeration

```cpp
enum class Preset {
    Small,   // Child/small hands
    Medium,  // Average adult hands (default)
    Large    // Large adult hands
};
```

---

## Communication

### Incoming Dependencies (Afferent)

| Caller | Input |
|--------|-------|
| `cli` | `LoadConfig(preset, optional<config_path>)` |

### Outgoing Dependencies (Efferent)

**None** (pure data module, no external dependencies)

**Note**: This module only depends on:
- C++ standard library (`<array>`, `<optional>`, `<string>`)
- `nlohmann/json` (header-only)
- `common/types.h` (`Finger` enum)

---

## Coupling

### Afferent Coupling (Incoming)
- `cli` module

**Count**: 1 (low)

### Efferent Coupling (Outgoing)
- None (data-only module)

**Count**: 0 (minimal)

**Stability**: Very stable (pure data, no logic dependencies)

---

## API Specification

```cpp
// In src/config/config.h
namespace config {

// Load configuration from preset and optional custom JSON file
// Throws: ConfigError if validation fails
Config LoadConfig(Preset preset, const std::optional<std::string>& config_path);

// Load preset only (no custom overlay)
Config LoadPreset(Preset preset);

// Load custom JSON file and merge with base preset
// Overlay semantics: custom values override preset values
Config LoadCustomConfig(Preset base_preset, const std::string& json_path);

// Exception type for configuration errors
class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& msg) : std::runtime_error(msg) {}
};

}  // namespace config
```

---

## Distance Matrix Validation (FR-2.9)

### Ordering Constraints

For each finger pair, the following must hold:

```
MinPrac < MinComf < MinRel < MaxRel < MaxComf < MaxPrac
```

**Example validation logic**:

```cpp
bool FingerPairDistance::Validate(std::string* error_msg) const {
    if (!(MinPrac < MinComf && MinComf < MinRel &&
          MinRel < MaxRel && MaxRel < MaxComf &&
          MaxComf < MaxPrac)) {
        if (error_msg) {
            *error_msg = "Distance ordering violated: "
                         "MinPrac < MinComf < MinRel < MaxRel < MaxComf < MaxPrac";
        }
        return false;
    }
    return true;
}
```

### Value Range

All distance values must be integers in range `[-20, 20]` (based on keyboard span).

---

## Preset Definitions

### Medium Hand (Default) - From SRS Appendix A.1

```cpp
constexpr FingerPairDistance kMediumPreset[10] = {
    // 1-2
    {.MinPrac = -8, .MinComf = -6, .MinRel = 1, .MaxRel = 5, .MaxComf = 8, .MaxPrac = 10},
    // 1-3
    {.MinPrac = -7, .MinComf = -5, .MinRel = 3, .MaxRel = 9, .MaxComf = 12, .MaxPrac = 14},
    // 1-4
    {.MinPrac = -5, .MinComf = -3, .MinRel = 5, .MaxRel = 11, .MaxComf = 13, .MaxPrac = 15},
    // 1-5
    {.MinPrac = -2, .MinComf = 0, .MinRel = 7, .MaxRel = 12, .MaxComf = 14, .MaxPrac = 16},
    // 2-3
    {.MinPrac = 1, .MinComf = 1, .MinRel = 1, .MaxRel = 2, .MaxComf = 5, .MaxPrac = 7},
    // 2-4
    {.MinPrac = 1, .MinComf = 1, .MinRel = 3, .MaxRel = 4, .MaxComf = 6, .MaxPrac = 8},
    // 2-5
    {.MinPrac = 2, .MinComf = 2, .MinRel = 5, .MaxRel = 6, .MaxComf = 10, .MaxPrac = 12},
    // 3-4
    {.MinPrac = 1, .MinComf = 1, .MinRel = 1, .MaxRel = 2, .MaxComf = 2, .MaxPrac = 4},
    // 3-5
    {.MinPrac = 1, .MinComf = 1, .MinRel = 3, .MaxRel = 4, .MaxComf = 6, .MaxPrac = 8},
    // 4-5
    {.MinPrac = 1, .MinComf = 1, .MinRel = 1, .MaxRel = 2, .MaxComf = 4, .MaxPrac = 6},
};
```

### Small Hand

Scale down all absolute values by 15% (round to nearest integer):

```cpp
constexpr FingerPairDistance kSmallPreset[10] = {
    // 1-2
    {.MinPrac = -7, .MinComf = -5, .MinRel = 1, .MaxRel = 4, .MaxComf = 7, .MaxPrac = 9},
    // ... (apply 0.85 scaling factor to all Medium values)
};
```

### Large Hand

Scale up all absolute values by 15%:

```cpp
constexpr FingerPairDistance kLargePreset[10] = {
    // 1-2
    {.MinPrac = -9, .MinComf = -7, .MinRel = 1, .MaxRel = 6, .MaxComf = 9, .MaxPrac = 12},
    // ... (apply 1.15 scaling factor to all Medium values)
};
```

---

## Default Rule Weights (SRS Appendix A.2)

```cpp
constexpr RuleWeights kDefaultWeights = {
    .weights = {
        2.0f,   // Rule 1: Below MinComf or above MaxComf
        1.0f,   // Rule 2: Below MinRel or above MaxRel
        1.0f,   // Rule 3: Hand position change (triplet)
        1.0f,   // Rule 4: Distance exceeds comfort (triplet)
        1.0f,   // Rule 5: Fourth finger usage
        1.0f,   // Rule 6: Third and fourth finger consecutive
        1.0f,   // Rule 7: Third on white, fourth on black
        1.0f,   // Rule 8: Thumb on black key (0.5 base + 1 per adjacent white)
        1.0f,   // Rule 9: Fifth finger on black key
        1.0f,   // Rule 10: Thumb crossing (same level)
        2.0f,   // Rule 11: Thumb on black crossed by finger on white
        1.0f,   // Rule 12: Same finger reuse with position change
        10.0f,  // Rule 13: Below MinPrac or above MaxPrac
        1.0f,   // Rule 14: Rules 1, 2, 13 within chord (doubled)
        1.0f    // Rule 15: Same pitch, different finger
    }
};
```

---

## JSON Schema

### Custom Configuration File Format

```json
{
    "distance_matrix": {
        "1-2": {
            "MinPrac": -8,
            "MinComf": -6,
            "MinRel": 1,
            "MaxRel": 5,
            "MaxComf": 8,
            "MaxPrac": 10
        },
        "1-3": { "MinPrac": -7, /* ... */ },
        ...
    },
    "rule_weights": [
        2.0,  // Rule 1
        1.0,  // Rule 2
        ...
        1.0   // Rule 15
    ]
}
```

### Partial Configuration (Overlay Semantics - FR-2.8)

User can specify only the fields they want to override:

```json
{
    "distance_matrix": {
        "1-2": {
            "MaxPrac": 12  // Override only this field; others from preset
        }
    },
    "rule_weights": [
        3.0,  // Override Rule 1 penalty
        null, null, null, null, null, null, null, null, null,
        null, null, null, null, null
    ]
}
```

**Merge logic**: For each field in custom JSON:
- If present and non-null → Use custom value
- If absent or null → Use preset value

---

## Testing Strategy

### Unit Tests

| Test Case | Scenario | Expected Outcome |
|-----------|----------|------------------|
| `LoadMediumPreset` | Load default preset | All 10 finger pairs match Appendix A.1 |
| `LoadSmallPreset` | Load small hand preset | Values scaled by 0.85 |
| `LoadLargePreset` | Load large hand preset | Values scaled by 1.15 |
| `ValidateOrdering` | MinPrac < MinComf < ... | Validation passes |
| `ValidateOrderingViolation` | MaxComf < MaxRel | Throws `ConfigError` |
| `ValidateNegativeWeight` | Rule weight = -1.0 | Throws `ConfigError` |
| `ParseValidJSON` | Well-formed JSON file | Config loaded successfully |
| `ParseInvalidJSON` | Malformed JSON | Throws `ConfigError` |
| `MergePartialConfig` | Override 1-2 MaxPrac only | Other fields from preset |
| `UnrecognizedField` | JSON contains "unknown_field" | Warning printed, field ignored |

**Implementation**:

```cpp
TEST(ConfigTest, LoadMediumPreset) {
    auto config = config::LoadPreset(Preset::Medium);
    auto& matrix = config.GetDistanceMatrix();
    auto pair_1_2 = matrix.Get(Finger::Thumb, Finger::Index);
    EXPECT_EQ(pair_1_2.MinPrac, -8);
    EXPECT_EQ(pair_1_2.MaxPrac, 10);
}

TEST(ConfigTest, ValidateOrderingViolation) {
    FingerPairDistance invalid{
        .MinPrac = -8,
        .MinComf = -6,
        .MinRel = 1,
        .MaxRel = 5,
        .MaxComf = 3,  // VIOLATION: MaxComf < MaxRel
        .MaxPrac = 10
    };
    std::string error_msg;
    EXPECT_FALSE(invalid.Validate(&error_msg));
    EXPECT_THAT(error_msg, testing::HasSubstr("ordering violated"));
}
```

### Integration Tests

| Test Case | Scenario | Verification |
|-----------|----------|--------------|
| `LoadConfigFromFile` | Load custom JSON + Medium preset | Merged config correct |
| `InvalidConfigExits` | CLI invoked with invalid JSON | Exit code 4, error message |
| `MissingConfigFile` | --config points to nonexistent file | Exit code 4 |

---

## Implementation Notes

### Finger Pair Indexing

Finger pairs are stored in a fixed order:

```cpp
enum class FingerPairIndex : size_t {
    Pair_1_2 = 0,
    Pair_1_3 = 1,
    Pair_1_4 = 2,
    Pair_1_5 = 3,
    Pair_2_3 = 4,
    Pair_2_4 = 5,
    Pair_2_5 = 6,
    Pair_3_4 = 7,
    Pair_3_5 = 8,
    Pair_4_5 = 9
};

// Helper function to convert (f1, f2) → index
size_t GetPairIndex(Finger f1, Finger f2) {
    // Ensure f1 < f2 (canonical order)
    if (f1 > f2) std::swap(f1, f2);

    int i1 = static_cast<int>(f1);  // 1-5
    int i2 = static_cast<int>(f2);  // 1-5

    // Combinatorial index: C(i2, 2) + (i1 - 1)
    return /* calculate index */;
}
```

### Reverse Distance Lookup (Problem Description §2.1)

For reverse finger pairs (e.g., 2-1 instead of 1-2):

```
MinPrac(2-1) = -MaxPrac(1-2)
MaxPrac(2-1) = -MinPrac(1-2)
```

**Implementation**:

```cpp
const FingerPairDistance& DistanceMatrix::Get(Finger f1, Finger f2) const {
    bool reversed = (f1 > f2);
    if (reversed) std::swap(f1, f2);

    size_t idx = GetPairIndex(f1, f2);
    const auto& base = pairs_[idx];

    if (reversed) {
        // Return negated and swapped bounds
        static thread_local FingerPairDistance reversed_cache;
        reversed_cache = {
            .MinPrac = -base.MaxPrac,
            .MinComf = -base.MaxComf,
            .MinRel = -base.MaxRel,
            .MaxRel = -base.MinRel,
            .MaxComf = -base.MinComf,
            .MaxPrac = -base.MinPrac
        };
        return reversed_cache;
    }

    return base;
}
```

---

## File Structure

```
src/config/
├── config.h           // Public API (LoadConfig, Config class)
├── config.cpp         // Implementation
├── presets.cpp        // Embedded preset definitions
├── json_parser.cpp    // JSON loading + validation
└── validation.cpp     // Constraint checking
```

---

## Dependencies

| Dependency | Purpose |
|------------|---------|
| `nlohmann/json` | JSON parsing (header-only) |
| `common/types.h` | `Finger` enum |
| C++ stdlib | `<array>`, `<optional>`, `<string>`, `<stdexcept>` |

---

## Error Handling

All configuration errors throw `config::ConfigError`:

```cpp
// Example error messages (FR-2.10)
throw ConfigError("Distance matrix: MinPrac must be less than MinComf for finger pair 1-2");
throw ConfigError("Rule weight 13 cannot be negative (got -2.5)");
throw ConfigError("JSON parse error at line 42: unexpected token");
throw ConfigError("Missing required field 'distance_matrix.1-2.MinPrac'");
```

---

## Success Criteria

- ✅ All 3 presets (Small, Medium, Large) validate successfully
- ✅ Distance matrix ordering constraint checked for all 10 pairs
- ✅ Partial JSON configs merge correctly with preset base
- ✅ Invalid configs throw `ConfigError` with actionable message
- ✅ Unrecognized JSON fields ignored with warning (FR-2.11)
- ✅ Config immutable after construction (const correctness)
