#ifndef PIANO_FINGERING_EVALUATOR_RULES_H_
#define PIANO_FINGERING_EVALUATOR_RULES_H_

#include "config/finger_pair.h"
#include "domain/finger.h"

namespace piano_fingering::evaluator {

[[nodiscard]] config::FingerPair finger_pair_from(domain::Finger f1,
                                                  domain::Finger f2);

}  // namespace piano_fingering::evaluator

#endif  // PIANO_FINGERING_EVALUATOR_RULES_H_
