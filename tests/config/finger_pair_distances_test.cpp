#include "config/finger_pair_distances.h"
#include <gtest/gtest.h>

namespace piano_fingering::config {
namespace {

TEST(FingerPairDistancesTest, ConstructsWithValidValues) {
  FingerPairDistances dist{-5, -3, -1, 1, 3, 5};
  EXPECT_EQ(dist.min_prac, -5);
  EXPECT_EQ(dist.max_prac, 5);
}

TEST(FingerPairDistancesTest, IsValidReturnsTrueForValidDistances) {
  FingerPairDistances dist{-5, -3, -1, 1, 3, 5};
  EXPECT_TRUE(dist.is_valid());
}

TEST(FingerPairDistancesTest, IsValidFailsWhenMinPracGreaterThanMinComf) {
  FingerPairDistances dist{0, -1, -2, 1, 2, 3};
  EXPECT_FALSE(dist.is_valid());
}

TEST(FingerPairDistancesTest, IsValidFailsWhenMinRelNotLessThanMaxRel) {
  FingerPairDistances dist{-5, -3, 1, 1, 3, 5};  // min_rel == max_rel
  EXPECT_FALSE(dist.is_valid());
}

TEST(FingerPairDistancesTest, IsValidFailsWhenValueOutOfRange) {
  FingerPairDistances dist{-21, -3, -1, 1, 3, 5};
  EXPECT_FALSE(dist.is_valid());
}

TEST(FingerPairDistancesTest, BoundaryValuesValid) {
  FingerPairDistances dist{-20, -20, -20, 20, 20, 20};
  EXPECT_TRUE(dist.is_valid());
}

TEST(FingerPairDistancesTest, EqualityOperator) {
  FingerPairDistances a{-5, -3, -1, 1, 3, 5};
  FingerPairDistances b{-5, -3, -1, 1, 3, 5};
  EXPECT_EQ(a, b);
}

}  // namespace
}  // namespace piano_fingering::config
