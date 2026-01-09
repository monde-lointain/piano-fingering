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


// Helper to compute fingering index from slice location - O(S)
// Counts number of non-empty, non-rest slices before the target
size_t compute_fingering_idx(
    const std::vector<domain::Measure>& measures,
    size_t target_measure_idx,
    size_t target_slice_idx) {
  size_t fingering_idx = 0;

  for (size_t m_idx = 0; m_idx < measures.size() && m_idx <= target_measure_idx; ++m_idx) {
    const auto& measure = measures[m_idx];
    size_t slice_limit = (m_idx == target_measure_idx) ? target_slice_idx : measure.size();

    for (size_t s_idx = 0; s_idx < slice_limit; ++s_idx) {
      const auto& slice = measure[s_idx];
      if (slice.empty()) {
        continue;
      }

      bool has_non_rest = std::any_of(
          slice.begin(), slice.end(),
          [](const auto& note) { return !note.is_rest(); });

      if (has_non_rest) {
        ++fingering_idx;
      }
    }
  }

  return fingering_idx;
}

// Get NoteInfo at a specific slice location - O(1) access to slice, O(N) to find note
std::optional<NoteInfo> get_note_at_location(
    const std::vector<domain::Measure>& measures,
    const std::vector<domain::Fingering>& fingerings,
    const ScoreEvaluator::SliceLocation& location) {
  if (location.measure_idx >= measures.size()) {
    return std::nullopt;
  }

  const auto& measure = measures[location.measure_idx];
  if (location.slice_idx >= measure.size()) {
    return std::nullopt;
  }

  const auto& slice = measure[location.slice_idx];
  if (slice.empty()) {
    return std::nullopt;
  }

  // Compute fingering index for this slice
  size_t fingering_idx = compute_fingering_idx(measures, location.measure_idx, location.slice_idx);
  if (fingering_idx >= fingerings.size()) {
    return std::nullopt;
  }

  const auto& slice_fingering = fingerings[fingering_idx];

  // Find the note at note_idx_in_slice (counting only non-rest notes)
  size_t non_rest_count = 0;
  for (const auto& note : slice) {
    if (!note.is_rest()) {
      if (non_rest_count == location.note_idx_in_slice) {
        if (non_rest_count < slice_fingering.size()) {
          const auto& finger_opt = slice_fingering[non_rest_count];
          if (finger_opt.has_value()) {
            return NoteInfo{*finger_opt, note.absolute_pitch(), note.pitch().is_black_key()};
          }
        }
        return std::nullopt;
      }
      ++non_rest_count;
    }
  }

  return std::nullopt;
}

// Find previous slice location - O(S) worst case
std::optional<ScoreEvaluator::SliceLocation> find_prev_slice(
    const std::vector<domain::Measure>& measures,
    size_t current_measure_idx,
    size_t current_slice_idx) {
  // Try previous slice in same measure
  if (current_slice_idx > 0) {
    for (size_t s_idx = current_slice_idx - 1; s_idx != SIZE_MAX; --s_idx) {
      const auto& slice = measures[current_measure_idx][s_idx];
      if (!slice.empty()) {
        bool has_non_rest = std::any_of(
            slice.begin(), slice.end(),
            [](const auto& note) { return !note.is_rest(); });
        if (has_non_rest) {
          // Count notes in this slice
          size_t note_count = 0;
          for (const auto& note : slice) {
            if (!note.is_rest()) {
              ++note_count;
            }
          }
          // Return location of last note in previous slice
          return ScoreEvaluator::SliceLocation{current_measure_idx, s_idx, note_count - 1};
        }
      }
      if (s_idx == 0) break;
    }
  }

  // Try previous measure
  if (current_measure_idx > 0) {
    for (size_t m_idx = current_measure_idx - 1; m_idx != SIZE_MAX; --m_idx) {
      const auto& measure = measures[m_idx];
      for (size_t s_idx = measure.size() - 1; s_idx != SIZE_MAX; --s_idx) {
        const auto& slice = measure[s_idx];
        if (!slice.empty()) {
          bool has_non_rest = std::any_of(
              slice.begin(), slice.end(),
              [](const auto& note) { return !note.is_rest(); });
          if (has_non_rest) {
            size_t note_count = 0;
            for (const auto& note : slice) {
              if (!note.is_rest()) {
                ++note_count;
              }
            }
            return ScoreEvaluator::SliceLocation{m_idx, s_idx, note_count - 1};
          }
        }
        if (s_idx == 0) break;
      }
      if (m_idx == 0) break;
    }
  }

  return std::nullopt;
}

// Find next slice location - O(S) worst case
std::optional<ScoreEvaluator::SliceLocation> find_next_slice(
    const std::vector<domain::Measure>& measures,
    size_t current_measure_idx,
    size_t current_slice_idx) {
  // Try next slice in same measure
  if (current_measure_idx < measures.size()) {
    const auto& current_measure = measures[current_measure_idx];
    for (size_t s_idx = current_slice_idx + 1; s_idx < current_measure.size(); ++s_idx) {
      const auto& slice = current_measure[s_idx];
      if (!slice.empty()) {
        bool has_non_rest = std::any_of(
            slice.begin(), slice.end(),
            [](const auto& note) { return !note.is_rest(); });
        if (has_non_rest) {
          return ScoreEvaluator::SliceLocation{current_measure_idx, s_idx, 0};
        }
      }
    }
  }

  // Try next measure
  for (size_t m_idx = current_measure_idx + 1; m_idx < measures.size(); ++m_idx) {
    const auto& measure = measures[m_idx];
    for (size_t s_idx = 0; s_idx < measure.size(); ++s_idx) {
      const auto& slice = measure[s_idx];
      if (!slice.empty()) {
        bool has_non_rest = std::any_of(
            slice.begin(), slice.end(),
            [](const auto& note) { return !note.is_rest(); });
        if (has_non_rest) {
          return ScoreEvaluator::SliceLocation{m_idx, s_idx, 0};
        }
      }
    }
  }

  return std::nullopt;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
double ScoreEvaluator::evaluate_delta(
    const domain::Piece& piece,
    const std::vector<domain::Fingering>& current_fingerings,
    const std::vector<domain::Fingering>& proposed_fingerings,
    const SliceLocation& changed_location,
    domain::Hand hand) const {
  const auto& distances =
      (hand == domain::Hand::kLeft) ? config_->left_hand : config_->right_hand;
  const auto& weights = config_->weights;
  const auto& measures =
      (hand == domain::Hand::kLeft) ? piece.left_hand() : piece.right_hand();

  // Get the changed note info - O(1) access to slice
  auto old_changed_opt = get_note_at_location(measures, current_fingerings, changed_location);
  auto new_changed_opt = get_note_at_location(measures, proposed_fingerings, changed_location);

  if (!old_changed_opt.has_value() || !new_changed_opt.has_value()) {
    // Invalid location or missing fingering - fallback to full evaluation
    double new_score = evaluate(piece, proposed_fingerings, hand);
    double old_score = evaluate(piece, current_fingerings, hand);
    return new_score - old_score;
  }

  const auto& old_changed = *old_changed_opt;
  const auto& new_changed = *new_changed_opt;

  double delta = 0.0;
  double old_penalty = 0.0;
  double new_penalty = 0.0;

  // Rule 5: changed note only - O(1)
  old_penalty += apply_rule_5(old_changed.finger);
  new_penalty += apply_rule_5(new_changed.finger);

  // Helper to apply two-note rules between consecutive notes - O(1)
  auto apply_pair_penalties = [&](const NoteInfo& n1, const NoteInfo& n2,
                                  const NoteInfo* prev_note) {
    double penalty = 0.0;

    penalty += apply_rule_6(n1.finger, n2.finger);
    penalty += apply_rule_7(n1.finger, n1.is_black, n2.finger, n2.is_black);

    std::optional<bool> prev_black =
        prev_note ? std::optional<bool>(prev_note->is_black) : std::nullopt;
    std::optional<bool> next_black = n2.is_black;
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

  // Helper to apply three-note rules on a triplet - O(1)
  auto apply_triplet_penalties = [&](const NoteInfo& n1, const NoteInfo& n2,
                                      const NoteInfo& n3) {
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
  };

  // Get neighbor locations - O(S) worst case
  auto prev_loc_opt = find_prev_slice(measures, changed_location.measure_idx, changed_location.slice_idx);
  auto next_loc_opt = find_next_slice(measures, changed_location.measure_idx, changed_location.slice_idx);

  // Get neighbor notes
  auto prev_old_opt = prev_loc_opt ? get_note_at_location(measures, current_fingerings, *prev_loc_opt) : std::nullopt;
  auto prev_new_opt = prev_loc_opt ? get_note_at_location(measures, proposed_fingerings, *prev_loc_opt) : std::nullopt;
  auto next_old_opt = next_loc_opt ? get_note_at_location(measures, current_fingerings, *next_loc_opt) : std::nullopt;
  auto next_new_opt = next_loc_opt ? get_note_at_location(measures, proposed_fingerings, *next_loc_opt) : std::nullopt;

  // Two-note rules: [prev, changed] - O(1)
  if (prev_old_opt.has_value() && prev_new_opt.has_value()) {
    auto prev_prev_loc = prev_loc_opt ? find_prev_slice(measures, prev_loc_opt->measure_idx, prev_loc_opt->slice_idx) : std::nullopt;
    auto prev_prev_old = prev_prev_loc ? get_note_at_location(measures, current_fingerings, *prev_prev_loc) : std::nullopt;
    auto prev_prev_new = prev_prev_loc ? get_note_at_location(measures, proposed_fingerings, *prev_prev_loc) : std::nullopt;

    old_penalty += apply_pair_penalties(*prev_old_opt, old_changed,
                                        prev_prev_old.has_value() ? &(*prev_prev_old) : nullptr);
    new_penalty += apply_pair_penalties(*prev_new_opt, new_changed,
                                        prev_prev_new.has_value() ? &(*prev_prev_new) : nullptr);
  }

  // Two-note rules: [changed, next] - O(1)
  if (next_old_opt.has_value() && next_new_opt.has_value()) {
    old_penalty += apply_pair_penalties(old_changed, *next_old_opt,
                                        prev_old_opt.has_value() ? &(*prev_old_opt) : nullptr);
    new_penalty += apply_pair_penalties(new_changed, *next_new_opt,
                                        prev_new_opt.has_value() ? &(*prev_new_opt) : nullptr);
  }

  // Three-note rules: triplets involving changed note - O(1) each
  // Triplet [prev, changed, next]
  if (prev_old_opt.has_value() && next_old_opt.has_value() &&
      prev_new_opt.has_value() && next_new_opt.has_value()) {
    old_penalty += apply_triplet_penalties(*prev_old_opt, old_changed, *next_old_opt);
    new_penalty += apply_triplet_penalties(*prev_new_opt, new_changed, *next_new_opt);
  }

  // Triplet [changed, next, next+1]
  if (next_old_opt.has_value() && next_new_opt.has_value() && next_loc_opt.has_value()) {
    auto next2_loc = find_next_slice(measures, next_loc_opt->measure_idx, next_loc_opt->slice_idx);
    if (next2_loc.has_value()) {
      auto next2_old = get_note_at_location(measures, current_fingerings, *next2_loc);
      auto next2_new = get_note_at_location(measures, proposed_fingerings, *next2_loc);
      if (next2_old.has_value() && next2_new.has_value()) {
        old_penalty += apply_triplet_penalties(old_changed, *next_old_opt, *next2_old);
        new_penalty += apply_triplet_penalties(new_changed, *next_new_opt, *next2_new);
      }
    }
  }

  // Triplet [prev-1, prev, changed]
  if (prev_old_opt.has_value() && prev_new_opt.has_value() && prev_loc_opt.has_value()) {
    auto prev2_loc = find_prev_slice(measures, prev_loc_opt->measure_idx, prev_loc_opt->slice_idx);
    if (prev2_loc.has_value()) {
      auto prev2_old = get_note_at_location(measures, current_fingerings, *prev2_loc);
      auto prev2_new = get_note_at_location(measures, proposed_fingerings, *prev2_loc);
      if (prev2_old.has_value() && prev2_new.has_value()) {
        old_penalty += apply_triplet_penalties(*prev2_old, *prev_old_opt, old_changed);
        new_penalty += apply_triplet_penalties(*prev2_new, *prev_new_opt, new_changed);
      }
    }
  }

  // Chord rules (Rule 14): if changed slice is a chord - O(1) access + O(M²) processing
  if (changed_location.measure_idx < measures.size()) {
    const auto& measure = measures[changed_location.measure_idx];
    if (changed_location.slice_idx < measure.size()) {
      const auto& slice = measure[changed_location.slice_idx];
      if (slice.size() > 1) {
        // Compute fingering index for this slice - O(S)
        size_t fingering_idx = compute_fingering_idx(measures, changed_location.measure_idx, changed_location.slice_idx);

        if (fingering_idx < current_fingerings.size() &&
            fingering_idx < proposed_fingerings.size()) {
          old_penalty += process_chord_slice(slice,
                                             current_fingerings[fingering_idx],
                                             distances, weights);
          new_penalty += process_chord_slice(slice,
                                             proposed_fingerings[fingering_idx],
                                             distances, weights);
        }
      }
    }
  }

  delta = new_penalty - old_penalty;
  return delta;
}

}  // namespace piano_fingering::evaluator
