#include "evaluator/rules.h"

#include <gtest/gtest.h>

#include "config/finger_pair.h"
#include "domain/finger.h"

namespace piano_fingering::evaluator {
namespace {

using config::FingerPair;
using domain::Finger;

TEST(RulesTest, FingerPairFromAscending) {
  EXPECT_EQ(finger_pair_from(Finger::kThumb, Finger::kIndex),
            FingerPair::kThumbIndex);
  EXPECT_EQ(finger_pair_from(Finger::kMiddle, Finger::kRing),
            FingerPair::kMiddleRing);
  EXPECT_EQ(finger_pair_from(Finger::kRing, Finger::kPinky),
            FingerPair::kRingPinky);
}

TEST(RulesTest, FingerPairFromDescending) {
  EXPECT_EQ(finger_pair_from(Finger::kIndex, Finger::kThumb),
            FingerPair::kThumbIndex);
  EXPECT_EQ(finger_pair_from(Finger::kPinky, Finger::kThumb),
            FingerPair::kThumbPinky);
}

}  // namespace
}  // namespace piano_fingering::evaluator
