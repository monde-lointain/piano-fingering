#include "domain/pitch.h"

#include <gtest/gtest.h>

#include <sstream>

namespace piano_fingering::domain {
namespace {

TEST(PitchTest, ConstructValid) {
  EXPECT_NO_THROW(Pitch(0));
  EXPECT_NO_THROW(Pitch(13));
  EXPECT_NO_THROW(Pitch(7));
}

TEST(PitchTest, ConstructInvalid) {
  EXPECT_THROW({ [[maybe_unused]] Pitch p(-1); }, std::invalid_argument);
  EXPECT_THROW({ [[maybe_unused]] Pitch p(14); }, std::invalid_argument);
}

TEST(PitchTest, ValueAccessor) {
  Pitch p(5);
  EXPECT_EQ(p.value(), 5);
}

TEST(PitchTest, BlackKeysAreCorrect) {
  // Black keys: {1, 3, 7, 9, 11} (C#, D#, F#, G#, A#)
  EXPECT_TRUE(Pitch(1).is_black_key());   // C#
  EXPECT_TRUE(Pitch(3).is_black_key());   // D#
  EXPECT_TRUE(Pitch(7).is_black_key());   // F#
  EXPECT_TRUE(Pitch(9).is_black_key());   // G#
  EXPECT_TRUE(Pitch(11).is_black_key());  // A#
}

TEST(PitchTest, WhiteKeysAreNotBlack) {
  EXPECT_FALSE(Pitch(0).is_black_key());   // C
  EXPECT_FALSE(Pitch(2).is_black_key());   // D
  EXPECT_FALSE(Pitch(4).is_black_key());   // E (imaginary gap at 5)
  EXPECT_FALSE(Pitch(6).is_black_key());   // F
  EXPECT_FALSE(Pitch(8).is_black_key());   // G
  EXPECT_FALSE(Pitch(10).is_black_key());  // A
  EXPECT_FALSE(Pitch(12).is_black_key());  // B
  EXPECT_FALSE(Pitch(13).is_black_key());  // imaginary gap
}

TEST(PitchTest, DistanceSameNote) {
  Pitch p(5);
  EXPECT_EQ(p.distance_to(Pitch(5)), 0);
}

TEST(PitchTest, DistanceDifferentNotes) {
  Pitch p1(2);
  Pitch p2(10);
  EXPECT_EQ(p1.distance_to(p2), 8);
  EXPECT_EQ(p2.distance_to(p1), 8);
}

TEST(PitchTest, Comparison) {
  Pitch p1(3);
  Pitch p2(7);
  Pitch p3(3);

  EXPECT_TRUE(p1 == p3);
  EXPECT_FALSE(p1 == p2);
  EXPECT_TRUE(p1 != p2);
  EXPECT_TRUE(p1 < p2);
  EXPECT_TRUE(p2 > p1);
  EXPECT_TRUE(p1 <= p3);
  EXPECT_TRUE(p1 >= p3);
}

TEST(PitchTest, StreamOutput) {
  std::ostringstream oss;
  oss << Pitch(7);
  EXPECT_EQ(oss.str(), "Pitch(7)");
}

}  // namespace
}  // namespace piano_fingering::domain
