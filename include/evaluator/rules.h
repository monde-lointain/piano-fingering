#ifndef PIANO_FINGERING_EVALUATOR_RULES_H_
#define PIANO_FINGERING_EVALUATOR_RULES_H_

#include "config/finger_pair.h"
#include "config/finger_pair_distances.h"
#include "config/rule_weights.h"
#include "domain/finger.h"

namespace piano_fingering::evaluator {

[[nodiscard]] config::FingerPair finger_pair_from(domain::Finger f1,
                                                  domain::Finger f2);

[[nodiscard]] double apply_cascading_penalty(
    const config::FingerPairDistances& distances, int actual_distance,
    const config::RuleWeights& weights);

[[nodiscard]] double apply_chord_penalty(
    const config::FingerPairDistances& distances, int actual_distance,
    const config::RuleWeights& weights);

}  // namespace piano_fingering::evaluator

#endif  // PIANO_FINGERING_EVALUATOR_RULES_H_
