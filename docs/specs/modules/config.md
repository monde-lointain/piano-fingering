# Module: Configuration

## Responsibilities

1. **Define biomechanical constraints**: Distance matrices for Small/Medium/Large hands
2. **Define rule penalties**: Weights for all 15 fingering rules
3. **Provide embedded presets**: Compile-time constants for 3 hand sizes
4. **Parse custom JSON configs**: Load user-specific overlays from file
5. **Validate constraints**: Ensure MinPrac < MinComf < MinRel < MaxRel < MaxComf < MaxPrac

---

## Data Ownership

### Distance Matrix

Stores comfortable/practical finger-pair distances:

| Field | Type | Description |
|-------|------|-------------|
| `finger_pairs` | `std::array<FingerPairDistances, 10>` | Indexed by pair (1-2, 1-3, ..., 4-5) |
| `FingerPairDistances` | `{MinPrac, MinComf, MinRel, MaxRel, MaxComf, MaxPrac}` | All `int` values |

**Constraints:**
- For each pair: `MinPrac ≤ MinComf ≤ MinRel < MaxRel ≤ MaxComf ≤ MaxPrac`
- All values in range `[-20, 20]` (keyboard units)

**Example (Medium Right Hand, pair 1-2):**
```cpp
FingerPairDistances pair_1_2 {
  .MinPrac = -8,
  .MinComf = -6,
  .MinRel = 1,
  .MaxRel = 5,
  .MaxComf = 8,
  .MaxPrac = 10
};
```

### Rule Weights

Penalty multipliers for 15 rules:

| Field | Type | Description |
|-------|------|-------------|
| `weights` | `std::array<double, 15>` | Index 0 = Rule 1, ..., Index 14 = Rule 15 |

**Constraints:**
- All values ≥ 0.0
- Default values from SRS Appendix A.2

**Default Weights:**
```cpp
constexpr std::array<double, 15> DEFAULT_WEIGHTS = {
  2.0,   // Rule 1: Below MinComf or above MaxComf (additional)
  1.0,   // Rule 2: Below MinRel or above MaxRel (base)
  1.0,   // Rule 3: Hand position change (triplet)
  1.0,   // Rule 4: Distance exceeds comfort (triplet)
  1.0,   // Rule 5: Fourth finger usage
  1.0,   // Rule 6: Third and fourth finger consecutive
  1.0,   // Rule 7: Third on white, fourth on black
  0.5,   // Rule 8: Thumb on black key (base)
  1.0,   // Rule 9: Fifth finger on black key
  1.0,   // Rule 10: Thumb crossing (same level)
  2.0,   // Rule 11: Thumb on black crossed by finger on white
  1.0,   // Rule 12: Same finger reuse with position change
  10.0,  // Rule 13: Below MinPrac or above MaxPrac (additional)
  1.0,   // Rule 14: Apply rules 1,2,13 within chord (marker)
  1.0    // Rule 15: Same pitch, different finger consecutively
};
```

### Algorithm Parameters

| Field | Type | Description | Default |
|-------|------|-------------|---------|
| `beam_width` | `size_t` | Max states kept per slice | 100 |
| `ils_iterations` | `size_t` | ILS improvement iterations | 1000 (balanced) |
| `perturbation_strength` | `size_t` | Notes modified per perturb | 3 |

### Preset Definition

```cpp
struct Preset {
  std::string name;
  DistanceMatrix left_hand;
  DistanceMatrix right_hand;
  RuleWeights weights;
};
```

Three presets embedded as `constexpr`:
- `PRESET_SMALL` - Reduced distances for children/small hands
- `PRESET_MEDIUM` - Default from SRS Table 1
- `PRESET_LARGE` - Increased distances for large hands

---

## Communication

### Inbound API

```cpp
// Primary loader
class ConfigManager {
public:
  // Load preset by name
  static Config load_preset(const std::string& name);  // "Small", "Medium", "Large"

  // Load custom JSON with preset as base
  static Config load_custom(const std::filesystem::path& json_path,
                            const std::string& base_preset = "Medium");

  // Validate configuration
  static bool validate(const Config& cfg, std::string& error_msg);
};
```

### Outbound Dependencies

- **Domain**: Uses `Hand` enum to distinguish left/right matrices
- **None**: No other module dependencies

---

## Coupling Analysis

### Afferent Coupling
Depended upon by:
- **CLI** (loads config based on flags)
- **Evaluator** (reads rule weights and distance matrices)
- **Optimizer** (reads algorithm parameters)

### Efferent Coupling
- **Domain** (for `Hand` enum)
- **External JSON library** (for custom config parsing)

**Instability:** Low (stable module - core data definitions)

---

## Testing Strategy

### Unit Tests

#### 1. Constraint Validation
```cpp
TEST(ConfigTest, DistanceMatrixConstraints) {
  DistanceMatrix dm = PRESET_MEDIUM.right_hand;
  for (auto [pair, distances] : dm.finger_pairs) {
    EXPECT_LT(distances.MinPrac, distances.MinComf);
    EXPECT_LT(distances.MinComf, distances.MinRel);
    EXPECT_LT(distances.MinRel, distances.MaxRel);
    EXPECT_LT(distances.MaxRel, distances.MaxComf);
    EXPECT_LT(distances.MaxComf, distances.MaxPrac);
  }
}

TEST(ConfigTest, InvalidMatrixRejected) {
  DistanceMatrix invalid;
  invalid.set_pair(FingerPair::THUMB_INDEX,
                   {.MinPrac = 10, .MinComf = 5, /* ... */});  // Inverted!
  std::string error;
  EXPECT_FALSE(ConfigManager::validate(Config{invalid, {}, {}}, error));
  EXPECT_THAT(error, HasSubstr("MinPrac must be < MinComf"));
}
```

#### 2. JSON Parsing

**Valid Custom Config:**
```json
{
  "distance_matrix": {
    "right_hand": {
      "1-2": {
        "MinPrac": -10,
        "MaxPrac": 12
      }
    }
  },
  "rule_weights": [
    2.5,  // Override Rule 1 weight
    null, // Keep default for Rule 2
    // ... can omit remaining
  ]
}
```

```cpp
TEST(ConfigTest, PartialJSONOverlay) {
  Config cfg = ConfigManager::load_custom("partial.json", "Medium");

  // Check override applied
  EXPECT_EQ(cfg.right_hand.get_pair(FingerPair::THUMB_INDEX).MinPrac, -10);

  // Check default preserved
  EXPECT_EQ(cfg.right_hand.get_pair(FingerPair::INDEX_MIDDLE).MinPrac, 1);

  // Check rule weight override
  EXPECT_DOUBLE_EQ(cfg.weights[0], 2.5);
  EXPECT_DOUBLE_EQ(cfg.weights[1], 1.0);  // Default
}
```

#### 3. Missing Fields
```cpp
TEST(ConfigTest, MissingFieldsUsesDefaults) {
  Config cfg = ConfigManager::load_custom("empty.json", "Large");
  EXPECT_EQ(cfg.right_hand, PRESET_LARGE.right_hand);  // No overrides
}
```

#### 4. Invalid JSON
```cpp
TEST(ConfigTest, MalformedJSONThrows) {
  EXPECT_THROW(
    ConfigManager::load_custom("malformed.json", "Medium"),
    ConfigurationError
  );
}
```

#### 5. Edge Cases
- **Negative weights rejected**
- **Out-of-range distances rejected** ([-20, 20] bounds)
- **Unknown preset name throws**
- **Unknown JSON fields logged as warnings**

### Integration Tests
**N/A** - Config is self-contained data module.

---

## Design Constraints

1. **Presets are immutable**: Compiled into binary as `constexpr` data
2. **JSON overlay only**: Custom configs cannot define new presets, only override fields
3. **No state mutation**: ConfigManager is stateless (static methods only)
4. **Validation on load**: Invalid configs throw immediately, not during use

---

## File Structure

```
include/config/
  distance_matrix.h       // DistanceMatrix struct + accessors
  rule_weights.h          // RuleWeights typedef
  preset.h                // Preset struct + embedded constants
  config_manager.h        // Loading and validation logic
src/config/
  config_manager.cpp      // JSON parsing implementation
  embedded_presets.cpp    // PRESET_SMALL/MEDIUM/LARGE definitions
```

---

## Critical Implementation Details

### Embedded Preset Data

```cpp
// embedded_presets.cpp
namespace presets {

constexpr DistanceMatrix MEDIUM_RIGHT_HAND = {
  .finger_pairs = {{
    // Index 0: FingerPair::THUMB_INDEX (1-2)
    {.MinPrac = -8, .MinComf = -6, .MinRel = 1, .MaxRel = 5, .MaxComf = 8, .MaxPrac = 10},
    // Index 1: FingerPair::THUMB_MIDDLE (1-3)
    {.MinPrac = -7, .MinComf = -5, .MinRel = 3, .MaxRel = 9, .MaxComf = 12, .MaxPrac = 14},
    // ... remaining 8 pairs
  }}
};

constexpr Preset PRESET_MEDIUM = {
  .name = "Medium",
  .left_hand = /* symmetric transformation of right hand */,
  .right_hand = MEDIUM_RIGHT_HAND,
  .weights = DEFAULT_WEIGHTS
};

} // namespace presets
```

### JSON Schema

Using a lightweight JSON library (e.g., `nlohmann/json` or `simdjson`):

```cpp
#include <nlohmann/json.hpp>

Config ConfigManager::load_custom(const fs::path& path, const std::string& base) {
  Config cfg = load_preset(base);  // Start with preset

  std::ifstream file(path);
  if (!file.is_open()) {
    throw ConfigurationError("Cannot open config file: " + path.string());
  }

  nlohmann::json j;
  file >> j;

  // Apply overrides
  if (j.contains("distance_matrix")) {
    apply_matrix_overrides(cfg, j["distance_matrix"]);
  }

  if (j.contains("rule_weights")) {
    apply_weight_overrides(cfg, j["rule_weights"]);
  }

  // Validate final config
  std::string error;
  if (!validate(cfg, error)) {
    throw ConfigurationError("Invalid configuration: " + error);
  }

  return cfg;
}
```

---

## Dependencies

### External Libraries

**Option 1: nlohmann/json** (header-only, MIT license)
- Pro: Intuitive API, widely used
- Con: Slower than binary parsers

**Option 2: simdjson** (header-only, Apache 2.0)
- Pro: Extremely fast
- Con: Overkill for small config files

**Recommendation:** `nlohmann/json` for simplicity (config loading is not in hot path).

### Build Integration

```cmake
# CMakeLists.txt
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(piano-fingering PRIVATE nlohmann_json::nlohmann_json)
```

---

## Future Extensibility

- **User-defined rules**: Allow custom rule definitions in JSON
- **Hand-specific weights**: Different penalties for left vs. right hand
- **Dynamic presets**: Load from `~/.config/piano-fingering/presets/` if present
