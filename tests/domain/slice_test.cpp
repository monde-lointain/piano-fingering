#include "domain/slice.h"

#include <gtest/gtest.h>

#include <sstream>

namespace piano_fingering::domain {
namespace {

TEST(SliceTest, ConstructEmpty) {
  EXPECT_NO_THROW(Slice());
  Slice s;
  EXPECT_EQ(s.size(), 0);
  EXPECT_TRUE(s.empty());
}

TEST(SliceTest, ConstructWithNotes) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Note n2(Pitch(7), 4, 240, false, 1, 1);
  EXPECT_NO_THROW(Slice({n1, n2}));
  Slice s({n1, n2});
  EXPECT_EQ(s.size(), 2);
  EXPECT_FALSE(s.empty());
}

TEST(SliceTest, ConstructTooManyNotes) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Note n2(Pitch(2), 4, 240, false, 1, 1);
  Note n3(Pitch(4), 4, 240, false, 1, 1);
  Note n4(Pitch(6), 4, 240, false, 1, 1);
  Note n5(Pitch(8), 4, 240, false, 1, 1);
  Note n6(Pitch(10), 4, 240, false, 1, 1);
  EXPECT_THROW(
      { [[maybe_unused]] Slice s({n1, n2, n3, n4, n5, n6}); },
      std::invalid_argument);
}

TEST(SliceTest, NotesSortedByAbsolutePitch) {
  Note n1(Pitch(7), 4, 240, false, 1, 1);  // abs = 4*14 + 7 = 63
  Note n2(Pitch(0), 5, 240, false, 1, 1);  // abs = 5*14 + 0 = 70
  Note n3(Pitch(2), 3, 240, false, 1, 1);  // abs = 3*14 + 2 = 44
  Slice s({n1, n2, n3});

  EXPECT_EQ(s[0].absolute_pitch(), 44);
  EXPECT_EQ(s[1].absolute_pitch(), 63);
  EXPECT_EQ(s[2].absolute_pitch(), 70);
}

TEST(SliceTest, ConstAccess) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  const Slice s({n1});
  EXPECT_EQ(s[0].pitch().value(), 0);
}

TEST(SliceTest, AccessOutOfBounds) {
  Slice s;
  EXPECT_THROW({ [[maybe_unused]] auto n = s[0]; }, std::out_of_range);

  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s2({n1});
  EXPECT_THROW({ [[maybe_unused]] auto n = s2[1]; }, std::out_of_range);
}

TEST(SliceTest, Iteration) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Note n2(Pitch(7), 4, 240, false, 1, 1);
  Slice s({n1, n2});

  int count = 0;
  for (const auto& note : s) {
    ++count;
    [[maybe_unused]] int val = note.pitch().value();
  }
  EXPECT_EQ(count, 2);
}

TEST(SliceTest, StreamOutput) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Note n2(Pitch(7), 4, 240, false, 1, 1);
  Slice s({n1, n2});

  std::ostringstream oss;
  oss << s;
  std::string output = oss.str();
  EXPECT_TRUE(output.find("Slice") != std::string::npos);
  EXPECT_TRUE(output.find("2 notes") != std::string::npos);
}

}  // namespace
}  // namespace piano_fingering::domain
