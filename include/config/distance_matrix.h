#ifndef PIANO_FINGERING_CONFIG_DISTANCE_MATRIX_H_
#define PIANO_FINGERING_CONFIG_DISTANCE_MATRIX_H_

#include <algorithm>
#include <array>

#include "config/finger_pair.h"
#include "config/finger_pair_distances.h"

namespace piano_fingering::config {

struct DistanceMatrix {
  std::array<FingerPairDistances, kFingerPairCount> finger_pairs{};

  [[nodiscard]] constexpr const FingerPairDistances& get_pair(
      FingerPair pair) const noexcept {
    return finger_pairs[static_cast<std::size_t>(pair)];  // NOLINT
  }

  [[nodiscard]] constexpr FingerPairDistances& get_pair(
      FingerPair pair) noexcept {
    return finger_pairs[static_cast<std::size_t>(pair)];  // NOLINT
  }

  [[nodiscard]] constexpr bool is_valid() const noexcept {
    return std::all_of(finger_pairs.begin(), finger_pairs.end(),
                       [](const auto& pair) { return pair.is_valid(); });
  }

  [[nodiscard]] constexpr bool operator==(
      const DistanceMatrix& other) const noexcept = default;
};

}  // namespace piano_fingering::config

#endif  // PIANO_FINGERING_CONFIG_DISTANCE_MATRIX_H_
