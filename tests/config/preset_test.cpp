#include "config/preset.h"

#include <gtest/gtest.h>

namespace piano_fingering::config {
namespace {

TEST(PresetTest, PresetNamesConstantsDefined) {
  EXPECT_EQ(kPresetSmall, "Small");
  EXPECT_EQ(kPresetMedium, "Medium");
  EXPECT_EQ(kPresetLarge, "Large");
}

TEST(PresetTest, ToConfigConvertsCorrectly) {
  const Preset& preset = get_medium_preset();
  Config cfg = preset.to_config();
  EXPECT_EQ(cfg.left_hand, preset.left_hand);
  EXPECT_EQ(cfg.right_hand, preset.right_hand);
  EXPECT_EQ(cfg.weights, preset.weights);
}

TEST(PresetTest, MediumPresetIsValid) {
  const Preset& preset = get_medium_preset();
  EXPECT_EQ(preset.name, kPresetMedium);
  EXPECT_TRUE(preset.left_hand.is_valid());
  EXPECT_TRUE(preset.right_hand.is_valid());
  EXPECT_TRUE(preset.weights.is_valid());
}

TEST(PresetTest, MediumPresetMatchesSRS) {
  const Preset& preset = get_medium_preset();
  // From SRS Appendix A.1 - Right Hand 1-2 pair
  const auto& thumb_index = preset.right_hand.get_pair(FingerPair::kThumbIndex);
  EXPECT_EQ(thumb_index.min_prac, -8);
  EXPECT_EQ(thumb_index.min_comf, -6);
  EXPECT_EQ(thumb_index.min_rel, 1);
  EXPECT_EQ(thumb_index.max_rel, 5);
  EXPECT_EQ(thumb_index.max_comf, 8);
  EXPECT_EQ(thumb_index.max_prac, 10);
}

TEST(PresetTest, SmallPresetIsValid) {
  const Preset& preset = get_small_preset();
  EXPECT_TRUE(preset.to_config().is_valid());
}

TEST(PresetTest, LargePresetIsValid) {
  const Preset& preset = get_large_preset();
  EXPECT_TRUE(preset.to_config().is_valid());
}

TEST(PresetTest, PresetsHaveDifferentDistances) {
  const Preset& small = get_small_preset();
  const Preset& large = get_large_preset();
  EXPECT_NE(small.right_hand, large.right_hand);
}

}  // namespace
}  // namespace piano_fingering::config
