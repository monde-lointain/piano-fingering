#ifndef PIANO_FINGERING_CONFIG_DISTANCE_MATRIX_H_
#define PIANO_FINGERING_CONFIG_DISTANCE_MATRIX_H_

#include <array>
#include "config/finger_pair.h"
#include "config/finger_pair_distances.h"

namespace piano_fingering::config {

struct DistanceMatrix {
  std::array<FingerPairDistances, kFingerPairCount> finger_pairs{};

  [[nodiscard]] constexpr const FingerPairDistances& get_pair(
      FingerPair pair) const noexcept {
    return finger_pairs[static_cast<std::size_t>(pair)];
  }

  [[nodiscard]] constexpr FingerPairDistances& get_pair(
      FingerPair pair) noexcept {
    return finger_pairs[static_cast<std::size_t>(pair)];
  }

  [[nodiscard]] constexpr bool is_valid() const noexcept {
    for (const auto& pair : finger_pairs) {
      if (!pair.is_valid()) return false;
    }
    return true;
  }

  [[nodiscard]] constexpr bool operator==(
      const DistanceMatrix& other) const noexcept = default;
};

}  // namespace piano_fingering::config

#endif  // PIANO_FINGERING_CONFIG_DISTANCE_MATRIX_H_
