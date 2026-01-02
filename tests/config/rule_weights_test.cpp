#include "config/rule_weights.h"

#include <gtest/gtest.h>

namespace piano_fingering::config {
namespace {

TEST(RuleWeightsTest, Has15Weights) {
  RuleWeights weights{};
  EXPECT_EQ(weights.values.size(), kRuleCount);
}

TEST(RuleWeightsTest, IsValidReturnsTrueWhenAllNonNegative) {
  RuleWeights weights{};
  weights.values.fill(1.0);
  EXPECT_TRUE(weights.is_valid());
}

TEST(RuleWeightsTest, IsValidReturnsFalseForNegative) {
  RuleWeights weights{};
  weights.values[7] = -0.1;
  EXPECT_FALSE(weights.is_valid());
}

TEST(RuleWeightsTest, DefaultWeightsAreValid) {
  RuleWeights weights = RuleWeights::defaults();
  EXPECT_TRUE(weights.is_valid());
}

TEST(RuleWeightsTest, DefaultWeightsMatchSRS) {
  RuleWeights weights = RuleWeights::defaults();
  // From SRS Appendix A.2
  EXPECT_DOUBLE_EQ(weights.values[0], 2.0);    // Rule 1
  EXPECT_DOUBLE_EQ(weights.values[1], 1.0);    // Rule 2
  EXPECT_DOUBLE_EQ(weights.values[7], 0.5);    // Rule 8: thumb on black (base)
  EXPECT_DOUBLE_EQ(weights.values[12], 10.0);  // Rule 13
}

TEST(RuleWeightsTest, EqualityOperator) {
  RuleWeights a = RuleWeights::defaults();
  RuleWeights b = RuleWeights::defaults();
  EXPECT_EQ(a, b);
}

}  // namespace
}  // namespace piano_fingering::config
