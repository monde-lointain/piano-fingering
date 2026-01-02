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
      {{FingerPair::kThumbRing, FingerPair::kIndexRing, FingerPair::kMiddleRing,
        FingerPair::kRingPinky, FingerPair::kRingPinky}},
      {{FingerPair::kThumbPinky, FingerPair::kIndexPinky,
        FingerPair::kMiddlePinky, FingerPair::kRingPinky,
        FingerPair::kRingPinky}},
  }};
  return kLookup[a - 1][b - 1];
}

double apply_cascading_penalty(const config::FingerPairDistances& distances,
                               int actual_distance,
                               const config::RuleWeights& weights) {
  double penalty = 0.0;

  // Rule 2: relaxed range violation (base layer)
  int rel_violation = 0;
  if (actual_distance < distances.min_rel) {
    rel_violation = distances.min_rel - actual_distance;
  } else if (actual_distance > distances.max_rel) {
    rel_violation = actual_distance - distances.max_rel;
  }
  penalty += rel_violation * weights.values[1];

  // Rule 1: comfort range violation (middle layer)
  int comf_violation = 0;
  if (actual_distance < distances.min_comf) {
    comf_violation = distances.min_comf - actual_distance;
  } else if (actual_distance > distances.max_comf) {
    comf_violation = actual_distance - distances.max_comf;
  }
  penalty += comf_violation * weights.values[0];

  // Rule 13: practical range violation (outer layer)
  int prac_violation = 0;
  if (actual_distance < distances.min_prac) {
    prac_violation = distances.min_prac - actual_distance;
  } else if (actual_distance > distances.max_prac) {
    prac_violation = actual_distance - distances.max_prac;
  }
  penalty += prac_violation * weights.values[12];

  return penalty;
}

double apply_chord_penalty(const config::FingerPairDistances& distances,
                            int actual_distance,
                            const config::RuleWeights& weights) {
  double penalty = 0.0;

  int rel_violation = 0;
  if (actual_distance < distances.min_rel) {
    rel_violation = distances.min_rel - actual_distance;
  } else if (actual_distance > distances.max_rel) {
    rel_violation = actual_distance - distances.max_rel;
  }
  penalty += rel_violation * 2.0 * weights.values[1];  // Doubled

  int comf_violation = 0;
  if (actual_distance < distances.min_comf) {
    comf_violation = distances.min_comf - actual_distance;
  } else if (actual_distance > distances.max_comf) {
    comf_violation = actual_distance - distances.max_comf;
  }
  penalty += comf_violation * 2.0 * weights.values[0];  // Doubled

  int prac_violation = 0;
  if (actual_distance < distances.min_prac) {
    prac_violation = distances.min_prac - actual_distance;
  } else if (actual_distance > distances.max_prac) {
    prac_violation = actual_distance - distances.max_prac;
  }
  penalty += prac_violation * weights.values[12];  // NOT doubled

  return penalty;
}

}  // namespace piano_fingering::evaluator
