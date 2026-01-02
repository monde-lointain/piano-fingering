#ifndef PIANO_FINGERING_CONFIG_PRESET_H_
#define PIANO_FINGERING_CONFIG_PRESET_H_

#include <string>
#include <string_view>
#include "config/config.h"

namespace piano_fingering::config {

inline constexpr std::string_view kPresetSmall = "Small";
inline constexpr std::string_view kPresetMedium = "Medium";
inline constexpr std::string_view kPresetLarge = "Large";

struct Preset {
  std::string name;
  DistanceMatrix left_hand{};
  DistanceMatrix right_hand{};
  RuleWeights weights{};

  [[nodiscard]] Config to_config() const {
    return Config{left_hand, right_hand, weights, AlgorithmParameters{}};
  }
};

[[nodiscard]] const Preset& get_small_preset();
[[nodiscard]] const Preset& get_medium_preset();
[[nodiscard]] const Preset& get_large_preset();

}  // namespace piano_fingering::config

#endif  // PIANO_FINGERING_CONFIG_PRESET_H_
