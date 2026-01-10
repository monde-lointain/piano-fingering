#include "config/preset.h"

namespace piano_fingering::config {

// Mirror right hand to create left hand by swapping and negating min/max
DistanceMatrix mirror_to_left_hand(const DistanceMatrix& right) {
  DistanceMatrix left{};

  for (size_t i = 0; i < right.finger_pairs.size(); ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    const auto& r_pair = right.finger_pairs[i];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    auto& l_pair = left.finger_pairs[i];

    // Min and Max have to be interchanged and multiplied by -1
    // R(1-2) [-8, 10]  ->  Swap [10, -8]  ->  Negate [-10, 8]
    l_pair.min_prac = -r_pair.max_prac;
    l_pair.min_comf = -r_pair.max_comf;
    l_pair.min_rel = -r_pair.max_rel;
    l_pair.max_rel = -r_pair.min_rel;
    l_pair.max_comf = -r_pair.min_comf;
    l_pair.max_prac = -r_pair.min_prac;
  }
  return left;
}

// From SRS Appendix A.1 - Medium Hand (Default)
// Note: Left hand distances are mirrored (negated min/max)
DistanceMatrix make_medium_right_hand() {
  DistanceMatrix m{};
  m.finger_pairs[0] = {-8, -6, 1, 5, 8, 10};    // 1-2
  m.finger_pairs[1] = {-7, -5, 3, 9, 12, 14};   // 1-3
  m.finger_pairs[2] = {-5, -3, 5, 11, 13, 15};  // 1-4
  m.finger_pairs[3] = {-2, 0, 7, 12, 14, 16};   // 1-5
  m.finger_pairs[4] = {1, 1, 1, 2, 5, 7};       // 2-3
  m.finger_pairs[5] = {1, 1, 3, 4, 6, 8};       // 2-4
  m.finger_pairs[6] = {2, 2, 5, 6, 10, 12};     // 2-5
  m.finger_pairs[7] = {1, 1, 1, 2, 2, 4};       // 3-4
  m.finger_pairs[8] = {1, 1, 3, 4, 6, 8};       // 3-5
  m.finger_pairs[9] = {1, 1, 1, 2, 4, 6};       // 4-5
  return m;
}

DistanceMatrix make_medium_left_hand() {
  return mirror_to_left_hand(make_medium_right_hand());
}

// Small hand (exact values from spec)
DistanceMatrix make_small_right_hand() {
  DistanceMatrix m{};
  m.finger_pairs[0] = {-7, -5, 1, 3, 8, 10};   // 1-2
  m.finger_pairs[1] = {-6, -4, 3, 6, 10, 12};  // 1-3
  m.finger_pairs[2] = {-4, -2, 5, 8, 11, 13};  // 1-4
  m.finger_pairs[3] = {-2, 0, 7, 10, 12, 14};  // 1-5
  m.finger_pairs[4] = {1, 1, 1, 2, 4, 6};      // 2-3
  m.finger_pairs[5] = {1, 1, 3, 4, 6, 8};      // 2-4
  m.finger_pairs[6] = {2, 2, 5, 6, 8, 10};     // 2-5
  m.finger_pairs[7] = {1, 1, 1, 2, 2, 4};      // 3-4
  m.finger_pairs[8] = {1, 1, 3, 4, 6, 8};      // 3-5
  m.finger_pairs[9] = {1, 1, 1, 2, 4, 6};      // 4-5
  return m;
}

// Large hand (exact values from spec)
DistanceMatrix make_large_right_hand() {
  DistanceMatrix m{};
  m.finger_pairs[0] = {-10, -8, 1, 6, 9, 11};   // 1-2
  m.finger_pairs[1] = {-8, -6, 3, 9, 13, 15};   // 1-3
  m.finger_pairs[2] = {-6, -4, 5, 11, 14, 16};  // 1-4
  m.finger_pairs[3] = {-2, 0, 7, 12, 16, 18};   // 1-5
  m.finger_pairs[4] = {1, 1, 1, 2, 5, 7};       // 2-3
  m.finger_pairs[5] = {1, 1, 3, 4, 6, 8};       // 2-4
  m.finger_pairs[6] = {2, 2, 5, 6, 10, 12};     // 2-5
  m.finger_pairs[7] = {1, 1, 1, 2, 2, 4};       // 3-4
  m.finger_pairs[8] = {1, 1, 3, 4, 6, 8};       // 3-5
  m.finger_pairs[9] = {1, 1, 1, 2, 4, 6};       // 4-5
  return m;
}

const Preset& get_small_preset() {
  static const Preset kPreset{
      std::string(kPresetSmall),
      mirror_to_left_hand(make_small_right_hand()),  // NOLINT
      make_small_right_hand(), RuleWeights::defaults()};
  return kPreset;
}

const Preset& get_medium_preset() {
  static const Preset kPreset{std::string(kPresetMedium),  // NOLINT
                              make_medium_left_hand(), make_medium_right_hand(),
                              RuleWeights::defaults()};
  return kPreset;
}

const Preset& get_large_preset() {
  static const Preset kPreset{
      std::string(kPresetLarge),
      mirror_to_left_hand(make_large_right_hand()),  // NOLINT
      make_large_right_hand(), RuleWeights::defaults()};
  return kPreset;
}

}  // namespace piano_fingering::config
