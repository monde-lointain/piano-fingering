#include "config/algorithm_parameters.h"
#include <gtest/gtest.h>

namespace piano_fingering::config {
namespace {

TEST(AlgorithmParametersTest, HasDefaultValues) {
  AlgorithmParameters params{};
  EXPECT_EQ(params.beam_width, 100);
  EXPECT_EQ(params.ils_iterations, 1000);
  EXPECT_EQ(params.perturbation_strength, 3);
}

TEST(AlgorithmParametersTest, IsValidReturnsTrueForPositiveValues) {
  AlgorithmParameters params{100, 1000, 3};
  EXPECT_TRUE(params.is_valid());
}

TEST(AlgorithmParametersTest, IsValidReturnsFalseForZeroBeamWidth) {
  AlgorithmParameters params{0, 1000, 3};
  EXPECT_FALSE(params.is_valid());
}

TEST(AlgorithmParametersTest, EqualityOperator) {
  AlgorithmParameters a{100, 1000, 3};
  AlgorithmParameters b{100, 1000, 3};
  EXPECT_EQ(a, b);
}

}  // namespace
}  // namespace piano_fingering::config
