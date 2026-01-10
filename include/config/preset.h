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

// Preset factory functions for right hand matrices
[[nodiscard]] DistanceMatrix make_small_right_hand();
[[nodiscard]] DistanceMatrix make_medium_right_hand();
[[nodiscard]] DistanceMatrix make_large_right_hand();

// Mirror right hand matrix to left hand
[[nodiscard]] DistanceMatrix mirror_to_left_hand(const DistanceMatrix& right);

}  // namespace piano_fingering::config

#endif  // PIANO_FINGERING_CONFIG_PRESET_H_
