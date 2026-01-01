#include "domain/note.h"

#include <gtest/gtest.h>

#include <sstream>

namespace piano_fingering::domain {
namespace {

TEST(NoteTest, ConstructValid) {
  EXPECT_NO_THROW(Note(Pitch(0), 4, 240, false, 1, 1));
  EXPECT_NO_THROW(Note(Pitch(13), 10, 1, false, 2, 4));
}

TEST(NoteTest, ConstructInvalidOctave) {
  EXPECT_THROW(
      { [[maybe_unused]] Note n(Pitch(0), -1, 240, false, 1, 1); },
      std::invalid_argument);
  EXPECT_THROW(
      { [[maybe_unused]] Note n(Pitch(0), 11, 240, false, 1, 1); },
      std::invalid_argument);
}

TEST(NoteTest, ConstructInvalidDuration) {
  EXPECT_THROW(
      { [[maybe_unused]] Note n(Pitch(0), 4, 0, false, 1, 1); },
      std::invalid_argument);
}

TEST(NoteTest, ConstructInvalidStaff) {
  EXPECT_THROW(
      { [[maybe_unused]] Note n(Pitch(0), 4, 240, false, 0, 1); },
      std::invalid_argument);
  EXPECT_THROW(
      { [[maybe_unused]] Note n(Pitch(0), 4, 240, false, 3, 1); },
      std::invalid_argument);
}

TEST(NoteTest, ConstructInvalidVoice) {
  EXPECT_THROW(
      { [[maybe_unused]] Note n(Pitch(0), 4, 240, false, 1, 0); },
      std::invalid_argument);
  EXPECT_THROW(
      { [[maybe_unused]] Note n(Pitch(0), 4, 240, false, 1, 5); },
      std::invalid_argument);
}

TEST(NoteTest, Accessors) {
  Note n(Pitch(7), 5, 480, true, 2, 3);
  EXPECT_EQ(n.pitch().value(), 7);
  EXPECT_EQ(n.octave(), 5);
  EXPECT_EQ(n.duration(), 480);
  EXPECT_TRUE(n.is_rest());
  EXPECT_EQ(n.staff(), 2);
  EXPECT_EQ(n.voice(), 3);
}

TEST(NoteTest, AbsolutePitchCalculation) {
  Note n1(Pitch(0), 0, 240, false, 1, 1);  // C0 -> 0*14 + 0 = 0
  EXPECT_EQ(n1.absolute_pitch(), 0);

  Note n2(Pitch(13), 0, 240, false, 1, 1);  // Gap0 -> 0*14 + 13 = 13
  EXPECT_EQ(n2.absolute_pitch(), 13);

  Note n3(Pitch(0), 1, 240, false, 1, 1);  // C1 -> 1*14 + 0 = 14
  EXPECT_EQ(n3.absolute_pitch(), 14);

  Note n4(Pitch(7), 4, 240, false, 1, 1);  // F#4 -> 4*14 + 7 = 63
  EXPECT_EQ(n4.absolute_pitch(), 63);
}

TEST(NoteTest, ComparisonByAbsolutePitch) {
  Note n1(Pitch(7), 3, 240, false, 1, 1);   // abs = 3*14 + 7 = 49
  Note n2(Pitch(10), 4, 480, false, 2, 2);  // abs = 4*14 + 10 = 66
  Note n3(Pitch(7), 3, 100, true, 1, 3);    // abs = 3*14 + 7 = 49

  EXPECT_TRUE(n1 == n3);
  EXPECT_FALSE(n1 == n2);
  EXPECT_TRUE(n1 != n2);
  EXPECT_TRUE(n1 < n2);
  EXPECT_TRUE(n2 > n1);
  EXPECT_TRUE(n1 <= n3);
  EXPECT_TRUE(n1 >= n3);
}

TEST(NoteTest, StreamOutput) {
  std::ostringstream oss;
  Note n(Pitch(7), 4, 240, false, 1, 1);
  oss << n;
  EXPECT_EQ(oss.str(),
            "Note(Pitch(7), oct=4, dur=240, rest=0, staff=1, "
            "voice=1, abs=63)");
}

}  // namespace
}  // namespace piano_fingering::domain
