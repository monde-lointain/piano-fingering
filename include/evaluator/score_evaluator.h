#ifndef PIANO_FINGERING_EVALUATOR_SCORE_EVALUATOR_H_
#define PIANO_FINGERING_EVALUATOR_SCORE_EVALUATOR_H_

#include "config/config.h"

namespace piano_fingering::evaluator {

class ScoreEvaluator {
 public:
  explicit ScoreEvaluator(const config::Config& config) noexcept
      : config_(&config) {}

 private:
  const config::Config* config_;
};

}  // namespace piano_fingering::evaluator

#endif  // PIANO_FINGERING_EVALUATOR_SCORE_EVALUATOR_H_
