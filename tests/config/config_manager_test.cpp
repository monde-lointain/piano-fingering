#include "config/config_manager.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

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
  EXPECT_THROW(
      { [[maybe_unused]] auto cfg = ConfigManager::load_preset("Unknown"); },
      ConfigurationError);
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

class ConfigManagerJsonTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_dir_ = std::filesystem::temp_directory_path() / "config_test";
    std::filesystem::create_directories(test_dir_);
  }

  void TearDown() override { std::filesystem::remove_all(test_dir_); }

  void write_json(const std::string& filename, const std::string& content) {
    std::ofstream file(test_dir_ / filename);
    file << content;
  }

  std::filesystem::path test_dir_;
};

TEST_F(ConfigManagerJsonTest, LoadCustomWithEmptyJsonUsesBasePreset) {
  write_json("empty.json", "{}");
  Config cfg = ConfigManager::load_custom(test_dir_ / "empty.json", "Small");
  Config expected = ConfigManager::load_preset("Small");
  EXPECT_EQ(cfg.left_hand, expected.left_hand);
}

TEST_F(ConfigManagerJsonTest, LoadCustomOverridesAlgorithmParameters) {
  write_json("algo.json", R"({
    "algorithm": {
      "beam_width": 200,
      "ils_iterations": 500
    }
  })");
  Config cfg = ConfigManager::load_custom(test_dir_ / "algo.json");
  EXPECT_EQ(cfg.algorithm.beam_width, 200);
  EXPECT_EQ(cfg.algorithm.ils_iterations, 500);
  EXPECT_EQ(cfg.algorithm.perturbation_strength, 3);
}

TEST_F(ConfigManagerJsonTest, LoadCustomOverridesRuleWeights) {
  write_json("weights.json", R"({
    "rule_weights": [2.5, null, 3.0]
  })");
  Config cfg = ConfigManager::load_custom(test_dir_ / "weights.json");
  EXPECT_DOUBLE_EQ(cfg.weights.values[0], 2.5);
  EXPECT_DOUBLE_EQ(cfg.weights.values[1], 1.0);  // null keeps default
  EXPECT_DOUBLE_EQ(cfg.weights.values[2], 3.0);
}

TEST_F(ConfigManagerJsonTest, LoadCustomOverridesDistanceMatrix) {
  write_json("distance.json", R"({
    "distance_matrix": {
      "right_hand": {
        "1-2": { "MinPrac": -10, "MaxPrac": 12 }
      }
    }
  })");
  Config cfg = ConfigManager::load_custom(test_dir_ / "distance.json");
  const auto& thumb_index = cfg.right_hand.get_pair(FingerPair::kThumbIndex);
  EXPECT_EQ(thumb_index.min_prac, -10);
  EXPECT_EQ(thumb_index.max_prac, 12);
}

TEST_F(ConfigManagerJsonTest, LoadCustomThrowsForInvalidJson) {
  write_json("invalid.json", "{ not valid }");
  EXPECT_THROW(
      {
        [[maybe_unused]] auto cfg =
            ConfigManager::load_custom(test_dir_ / "invalid.json");
      },
      ConfigurationError);
}

TEST_F(ConfigManagerJsonTest, LoadCustomThrowsForNonexistentFile) {
  EXPECT_THROW(
      {
        [[maybe_unused]] auto cfg =
            ConfigManager::load_custom(test_dir_ / "nope.json");
      },
      ConfigurationError);
}

TEST_F(ConfigManagerJsonTest, LoadCustomThrowsForInvalidConfig) {
  write_json("bad.json", R"({
    "distance_matrix": {
      "right_hand": { "1-2": { "MinPrac": 100 } }
    }
  })");
  EXPECT_THROW(
      {
        [[maybe_unused]] auto cfg =
            ConfigManager::load_custom(test_dir_ / "bad.json");
      },
      ConfigurationError);
}

}  // namespace
}  // namespace piano_fingering::config
