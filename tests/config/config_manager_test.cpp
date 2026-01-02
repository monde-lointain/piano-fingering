#include "config/config_manager.h"
#include <gtest/gtest.h>
#include "config/configuration_error.h"
#include "config/preset.h"

namespace piano_fingering::config {
namespace {

TEST(ConfigManagerTest, LoadPresetMedium) {
  Config cfg = ConfigManager::load_preset(kPresetMedium);
  EXPECT_TRUE(cfg.is_valid());
  EXPECT_EQ(cfg.left_hand, get_medium_preset().left_hand);
}

TEST(ConfigManagerTest, LoadPresetSmall) {
  Config cfg = ConfigManager::load_preset(kPresetSmall);
  EXPECT_TRUE(cfg.is_valid());
}

TEST(ConfigManagerTest, LoadPresetLarge) {
  Config cfg = ConfigManager::load_preset(kPresetLarge);
  EXPECT_TRUE(cfg.is_valid());
}

TEST(ConfigManagerTest, LoadPresetThrowsForUnknown) {
  EXPECT_THROW({
    [[maybe_unused]] auto cfg = ConfigManager::load_preset("Unknown");
  }, ConfigurationError);
}

TEST(ConfigManagerTest, LoadPresetCaseInsensitive) {
  Config cfg1 = ConfigManager::load_preset("small");
  Config cfg2 = ConfigManager::load_preset("SMALL");
  EXPECT_EQ(cfg1.left_hand, cfg2.left_hand);
}

TEST(ConfigManagerTest, ValidateReturnsTrueForValidConfig) {
  Config cfg = ConfigManager::load_preset(kPresetMedium);
  std::string error;
  EXPECT_TRUE(ConfigManager::validate(cfg, error));
  EXPECT_TRUE(error.empty());
}

TEST(ConfigManagerTest, ValidateReturnsFalseForInvalidLeftHand) {
  Config cfg = ConfigManager::load_preset(kPresetMedium);
  cfg.left_hand.finger_pairs[0].min_prac = 100;
  std::string error;
  EXPECT_FALSE(ConfigManager::validate(cfg, error));
  EXPECT_FALSE(error.empty());
}

TEST(ConfigManagerTest, ValidateReturnsFalseForInvalidWeights) {
  Config cfg = ConfigManager::load_preset(kPresetMedium);
  cfg.weights.values[0] = -1.0;
  std::string error;
  EXPECT_FALSE(ConfigManager::validate(cfg, error));
}

}  // namespace
}  // namespace piano_fingering::config
