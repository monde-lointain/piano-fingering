#include "evaluator/score_evaluator.h"

#include <algorithm>
#include <vector>

#include "config/finger_pair_distances.h"
#include "domain/finger.h"
#include "domain/hand.h"
#include "domain/note.h"
#include "evaluator/rules.h"

namespace piano_fingering::evaluator {
namespace {

// Helper struct to hold note information
struct NoteInfo {
  domain::Finger finger;
  int pitch;
  bool is_black;
};

// Extract non-rest notes with fingerings from measures
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::vector<NoteInfo> collect_notes(
    const std::vector<domain::Measure>& measures,
    const std::vector<domain::Fingering>& fingerings) {
  std::vector<NoteInfo> notes;
  size_t fingering_idx = 0;

  for (const auto& measure : measures) {
    for (const auto& slice : measure) {
      if (slice.empty()) {
        continue;
      }

      bool has_non_rest =
          std::any_of(slice.begin(), slice.end(),
                      [](const auto& note) { return !note.is_rest(); });
      if (!has_non_rest) {
        continue;
      }

      if (fingering_idx >= fingerings.size()) {
        break;
      }

      const auto& slice_fingering = fingerings[fingering_idx];
      size_t note_idx = 0;
      for (const auto& note : slice) {
        if (!note.is_rest()) {
          if (note_idx < slice_fingering.size()) {
            const auto& finger_opt = slice_fingering[note_idx];
            if (finger_opt.has_value()) {
              notes.push_back({*finger_opt, note.absolute_pitch(),
                               note.pitch().is_black_key()});
            }
          }
          ++note_idx;
        }
      }

      ++fingering_idx;
    }
  }

  return notes;
}

// Process chord slice: compute penalties for all note pairs in a chord
double process_chord_slice(const domain::Slice& slice,
                           const domain::Fingering& chord_fingering,
                           const config::DistanceMatrix& distances,
                           const config::RuleWeights& weights) {
  std::vector<NoteInfo> chord_notes;
  size_t note_idx = 0;
  for (const auto& note : slice) {
    if (!note.is_rest()) {
      if (note_idx < chord_fingering.size()) {
        const auto& finger_opt = chord_fingering[note_idx];
        if (finger_opt.has_value()) {
          chord_notes.push_back({*finger_opt, note.absolute_pitch(),
                                 note.pitch().is_black_key()});
        }
      }
      ++note_idx;
    }
  }

  double penalty = 0.0;
  for (size_t j = 0; j < chord_notes.size(); ++j) {
    for (size_t k = j + 1; k < chord_notes.size(); ++k) {
      const auto& cn1 = chord_notes[j];
      const auto& cn2 = chord_notes[k];
      const auto& pair_distances =
          distances.get_pair(finger_pair_from(cn1.finger, cn2.finger));
      int actual_distance = cn2.pitch - cn1.pitch;
      penalty += apply_chord_penalty(pair_distances, actual_distance, weights);
    }
  }

  return penalty;
}

// Apply chord penalties (Rule 14)
double apply_chord_penalties(const std::vector<domain::Measure>& measures,
                             const std::vector<domain::Fingering>& fingerings,
                             const config::DistanceMatrix& distances,
                             const config::RuleWeights& weights) {
  double penalty = 0.0;
  size_t fingering_idx = 0;

  for (const auto& measure : measures) {
    for (const auto& slice : measure) {
      if (slice.empty()) {
        continue;
      }

      bool has_non_rest =
          std::any_of(slice.begin(), slice.end(),
                      [](const auto& note) { return !note.is_rest(); });
      if (!has_non_rest) {
        continue;
      }

      if (fingering_idx >= fingerings.size()) {
        break;
      }

      if (slice.size() > 1) {
        penalty += process_chord_slice(slice, fingerings[fingering_idx],
                                       distances, weights);
      }

      ++fingering_idx;
    }
  }

  return penalty;
}

// Compute Rule 11 parameters from two notes
struct Rule11Params {
  int lower_pitch;
  int higher_pitch;
  bool lower_black;
  bool higher_black;
  domain::Finger lower_finger;
  domain::Finger higher_finger;
};

Rule11Params compute_rule11_params(const NoteInfo& n1, const NoteInfo& n2) {
  if (n1.pitch < n2.pitch) {
    return {n1.pitch, n2.pitch, n1.is_black, n2.is_black, n1.finger, n2.finger};
  }
  return {n2.pitch, n1.pitch, n2.is_black, n1.is_black, n2.finger, n1.finger};
}

// Apply two-note rules between consecutive notes
double apply_two_note_rules(const std::vector<NoteInfo>& notes, size_t i,
                            const config::DistanceMatrix& distances,
                            const config::RuleWeights& weights,
                            domain::Hand hand) {
  if (i + 1 >= notes.size()) {
    return 0.0;
  }

  const auto& n1 = notes[i];
  const auto& n2 = notes[i + 1];
  double penalty = 0.0;

  penalty += apply_rule_5(n1.finger);
  if (i + 1 == notes.size() - 1) {
    penalty += apply_rule_5(n2.finger);
  }

  penalty += apply_rule_6(n1.finger, n2.finger);
  penalty += apply_rule_7(n1.finger, n1.is_black, n2.finger, n2.is_black);

  std::optional<bool> prev_black =
      (i > 0) ? std::optional<bool>(notes[i - 1].is_black) : std::nullopt;
  std::optional<bool> next_black =
      (i + 1 < notes.size()) ? std::optional<bool>(n2.is_black) : std::nullopt;
  penalty += apply_rule_8(n1.finger, n1.is_black, prev_black, next_black);

  penalty += apply_rule_9(n1.finger, n1.is_black, n2.is_black);
  penalty += apply_rule_9(n2.finger, n2.is_black, n1.is_black);

  bool crossing = is_crossing(n1.finger, n1.pitch, n2.finger, n2.pitch, hand);
  penalty += apply_rule_10(crossing, n1.is_black, n2.is_black);

  auto params = compute_rule11_params(n1, n2);
  penalty += apply_rule_11(params.lower_pitch, params.lower_black,
                           params.lower_finger, params.higher_pitch,
                           params.higher_black, params.higher_finger);

  const auto& pair_distances =
      distances.get_pair(finger_pair_from(n1.finger, n2.finger));
  int actual_distance = n2.pitch - n1.pitch;
  penalty += apply_cascading_penalty(pair_distances, actual_distance, weights);

  return penalty;
}

// Apply three-note rules for triplets
double apply_three_note_rules(const std::vector<NoteInfo>& notes, size_t i,
                              const config::DistanceMatrix& distances) {
  if (i + 2 >= notes.size()) {
    return 0.0;
  }

  const auto& n1 = notes[i];
  const auto& n2 = notes[i + 1];
  const auto& n3 = notes[i + 2];
  double penalty = 0.0;

  const auto& pair_distances =
      distances.get_pair(finger_pair_from(n1.finger, n2.finger));
  penalty += apply_rule_3(pair_distances, n1.pitch, n2.pitch, n3.pitch,
                          n1.finger, n2.finger, n3.finger);

  int span = n3.pitch - n1.pitch;
  const auto& span_distances =
      distances.get_pair(finger_pair_from(n1.finger, n3.finger));
  penalty += apply_rule_4(span_distances, span);

  penalty += apply_rule_12(n1.pitch, n2.pitch, n3.pitch, n1.finger, n2.finger,
                           n3.finger);

  penalty += apply_rule_15(n1.finger, n2.finger, n1.pitch, n2.pitch);

  return penalty;
}

}  // namespace

double ScoreEvaluator::evaluate(
    const domain::Piece& piece,
    const std::vector<domain::Fingering>& fingerings, domain::Hand hand) const {
  const auto& distances =
      (hand == domain::Hand::kLeft) ? config_->left_hand : config_->right_hand;
  const auto& weights = config_->weights;
  const auto& measures =
      (hand == domain::Hand::kLeft) ? piece.left_hand() : piece.right_hand();

  auto notes = collect_notes(measures, fingerings);

  if (notes.size() <= 1) {
    return 0.0;
  }

  double total_penalty = 0.0;

  for (size_t i = 0; i < notes.size(); ++i) {
    total_penalty += apply_two_note_rules(notes, i, distances, weights, hand);
    total_penalty += apply_three_note_rules(notes, i, distances);
  }

  total_penalty +=
      apply_chord_penalties(measures, fingerings, distances, weights);

  return total_penalty;
}

double ScoreEvaluator::evaluate_delta(
    const domain::Piece& piece,
    const std::vector<domain::Fingering>& current_fingerings,
    const std::vector<domain::Fingering>& proposed_fingerings,
    size_t changed_note_idx,
    domain::Hand hand) const {
  const auto& distances =
      (hand == domain::Hand::kLeft) ? config_->left_hand : config_->right_hand;
  const auto& weights = config_->weights;
  const auto& measures =
      (hand == domain::Hand::kLeft) ? piece.left_hand() : piece.right_hand();

  // Collect note info for both fingerings
  auto old_notes = collect_notes(measures, current_fingerings);
  auto new_notes = collect_notes(measures, proposed_fingerings);

  if (old_notes.size() != new_notes.size() || old_notes.empty()) {
    // Fallback to full evaluation if structure differs
    double new_score = evaluate(piece, proposed_fingerings, hand);
    double old_score = evaluate(piece, current_fingerings, hand);
    return new_score - old_score;
  }

  if (changed_note_idx >= old_notes.size()) {
    return 0.0;  // Invalid index
  }

  double delta = 0.0;

  // Compute old penalties for affected rules
  double old_penalty = 0.0;
  double new_penalty = 0.0;

  // Helper to apply two-note rules between notes at indices i and i+1
  auto apply_pair_rules = [&](const std::vector<NoteInfo>& notes, size_t i,
                              bool include_rule5_n1, bool include_rule5_n2) {
    if (i + 1 >= notes.size()) {
      return 0.0;
    }
    const auto& n1 = notes[i];
    const auto& n2 = notes[i + 1];
    double penalty = 0.0;

    // Rule 5
    if (include_rule5_n1) {
      penalty += apply_rule_5(n1.finger);
    }
    if (include_rule5_n2) {
      penalty += apply_rule_5(n2.finger);
    }

    // Rules 6-11, cascading penalty
    penalty += apply_rule_6(n1.finger, n2.finger);
    penalty += apply_rule_7(n1.finger, n1.is_black, n2.finger, n2.is_black);

    std::optional<bool> prev_black =
        (i > 0) ? std::optional<bool>(notes[i - 1].is_black) : std::nullopt;
    std::optional<bool> next_black =
        (i + 1 < notes.size()) ? std::optional<bool>(n2.is_black) : std::nullopt;
    penalty += apply_rule_8(n1.finger, n1.is_black, prev_black, next_black);

    penalty += apply_rule_9(n1.finger, n1.is_black, n2.is_black);
    penalty += apply_rule_9(n2.finger, n2.is_black, n1.is_black);

    bool crossing = is_crossing(n1.finger, n1.pitch, n2.finger, n2.pitch, hand);
    penalty += apply_rule_10(crossing, n1.is_black, n2.is_black);

    auto params = compute_rule11_params(n1, n2);
    penalty += apply_rule_11(params.lower_pitch, params.lower_black,
                             params.lower_finger, params.higher_pitch,
                             params.higher_black, params.higher_finger);

    const auto& pair_distances =
        distances.get_pair(finger_pair_from(n1.finger, n2.finger));
    int actual_distance = n2.pitch - n1.pitch;
    penalty += apply_cascading_penalty(pair_distances, actual_distance, weights);

    return penalty;
  };

  // Rule 5: apply to changed note
  old_penalty += apply_rule_5(old_notes[changed_note_idx].finger);
  new_penalty += apply_rule_5(new_notes[changed_note_idx].finger);

  // For the pair [i-1, i] (prev note to changed note):
  if (changed_note_idx > 0) {
    size_t i = changed_note_idx - 1;
    // Don't apply Rule 5 (already applied separately above)
    old_penalty += apply_pair_rules(old_notes, i, false, false);
    new_penalty += apply_pair_rules(new_notes, i, false, false);
  }

  // For the pair [i, i+1] (changed note to next note):
  if (changed_note_idx + 1 < old_notes.size()) {
    size_t i = changed_note_idx;
    // Don't apply Rule 5 to changed note (already applied)
    // Don't apply Rule 5 to next note (unchanged, so penalty unchanged)
    old_penalty += apply_pair_rules(old_notes, i, false, false);
    new_penalty += apply_pair_rules(new_notes, i, false, false);
  }

  // Three-note rules: check triplets involving changed note
  // Triplet [i-1, i, i+1]
  if (changed_note_idx > 0 && changed_note_idx + 1 < old_notes.size()) {
    size_t i = changed_note_idx - 1;
    old_penalty += apply_three_note_rules(old_notes, i, distances);
    new_penalty += apply_three_note_rules(new_notes, i, distances);
  }

  // Triplet [i, i+1, i+2]
  if (changed_note_idx + 2 < old_notes.size()) {
    size_t i = changed_note_idx;
    old_penalty += apply_three_note_rules(old_notes, i, distances);
    new_penalty += apply_three_note_rules(new_notes, i, distances);
  }

  // Triplet [i-2, i-1, i]
  if (changed_note_idx >= 2) {
    size_t i = changed_note_idx - 2;
    old_penalty += apply_three_note_rules(old_notes, i, distances);
    new_penalty += apply_three_note_rules(new_notes, i, distances);
  }

  // Chord rules (Rule 14): if changed slice is a chord
  // Need to find which slice corresponds to changed_note_idx
  size_t fingering_idx = 0;
  size_t note_count = 0;
  for (const auto& measure : measures) {
    for (const auto& slice : measure) {
      if (slice.empty()) {
        continue;
      }

      bool has_non_rest =
          std::any_of(slice.begin(), slice.end(),
                      [](const auto& note) { return !note.is_rest(); });
      if (!has_non_rest) {
        continue;
      }

      if (fingering_idx >= current_fingerings.size()) {
        break;
      }

      // Count notes in this slice
      size_t slice_note_count = 0;
      for (const auto& note : slice) {
        if (!note.is_rest()) {
          ++slice_note_count;
        }
      }

      // Check if changed_note_idx falls within this slice
      if (changed_note_idx >= note_count &&
          changed_note_idx < note_count + slice_note_count) {
        // This slice contains the changed note
        if (slice.size() > 1) {
          // It's a chord, recompute penalties
          old_penalty += process_chord_slice(slice, current_fingerings[fingering_idx],
                                             distances, weights);
          new_penalty += process_chord_slice(slice, proposed_fingerings[fingering_idx],
                                             distances, weights);
        }
        break;
      }

      note_count += slice_note_count;
      ++fingering_idx;
    }
  }

  delta = new_penalty - old_penalty;
  return delta;
}

}  // namespace piano_fingering::evaluator
