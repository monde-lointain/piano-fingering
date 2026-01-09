#include "evaluator/rules.h"

#include <gtest/gtest.h>

#include <optional>

#include "config/finger_pair.h"
#include "config/finger_pair_distances.h"
#include "config/rule_weights.h"
#include "domain/finger.h"
#include "domain/hand.h"

namespace piano_fingering::evaluator {
namespace {

using config::FingerPair;
using config::FingerPairDistances;
using config::RuleWeights;
using domain::Finger;
using domain::Hand;

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

TEST(RulesTest, Rule5FourthFingerPenalty) {
  EXPECT_DOUBLE_EQ(apply_rule_5(Finger::kRing), 1.0);
  EXPECT_DOUBLE_EQ(apply_rule_5(Finger::kThumb), 0.0);
  EXPECT_DOUBLE_EQ(apply_rule_5(Finger::kPinky), 0.0);
}

TEST(RulesTest, Rule6ThirdFourthConsecutive) {
  EXPECT_DOUBLE_EQ(apply_rule_6(Finger::kMiddle, Finger::kRing), 1.0);
  EXPECT_DOUBLE_EQ(apply_rule_6(Finger::kRing, Finger::kMiddle), 1.0);
  EXPECT_DOUBLE_EQ(apply_rule_6(Finger::kThumb, Finger::kIndex), 0.0);
}

TEST(RulesTest, Rule7ThirdWhiteFourthBlack) {
  EXPECT_DOUBLE_EQ(apply_rule_7(Finger::kMiddle, false, Finger::kRing, true),
                   1.0);
  EXPECT_DOUBLE_EQ(apply_rule_7(Finger::kRing, true, Finger::kMiddle, false),
                   1.0);
  EXPECT_DOUBLE_EQ(apply_rule_7(Finger::kMiddle, false, Finger::kRing, false),
                   0.0);
  EXPECT_DOUBLE_EQ(apply_rule_7(Finger::kThumb, false, Finger::kIndex, true),
                   0.0);
}

TEST(RulesTest, Rule8ThumbOnBlack) {
  EXPECT_DOUBLE_EQ(
      apply_rule_8(Finger::kThumb, true, std::nullopt, std::nullopt), 0.5);
  EXPECT_DOUBLE_EQ(
      apply_rule_8(Finger::kThumb, false, std::nullopt, std::nullopt), 0.0);
}

TEST(RulesTest, Rule8AdjacentWhite) {
  // Check BOTH prev and next (per confirmed design)
  EXPECT_DOUBLE_EQ(apply_rule_8(Finger::kThumb, true, false, std::nullopt),
                   1.5);
  EXPECT_DOUBLE_EQ(apply_rule_8(Finger::kThumb, true, std::nullopt, false),
                   1.5);
  EXPECT_DOUBLE_EQ(apply_rule_8(Finger::kThumb, true, false, false),
                   2.5);  // Both adjacent white
  EXPECT_DOUBLE_EQ(apply_rule_8(Finger::kThumb, true, true, std::nullopt), 0.5);
}

TEST(RulesTest, Rule9FifthOnBlackAdjacentWhite) {
  EXPECT_DOUBLE_EQ(apply_rule_9(Finger::kPinky, true, false), 1.0);
  EXPECT_DOUBLE_EQ(apply_rule_9(Finger::kPinky, true, true), 0.0);
  EXPECT_DOUBLE_EQ(apply_rule_9(Finger::kPinky, false, false), 0.0);
}

TEST(RulesTest, IsCrossingRightHand) {
  // Right hand: thumb higher than non-thumb = crossing
  EXPECT_TRUE(
      is_crossing(Finger::kThumb, 65, Finger::kIndex, 60, Hand::kRight));
  EXPECT_FALSE(
      is_crossing(Finger::kThumb, 60, Finger::kIndex, 65, Hand::kRight));
  // Both non-thumb: no crossing
  EXPECT_FALSE(
      is_crossing(Finger::kIndex, 60, Finger::kMiddle, 65, Hand::kRight));
}

TEST(RulesTest, IsCrossingLeftHand) {
  // Left hand: thumb lower than non-thumb = crossing
  EXPECT_TRUE(is_crossing(Finger::kThumb, 60, Finger::kIndex, 65, Hand::kLeft));
  EXPECT_FALSE(
      is_crossing(Finger::kThumb, 65, Finger::kIndex, 60, Hand::kLeft));
}

TEST(RulesTest, Rule10CrossingSameLevel) {
  // Both white = same level
  EXPECT_DOUBLE_EQ(apply_rule_10(true, false, false), 1.0);
  // Both black = same level
  EXPECT_DOUBLE_EQ(apply_rule_10(true, true, true), 1.0);
  // Different levels
  EXPECT_DOUBLE_EQ(apply_rule_10(true, false, true), 0.0);
  // Not crossing
  EXPECT_DOUBLE_EQ(apply_rule_10(false, false, false), 0.0);
}

TEST(RulesTest, Rule11ThumbBlackNonThumbWhite) {
  // Lower pitch white (non-thumb), higher pitch black (thumb) = Rule 11
  // Table 2 spec: Score = +2
  EXPECT_DOUBLE_EQ(
      apply_rule_11(60, false, Finger::kIndex, 65, true, Finger::kThumb), 2.0);
  // Thumb white: no penalty
  EXPECT_DOUBLE_EQ(
      apply_rule_11(60, false, Finger::kIndex, 65, false, Finger::kThumb), 0.0);
  // Non-thumb black: no penalty
  EXPECT_DOUBLE_EQ(
      apply_rule_11(60, true, Finger::kIndex, 65, true, Finger::kThumb), 0.0);
}

TEST(RulesTest, IsMonotonicAscending) {
  EXPECT_TRUE(is_monotonic(60, 62, 64));   // p1 < p2 < p3
  EXPECT_FALSE(is_monotonic(60, 64, 62));  // p2 not between
  EXPECT_FALSE(is_monotonic(60, 60, 64));  // p1 == p2
}

TEST(RulesTest, IsMonotonicDescending) {
  EXPECT_TRUE(is_monotonic(64, 62, 60));   // p1 > p2 > p3
  EXPECT_FALSE(is_monotonic(64, 60, 62));  // p2 not between
}

TEST(RulesTest, Rule3BaseOnly) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  // Span 9 exceeds MaxComf(8) but within MaxPrac(10), not monotonic
  // Base penalty only: +1
  EXPECT_DOUBLE_EQ(apply_rule_3(d, 60, 65, 69, Finger::kIndex, Finger::kThumb,
                                Finger::kMiddle),
                   1.0);
}

TEST(RulesTest, Rule3FullChange) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  // Span 12 exceeds MaxPrac(10), monotonic (60 < 64 < 72), f2 = thumb
  // Base (+1) + Full change (+1) = 2
  EXPECT_DOUBLE_EQ(apply_rule_3(d, 60, 64, 72, Finger::kIndex, Finger::kThumb,
                                Finger::kMiddle),
                   2.0);
}

TEST(RulesTest, Rule3HalfChangeNotThumb) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  // Span 12 exceeds MaxPrac, monotonic, but f2 != thumb
  // Base (+1) only (not full change)
  EXPECT_DOUBLE_EQ(apply_rule_3(d, 60, 64, 72, Finger::kIndex, Finger::kMiddle,
                                Finger::kPinky),
                   1.0);
}

TEST(RulesTest, Rule3Substitution) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  // Span 0 (same pitch), different fingers: Substitution (+1)
  // No base penalty (within comfort)
  EXPECT_DOUBLE_EQ(apply_rule_3(d, 60, 64, 60, Finger::kIndex, Finger::kThumb,
                                Finger::kMiddle),
                   1.0);
}

TEST(RulesTest, Rule3BaseAndSubstitution) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  // p1 == p3 but span calculation may still exceed comfort in edge cases
  // This tests same pitch with different fingers
  EXPECT_DOUBLE_EQ(apply_rule_3(d, 60, 64, 60, Finger::kThumb, Finger::kIndex,
                                Finger::kMiddle),
                   1.0);
}

TEST(RulesTest, Rule3NoPenalty) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  // Span 4 within comfort, same finger f1==f3, not substitution
  EXPECT_DOUBLE_EQ(apply_rule_3(d, 60, 62, 64, Finger::kThumb, Finger::kIndex,
                                Finger::kThumb),
                   0.0);
}

TEST(RulesTest, Rule4TripletSpanExceedsComfort) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  EXPECT_DOUBLE_EQ(apply_rule_4(d, 5), 0.0);   // Within comfort
  EXPECT_DOUBLE_EQ(apply_rule_4(d, 9), 1.0);   // Exceeds max_comf by 1
  EXPECT_DOUBLE_EQ(apply_rule_4(d, 12), 4.0);  // Exceeds max_comf by 4
  EXPECT_DOUBLE_EQ(apply_rule_4(d, -9), 3.0);  // Below min_comf (-6) by 3
  EXPECT_DOUBLE_EQ(apply_rule_4(d, -5), 0.0);  // Within comfort (negative)
}

TEST(RulesTest, Rule4TripletSpanContracted) {
  FingerPairDistances d{-8, -6, 1, 5, 8, 10};
  EXPECT_DOUBLE_EQ(apply_rule_4(d, -6), 0.0);   // At min_comf boundary
  EXPECT_DOUBLE_EQ(apply_rule_4(d, -7), 1.0);   // Below by 1: -6 - (-7) = 1
  EXPECT_DOUBLE_EQ(apply_rule_4(d, -10), 4.0);  // Below by 4: -6 - (-10) = 4
}

TEST(RulesTest, Rule12SameFingerReuse) {
  // Rule 12: Different first and third note, same finger, second pitch is
  // middle
  EXPECT_DOUBLE_EQ(
      apply_rule_12(60, 64, 68, Finger::kIndex, Finger::kThumb, Finger::kIndex),
      1.0);
  // Same pitch: no penalty
  EXPECT_DOUBLE_EQ(
      apply_rule_12(60, 64, 60, Finger::kIndex, Finger::kThumb, Finger::kIndex),
      0.0);
  // Different fingers: no penalty
  EXPECT_DOUBLE_EQ(apply_rule_12(60, 64, 68, Finger::kIndex, Finger::kThumb,
                                 Finger::kMiddle),
                   0.0);
  // Not monotonic: no penalty
  EXPECT_DOUBLE_EQ(
      apply_rule_12(60, 70, 65, Finger::kIndex, Finger::kThumb, Finger::kIndex),
      0.0);
}

TEST(RulesTest, Rule15SamePitchDifferentFinger) {
  EXPECT_DOUBLE_EQ(apply_rule_15(Finger::kThumb, Finger::kIndex, 60, 60), 1.0);
  EXPECT_DOUBLE_EQ(apply_rule_15(Finger::kThumb, Finger::kThumb, 60, 60), 0.0);
  EXPECT_DOUBLE_EQ(apply_rule_15(Finger::kThumb, Finger::kIndex, 60, 62), 0.0);
}

}  // namespace
}  // namespace piano_fingering::evaluator
