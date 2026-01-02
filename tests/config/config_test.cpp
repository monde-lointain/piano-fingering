#include "config/config.h"
#include <gtest/gtest.h>

namespace piano_fingering::config {
namespace {

TEST(ConfigTest, HasAllMembers) {
  Config cfg{};
  [[maybe_unused]] auto& lh = cfg.left_hand;
  [[maybe_unused]] auto& rh = cfg.right_hand;
  [[maybe_unused]] auto& w = cfg.weights;
  [[maybe_unused]] auto& a = cfg.algorithm;
}

TEST(ConfigTest, IsValidWhenAllComponentsValid) {
  Config cfg{};
  for (auto& pair : cfg.left_hand.finger_pairs) {
    pair = {-5, -3, -1, 1, 3, 5};
  }
  for (auto& pair : cfg.right_hand.finger_pairs) {
    pair = {-5, -3, -1, 1, 3, 5};
  }
  cfg.weights = RuleWeights::defaults();
  EXPECT_TRUE(cfg.is_valid());
}

TEST(ConfigTest, IsValidFailsWithInvalidLeftHand) {
  Config cfg{};
  for (auto& pair : cfg.right_hand.finger_pairs) {
    pair = {-5, -3, -1, 1, 3, 5};
  }
  cfg.weights = RuleWeights::defaults();
  // left_hand invalid (default zeros)
  EXPECT_FALSE(cfg.is_valid());
}

}  // namespace
}  // namespace piano_fingering::config
