#ifndef PIANO_FINGERING_CONFIG_CONFIG_H_
#define PIANO_FINGERING_CONFIG_CONFIG_H_

#include "config/algorithm_parameters.h"
#include "config/distance_matrix.h"
#include "config/rule_weights.h"

namespace piano_fingering::config {

struct Config {
  DistanceMatrix left_hand{};
  DistanceMatrix right_hand{};
  RuleWeights weights{};
  AlgorithmParameters algorithm{};

  [[nodiscard]] constexpr bool is_valid() const noexcept {
    return left_hand.is_valid() && right_hand.is_valid() &&
           weights.is_valid() && algorithm.is_valid();
  }

  [[nodiscard]] constexpr bool operator==(
      const Config& other) const noexcept = default;
};

}  // namespace piano_fingering::config

#endif  // PIANO_FINGERING_CONFIG_CONFIG_H_
