#include "evaluator/rules.h"

#include <algorithm>
#include <array>

namespace piano_fingering::evaluator {

using config::FingerPair;
using domain::Finger;
using domain::to_int;

FingerPair finger_pair_from(Finger f1, Finger f2) {
  int a = to_int(f1);
  int b = to_int(f2);
  if (a > b) {
    std::swap(a, b);
  }

  static constexpr std::array<std::array<FingerPair, 5>, 5> kLookup = {{
      {{FingerPair::kThumbIndex, FingerPair::kThumbIndex,
        FingerPair::kThumbMiddle, FingerPair::kThumbRing,
        FingerPair::kThumbPinky}},
      {{FingerPair::kThumbIndex, FingerPair::kIndexMiddle,
        FingerPair::kIndexMiddle, FingerPair::kIndexRing,
        FingerPair::kIndexPinky}},
      {{FingerPair::kThumbMiddle, FingerPair::kIndexMiddle,
        FingerPair::kMiddleRing, FingerPair::kMiddleRing,
        FingerPair::kMiddlePinky}},
      {{FingerPair::kThumbRing, FingerPair::kIndexRing,
        FingerPair::kMiddleRing, FingerPair::kRingPinky,
        FingerPair::kRingPinky}},
      {{FingerPair::kThumbPinky, FingerPair::kIndexPinky,
        FingerPair::kMiddlePinky, FingerPair::kRingPinky,
        FingerPair::kRingPinky}},
  }};
  return kLookup[a - 1][b - 1];
}

}  // namespace piano_fingering::evaluator
