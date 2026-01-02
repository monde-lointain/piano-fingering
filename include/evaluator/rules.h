#ifndef PIANO_FINGERING_EVALUATOR_RULES_H_
#define PIANO_FINGERING_EVALUATOR_RULES_H_

#include <optional>

#include "config/finger_pair.h"
#include "config/finger_pair_distances.h"
#include "config/rule_weights.h"
#include "domain/finger.h"
#include "domain/hand.h"

namespace piano_fingering::evaluator {

[[nodiscard]] config::FingerPair finger_pair_from(domain::Finger f1,
                                                  domain::Finger f2);

[[nodiscard]] double apply_cascading_penalty(
    const config::FingerPairDistances& distances, int actual_distance,
    const config::RuleWeights& weights);

[[nodiscard]] double apply_chord_penalty(
    const config::FingerPairDistances& distances, int actual_distance,
    const config::RuleWeights& weights);

[[nodiscard]] double apply_rule_5(domain::Finger f);
[[nodiscard]] double apply_rule_6(domain::Finger f1, domain::Finger f2);
[[nodiscard]] double apply_rule_7(domain::Finger f1, bool is_black1,
                                  domain::Finger f2, bool is_black2);

[[nodiscard]] double apply_rule_8(domain::Finger f, bool is_black,
                                  std::optional<bool> prev_is_black,
                                  std::optional<bool> next_is_black);
[[nodiscard]] double apply_rule_9(domain::Finger f, bool is_black,
                                  bool adj_is_black);
[[nodiscard]] bool is_crossing(domain::Finger f1, int pitch1, domain::Finger f2,
                               int pitch2, domain::Hand hand);
[[nodiscard]] double apply_rule_10(bool is_crossing, bool note1_black,
                                   bool note2_black);
[[nodiscard]] double apply_rule_11(int lower_pitch, bool lower_black,
                                   domain::Finger lower_finger,
                                   int higher_pitch, bool higher_black,
                                   domain::Finger higher_finger);

[[nodiscard]] bool is_monotonic(int p1, int p2, int p3);

[[nodiscard]] double apply_rule_3(const config::FingerPairDistances& d, int p1,
                                  int p2, int p3, domain::Finger f1,
                                  domain::Finger f2, domain::Finger f3);

[[nodiscard]] double apply_rule_4(const config::FingerPairDistances& d,
                                  int span);

[[nodiscard]] double apply_rule_12(int p1, int p2, int p3, domain::Finger f1,
                                   domain::Finger f2, domain::Finger f3);

[[nodiscard]] double apply_rule_15(domain::Finger f1, domain::Finger f2,
                                   int pitch1, int pitch2);

}  // namespace piano_fingering::evaluator

#endif  // PIANO_FINGERING_EVALUATOR_RULES_H_
