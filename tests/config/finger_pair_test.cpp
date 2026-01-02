#include "config/finger_pair.h"

#include <gtest/gtest.h>

namespace piano_fingering::config {
namespace {

TEST(FingerPairTest, HasTenValues) {
  EXPECT_EQ(static_cast<int>(FingerPair::kThumbIndex), 0);
  EXPECT_EQ(static_cast<int>(FingerPair::kRingPinky), 9);
}

TEST(FingerPairTest, CountConstant) { EXPECT_EQ(kFingerPairCount, 10); }

}  // namespace
}  // namespace piano_fingering::config
