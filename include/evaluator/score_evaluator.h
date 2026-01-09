#ifndef PIANO_FINGERING_EVALUATOR_SCORE_EVALUATOR_H_
#define PIANO_FINGERING_EVALUATOR_SCORE_EVALUATOR_H_

#include <vector>

#include "config/config.h"
#include "domain/fingering.h"
#include "domain/hand.h"
#include "domain/piece.h"

namespace piano_fingering::evaluator {

class ScoreEvaluator {
 public:
  struct SliceLocation {
    size_t measure_idx;
    size_t slice_idx;
    size_t note_idx_in_slice;
  };

  explicit ScoreEvaluator(const config::Config& config) noexcept
      : config_(&config) {}

  [[nodiscard]] double evaluate(
      const domain::Piece& piece,
      const std::vector<domain::Fingering>& fingerings,
      domain::Hand hand) const;

  [[nodiscard]] double evaluate_delta(
      const domain::Piece& piece,
      const std::vector<domain::Fingering>& current_fingerings,
      const std::vector<domain::Fingering>& proposed_fingerings,
      const SliceLocation& changed_location,
      domain::Hand hand) const;

 private:
  const config::Config* config_;
};

}  // namespace piano_fingering::evaluator

#endif  // PIANO_FINGERING_EVALUATOR_SCORE_EVALUATOR_H_
