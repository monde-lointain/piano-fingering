#include "config/preset.h"

namespace piano_fingering::config {
namespace {

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
  // Left hand: same absolute distances, mirrored orientation
  return make_medium_right_hand();
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

}  // namespace

const Preset& get_small_preset() {
  static const Preset preset{std::string(kPresetSmall),
                             make_small_right_hand(),  // NOLINT
                             make_small_right_hand(), RuleWeights::defaults()};
  return preset;
}

const Preset& get_medium_preset() {
  static const Preset preset{std::string(kPresetMedium),  // NOLINT
                             make_medium_left_hand(), make_medium_right_hand(),
                             RuleWeights::defaults()};
  return preset;
}

const Preset& get_large_preset() {
  static const Preset preset{std::string(kPresetLarge),
                             make_large_right_hand(),  // NOLINT
                             make_large_right_hand(), RuleWeights::defaults()};
  return preset;
}

}  // namespace piano_fingering::config
