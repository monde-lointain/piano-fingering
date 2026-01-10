#include "evaluator/rules.h"

#include <algorithm>
#include <array>
#include <optional>

namespace piano_fingering::evaluator {

using config::FingerPair;
using domain::Finger;
using domain::Hand;
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

  // Nested cascading structure: relaxed -> comfort -> practical
  if (actual_distance < distances.min_rel) {
    // Rule 2: relaxed minimum violation
    penalty += (distances.min_rel - actual_distance) *
               weights[config::RuleIndex::kRelaxedDistance];
    if (actual_distance < distances.min_comf) {
      // Rule 1: comfort minimum violation (only if relaxed violated)
      penalty += (distances.min_comf - actual_distance) *
                 weights[config::RuleIndex::kComfortDistance];
      if (actual_distance < distances.min_prac) {
        // Rule 13: practical minimum violation (only if comfort violated)
        penalty += (distances.min_prac - actual_distance) *
                   weights[config::RuleIndex::kPracticalDistance];
      }
    }
  } else if (actual_distance > distances.max_rel) {
    // Rule 2: relaxed maximum violation
    penalty += (actual_distance - distances.max_rel) *
               weights[config::RuleIndex::kRelaxedDistance];
    if (actual_distance > distances.max_comf) {
      // Rule 1: comfort maximum violation (only if relaxed violated)
      penalty += (actual_distance - distances.max_comf) *
                 weights[config::RuleIndex::kComfortDistance];
      if (actual_distance > distances.max_prac) {
        // Rule 13: practical maximum violation (only if comfort violated)
        penalty += (actual_distance - distances.max_prac) *
                   weights[config::RuleIndex::kPracticalDistance];
      }
    }
  }

  return penalty;
}

double apply_chord_penalty(const config::FingerPairDistances& distances,
                           int actual_distance,
                           const config::RuleWeights& weights) {
  double penalty = 0.0;

  // Nested cascading structure: relaxed -> comfort -> practical
  if (actual_distance < distances.min_rel) {
    // Rule 2: relaxed minimum violation (doubled for chords)
    penalty += (distances.min_rel - actual_distance) * 2.0 *
               weights[config::RuleIndex::kRelaxedDistance];
    if (actual_distance < distances.min_comf) {
      // Rule 1: comfort minimum violation (doubled, only if relaxed violated)
      penalty += (distances.min_comf - actual_distance) * 2.0 *
                 weights[config::RuleIndex::kComfortDistance];
      if (actual_distance < distances.min_prac) {
        // Rule 13: practical minimum violation (NOT doubled, only if comfort
        // violated)
        penalty += (distances.min_prac - actual_distance) *
                   weights[config::RuleIndex::kPracticalDistance];
      }
    }
  } else if (actual_distance > distances.max_rel) {
    // Rule 2: relaxed maximum violation (doubled for chords)
    penalty += (actual_distance - distances.max_rel) * 2.0 *
               weights[config::RuleIndex::kRelaxedDistance];
    if (actual_distance > distances.max_comf) {
      // Rule 1: comfort maximum violation (doubled, only if relaxed violated)
      penalty += (actual_distance - distances.max_comf) * 2.0 *
                 weights[config::RuleIndex::kComfortDistance];
      if (actual_distance > distances.max_prac) {
        // Rule 13: practical maximum violation (NOT doubled, only if comfort
        // violated)
        penalty += (actual_distance - distances.max_prac) *
                   weights[config::RuleIndex::kPracticalDistance];
      }
    }
  }

  return penalty;
}

double apply_rule_5(Finger f) { return (f == Finger::kRing) ? 1.0 : 0.0; }

double apply_rule_6(Finger f1, Finger f2) {
  bool has_middle = (f1 == Finger::kMiddle || f2 == Finger::kMiddle);
  bool has_ring = (f1 == Finger::kRing || f2 == Finger::kRing);
  return (has_middle && has_ring) ? 1.0 : 0.0;
}

double apply_rule_7(Finger f1, bool is_black1, Finger f2, bool is_black2) {
  bool middle_on_white = (f1 == Finger::kMiddle && !is_black1) ||
                         (f2 == Finger::kMiddle && !is_black2);
  bool ring_on_black =
      (f1 == Finger::kRing && is_black1) || (f2 == Finger::kRing && is_black2);
  return (middle_on_white && ring_on_black) ? 1.0 : 0.0;
}

double apply_rule_8(Finger f, bool is_black, std::optional<bool> prev_is_black,
                    std::optional<bool> next_is_black) {
  if (f != Finger::kThumb || !is_black) {
    return 0.0;
  }
  double penalty = 0.5;
  if (prev_is_black.has_value() && !*prev_is_black) {
    penalty += 1.0;
  }
  if (next_is_black.has_value() && !*next_is_black) {
    penalty += 1.0;
  }
  return penalty;
}

double apply_rule_9(Finger f, bool is_black, bool adj_is_black) {
  if (f != Finger::kPinky || !is_black) {
    return 0.0;
  }
  return adj_is_black ? 0.0 : 1.0;
}

bool is_crossing(Finger f1, int pitch1, Finger f2, int pitch2, Hand hand) {
  bool f1_is_thumb = (f1 == Finger::kThumb);
  bool f2_is_thumb = (f2 == Finger::kThumb);
  // Must have exactly one thumb
  if (f1_is_thumb == f2_is_thumb) {
    return false;
  }

  int thumb_pitch = f1_is_thumb ? pitch1 : pitch2;
  int other_pitch = f1_is_thumb ? pitch2 : pitch1;

  if (hand == Hand::kRight) {
    return thumb_pitch > other_pitch;  // Thumb higher = crossing
  }
  return thumb_pitch < other_pitch;  // Thumb lower = crossing
}

double apply_rule_10(bool is_crossing, bool note1_black, bool note2_black) {
  if (!is_crossing) {
    return 0.0;
  }
  return (note1_black == note2_black) ? 1.0 : 0.0;
}

double apply_rule_11([[maybe_unused]] int lower_pitch, bool lower_black,
                     Finger lower_finger, [[maybe_unused]] int higher_pitch,
                     bool higher_black, Finger higher_finger) {
  // Rule 11: lower note white (non-thumb), higher note black (thumb)
  // Table 2: Score = +2 for this violation
  bool lower_is_non_thumb = (lower_finger != Finger::kThumb);
  bool higher_is_thumb = (higher_finger == Finger::kThumb);
  if (!lower_is_non_thumb || !higher_is_thumb) {
    return 0.0;
  }
  return (!lower_black && higher_black) ? 2.0 : 0.0;
}

bool is_monotonic(int p1, int p2, int p3) {
  // p2 strictly between p1 and p3
  return (p1 < p2 && p2 < p3) || (p1 > p2 && p2 > p3);
}

double apply_rule_3(const config::FingerPairDistances& d, int p1, int p2,
                    int p3, Finger f1, Finger f2, Finger f3) {
  double penalty = 0.0;
  int span = p3 - p1;  // Signed distance (can be negative)

  // 1. Base penalty: span outside comfort range
  bool outside_comfort = (span < d.min_comf || span > d.max_comf);
  if (outside_comfort) {
    penalty += 1.0;
  }

  // 2. Full change penalty: monotonic + thumb pivot + outside practical
  bool outside_practical = (span < d.min_prac || span > d.max_prac);
  if (is_monotonic(p1, p2, p3) && f2 == Finger::kThumb && outside_practical) {
    penalty += 1.0;
  }

  // 3. Substitution penalty: same pitch, different finger
  if (p1 == p3 && f1 != f3) {
    penalty += 1.0;
  }

  return penalty;
}

double apply_rule_4(const config::FingerPairDistances& d, int span) {
  if (span < d.min_comf) {
    return static_cast<double>(d.min_comf - span);
  }
  if (span > d.max_comf) {
    return static_cast<double>(span - d.max_comf);
  }
  return 0.0;
}

double apply_rule_12(int p1, int p2, int p3, Finger f1,
                     [[maybe_unused]] Finger f2, Finger f3) {
  // Different first and third note, played by same finger, second pitch is
  // middle
  bool different_pitches = (p1 != p3);
  bool same_outer_finger = (f1 == f3);
  bool monotonic = is_monotonic(p1, p2, p3);
  return (different_pitches && same_outer_finger && monotonic) ? 1.0 : 0.0;
}

double apply_rule_15(Finger f1, Finger f2, int pitch1, int pitch2) {
  return (f1 != f2 && pitch1 == pitch2) ? 1.0 : 0.0;
}

}  // namespace piano_fingering::evaluator
