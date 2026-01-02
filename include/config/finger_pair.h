#ifndef PIANO_FINGERING_CONFIG_FINGER_PAIR_H_
#define PIANO_FINGERING_CONFIG_FINGER_PAIR_H_

#include <cstddef>

namespace piano_fingering::config {

enum class FingerPair {
  kThumbIndex = 0,   // 1-2
  kThumbMiddle,      // 1-3
  kThumbRing,        // 1-4
  kThumbPinky,       // 1-5
  kIndexMiddle,      // 2-3
  kIndexRing,        // 2-4
  kIndexPinky,       // 2-5
  kMiddleRing,       // 3-4
  kMiddlePinky,      // 3-5
  kRingPinky         // 4-5
};

inline constexpr std::size_t kFingerPairCount = 10;

}  // namespace piano_fingering::config

#endif  // PIANO_FINGERING_CONFIG_FINGER_PAIR_H_
