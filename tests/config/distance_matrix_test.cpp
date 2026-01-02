#include "config/distance_matrix.h"
#include <gtest/gtest.h>

namespace piano_fingering::config {
namespace {

TEST(DistanceMatrixTest, HasTenFingerPairs) {
  DistanceMatrix matrix{};
  EXPECT_EQ(matrix.finger_pairs.size(), kFingerPairCount);
}

TEST(DistanceMatrixTest, GetPairReturnsCorrectElement) {
  DistanceMatrix matrix{};
  matrix.finger_pairs[static_cast<size_t>(FingerPair::kThumbIndex)] =
      {-5, -3, -1, 1, 3, 5};
  const auto& pair = matrix.get_pair(FingerPair::kThumbIndex);
  EXPECT_EQ(pair.min_prac, -5);
}

TEST(DistanceMatrixTest, IsValidReturnsTrueWhenAllPairsValid) {
  DistanceMatrix matrix{};
  for (auto& pair : matrix.finger_pairs) {
    pair = {-5, -3, -1, 1, 3, 5};
  }
  EXPECT_TRUE(matrix.is_valid());
}

TEST(DistanceMatrixTest, IsValidReturnsFalseWhenAnyPairInvalid) {
  DistanceMatrix matrix{};
  for (auto& pair : matrix.finger_pairs) {
    pair = {-5, -3, -1, 1, 3, 5};
  }
  matrix.finger_pairs[0] = {5, -3, -1, 1, 3, 5};  // Invalid
  EXPECT_FALSE(matrix.is_valid());
}

TEST(DistanceMatrixTest, EqualityOperator) {
  DistanceMatrix a{}, b{};
  for (size_t i = 0; i < kFingerPairCount; ++i) {
    a.finger_pairs[i] = {-5, -3, -1, 1, 3, 5};
    b.finger_pairs[i] = {-5, -3, -1, 1, 3, 5};
  }
  EXPECT_EQ(a, b);
}

}  // namespace
}  // namespace piano_fingering::config
