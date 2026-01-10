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
| `FingerPairDistances` | `{min_prac, min_comf, min_rel, max_rel, max_comf, max_prac}` | All `int` values |

**Structures:**
```cpp
enum class FingerPair {
  kThumbIndex = 0, kThumbMiddle, kThumbRing, kThumbPinky,
  kIndexMiddle, kIndexRing, kIndexPinky,
  kMiddleRing, kMiddlePinky,
  kRingPinky
};
inline constexpr std::size_t kFingerPairCount = 10;

inline constexpr int kMinDistanceValue = -20;
inline constexpr int kMaxDistanceValue = 20;

struct FingerPairDistances {
  int min_prac, min_comf, min_rel, max_rel, max_comf, max_prac;
  [[nodiscard]] constexpr bool is_valid() const noexcept;
  [[nodiscard]] constexpr bool operator==(const FingerPairDistances&) const noexcept = default;
};

struct DistanceMatrix {
  std::array<FingerPairDistances, kFingerPairCount> finger_pairs{};
  [[nodiscard]] constexpr const FingerPairDistances& get_pair(FingerPair pair) const noexcept;
  [[nodiscard]] constexpr FingerPairDistances& get_pair(FingerPair pair) noexcept;
  [[nodiscard]] constexpr bool is_valid() const noexcept;
  [[nodiscard]] constexpr bool operator==(const DistanceMatrix&) const noexcept = default;
};
```

**Constraints:**
- For each pair: `min_prac ≤ min_comf ≤ min_rel < max_rel ≤ max_comf ≤ max_prac`
- All values in range `[-20, 20]` (keyboard units)

**Example (Medium Right Hand, pair 1-2):**
```cpp
FingerPairDistances pair_1_2 {
  .min_prac = -8,
  .min_comf = -6,
  .min_rel = 1,
  .max_rel = 5,
  .max_comf = 8,
  .max_prac = 10
};
```

### Rule Weights

Penalty multipliers for 15 rules:

| Field | Type | Description |
|-------|------|-------------|
| `values` | `std::array<double, 15>` | Index 0 = Rule 1, ..., Index 14 = Rule 15 |

**Structure:**
```cpp
struct RuleWeights {
  std::array<double, kRuleCount> values{};
  [[nodiscard]] constexpr bool is_valid() const noexcept;
  [[nodiscard]] static constexpr RuleWeights defaults() noexcept;
  [[nodiscard]] constexpr bool operator==(const RuleWeights&) const noexcept = default;
};

inline constexpr std::size_t kRuleCount = 15;
```

**Constraints:**
- All values ≥ 0.0
- Default values from SRS Appendix A.2

**Default Weights:**
```cpp
constexpr std::array<double, 15> DEFAULT_WEIGHTS_VALUES = {
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

```cpp
struct AlgorithmParameters {
  std::size_t beam_width = 100;
  std::size_t ils_iterations = 1000;
  std::size_t perturbation_strength = 3;
  [[nodiscard]] constexpr bool is_valid() const noexcept;
  [[nodiscard]] constexpr bool operator==(const AlgorithmParameters&) const noexcept = default;
};
```

| Field | Type | Description | Default |
|-------|------|-------------|---------|
| `beam_width` | `size_t` | Max states kept per slice | 100 |
| `ils_iterations` | `size_t` | ILS improvement iterations | 1000 (balanced) |
| `perturbation_strength` | `size_t` | Notes modified per perturb | 3 |

### Config Structure

```cpp
struct Config {
  DistanceMatrix left_hand{};
  DistanceMatrix right_hand{};
  RuleWeights weights{};
  AlgorithmParameters algorithm{};
  [[nodiscard]] constexpr bool is_valid() const noexcept;
  [[nodiscard]] constexpr bool operator==(const Config&) const noexcept = default;
};
```

### Preset Definition

```cpp
struct Preset {
  std::string name;
  DistanceMatrix left_hand{};
  DistanceMatrix right_hand{};
  RuleWeights weights{};
  [[nodiscard]] Config to_config() const;
};

inline constexpr std::string_view kPresetSmall = "Small";
inline constexpr std::string_view kPresetMedium = "Medium";
inline constexpr std::string_view kPresetLarge = "Large";

[[nodiscard]] const Preset& get_small_preset();
[[nodiscard]] const Preset& get_medium_preset();
[[nodiscard]] const Preset& get_large_preset();
```

Three presets embedded as `constexpr`:
- `PRESET_SMALL` - Reduced distances for children/small hands
- `PRESET_MEDIUM` - Default from SRS Table 1
- `PRESET_LARGE` - Increased distances for large hands

---

## Communication

### Inbound API

```cpp
// Primary loader (static methods only, no instances)
class ConfigManager {
public:
  ConfigManager() = delete;  // Stateless class

  // Load preset by name
  [[nodiscard]] static Config load_preset(std::string_view name);

  // Load custom JSON with preset as base
  [[nodiscard]] static Config load_custom(const std::filesystem::path& path,
                                          std::string_view base_preset = "Medium");

  // Validate configuration
  [[nodiscard]] static bool validate(const Config& config, std::string& error_message);
};

// Exception type
class ConfigurationError : public std::runtime_error {
public:
  explicit ConfigurationError(const std::string& message);
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
  const Preset& medium = get_medium_preset();
  DistanceMatrix dm = medium.right_hand;
  for (const auto& distances : dm.finger_pairs) {
    EXPECT_LT(distances.min_prac, distances.min_comf);
    EXPECT_LT(distances.min_comf, distances.min_rel);
    EXPECT_LT(distances.min_rel, distances.max_rel);
    EXPECT_LT(distances.max_rel, distances.max_comf);
    EXPECT_LT(distances.max_comf, distances.max_prac);
  }
}

TEST(ConfigTest, InvalidMatrixRejected) {
  DistanceMatrix invalid;
  invalid.get_pair(FingerPair::kThumbIndex) =
      {.min_prac = 10, .min_comf = 5, /* ... */};  // Inverted!
  std::string error;
  EXPECT_FALSE(ConfigManager::validate(Config{invalid, {}, {}, {}}, error));
  EXPECT_THAT(error, HasSubstr("min_prac must be < min_comf"));
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
  EXPECT_EQ(cfg.right_hand.get_pair(FingerPair::kThumbIndex).min_prac, -10);

  // Check default preserved
  EXPECT_EQ(cfg.right_hand.get_pair(FingerPair::kIndexMiddle).min_prac, 1);

  // Check rule weight override
  EXPECT_DOUBLE_EQ(cfg.weights.values[0], 2.5);
  EXPECT_DOUBLE_EQ(cfg.weights.values[1], 1.0);  // Default
}
```

#### 3. Missing Fields
```cpp
TEST(ConfigTest, MissingFieldsUsesDefaults) {
  Config cfg = ConfigManager::load_custom("empty.json", "Large");
  EXPECT_EQ(cfg.right_hand, get_large_preset().right_hand);  // No overrides
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
  finger_pair.h              // FingerPair enum
  finger_pair_distances.h    // FingerPairDistances struct + constants
  distance_matrix.h          // DistanceMatrix struct + accessors
  rule_weights.h             // RuleWeights struct
  algorithm_parameters.h     // AlgorithmParameters struct
  config.h                   // Config struct
  configuration_error.h      // ConfigurationError exception
  preset.h                   // Preset struct + accessor functions
  config_manager.h           // Loading and validation logic
src/config/
  config_manager.cpp         // JSON parsing implementation
  embedded_presets.cpp       // Preset definitions (get_*_preset functions)
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
