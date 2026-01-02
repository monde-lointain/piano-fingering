#include "evaluator/rules.h"

#include <gtest/gtest.h>

#include "config/finger_pair.h"
#include "config/finger_pair_distances.h"
#include "config/rule_weights.h"
#include "domain/finger.h"

namespace piano_fingering::evaluator {
namespace {

using config::FingerPair;
using config::FingerPairDistances;
using config::RuleWeights;
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

TEST(RulesTest, CascadingPenaltyWithinRelaxedRange) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};  // Medium 1-2
  RuleWeights w = RuleWeights::defaults();
  EXPECT_DOUBLE_EQ(apply_cascading_penalty(d, 3, w), 0.0);  // Within relaxed
}

TEST(RulesTest, CascadingPenaltyBeyondRelaxed) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  RuleWeights w = RuleWeights::defaults();
  // Distance 6 exceeds MaxRel(5) by 1 unit -> Rule 2: 1*1.0 = 1.0
  EXPECT_DOUBLE_EQ(apply_cascading_penalty(d, 6, w), 1.0);
}

TEST(RulesTest, CascadingPenaltyBeyondComfort) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  RuleWeights w = RuleWeights::defaults();
  // Distance 9 exceeds MaxComf(8) by 1, MaxRel(5) by 4
  // Rule 2: 4*1.0 = 4.0, Rule 1: 1*2.0 = 2.0 -> Total 6.0
  EXPECT_DOUBLE_EQ(apply_cascading_penalty(d, 9, w), 6.0);
}

TEST(RulesTest, CascadingPenaltyBeyondPractical) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  RuleWeights w = RuleWeights::defaults();
  // Distance 12 exceeds MaxPrac(10) by 2, MaxComf(8) by 4, MaxRel(5) by 7
  // Rule 2: 7*1.0=7, Rule 1: 4*2.0=8, Rule 13: 2*10.0=20 -> Total 35.0
  EXPECT_DOUBLE_EQ(apply_cascading_penalty(d, 12, w), 35.0);
}

TEST(RulesTest, CascadingPenaltyNegativeDistance) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  RuleWeights w = RuleWeights::defaults();
  // Distance -10 below MinPrac(-8) by 2, MinComf(-6) by 4, MinRel(1) by 11
  // Rule 2: 11*1.0=11, Rule 1: 4*2.0=8, Rule 13: 2*10.0=20 -> Total 39.0
  EXPECT_DOUBLE_EQ(apply_cascading_penalty(d, -10, w), 39.0);
}

TEST(RulesTest, Rule14DoublesRules1And2) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  RuleWeights w = RuleWeights::defaults();
  // Distance 9: Rule 2 violation=4, Rule 1 violation=1
  // Chord: Rule 2 doubled: 4*2*1.0=8, Rule 1 doubled: 1*2*2.0=4 -> Total 12.0
  EXPECT_DOUBLE_EQ(apply_chord_penalty(d, 9, w), 12.0);
}

TEST(RulesTest, Rule14DoesNotDoubleRule13) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  RuleWeights w = RuleWeights::defaults();
  // Distance 12: prac_viol=2, comf_viol=4, rel_viol=7
  // Chord: 7*2*1.0 + 4*2*2.0 + 2*10.0 = 14 + 16 + 20 = 50.0
  EXPECT_DOUBLE_EQ(apply_chord_penalty(d, 12, w), 50.0);
}

}  // namespace
}  // namespace piano_fingering::evaluator
