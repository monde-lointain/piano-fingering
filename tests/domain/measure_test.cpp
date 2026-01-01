#include "domain/measure.h"

#include <gtest/gtest.h>

#include <sstream>

namespace piano_fingering::domain {
namespace {

TEST(MeasureTest, ConstructValid) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);

  EXPECT_NO_THROW(Measure(1, {s1}, ts));
  EXPECT_NO_THROW(Measure(100, {s1, s1}, ts));
}

TEST(MeasureTest, ConstructInvalidNumber) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);

  EXPECT_THROW(
      { [[maybe_unused]] Measure m(0, {s1}, ts); }, std::invalid_argument);
}

TEST(MeasureTest, ConstructEmptySlices) {
  TimeSignature ts(4, 4);
  EXPECT_THROW(
      { [[maybe_unused]] Measure m(1, {}, ts); }, std::invalid_argument);
}

TEST(MeasureTest, Accessors) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Note n2(Pitch(7), 4, 240, false, 1, 1);
  Slice s1({n1});
  Slice s2({n2});
  TimeSignature ts(3, 4);

  Measure m(42, {s1, s2}, ts);

  EXPECT_EQ(m.number(), 42);
  EXPECT_EQ(m.size(), 2);
  EXPECT_EQ(m.time_signature().numerator(), 3);
  EXPECT_EQ(m.time_signature().denominator(), 4);
}

TEST(MeasureTest, SliceAccess) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);
  Measure m(1, {s1}, ts);

  EXPECT_EQ(m[0].size(), 1);
  EXPECT_EQ(m[0][0].pitch().value(), 0);
}

TEST(MeasureTest, ConstSliceAccess) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);
  const Measure m(1, {s1}, ts);

  EXPECT_EQ(m[0].size(), 1);
}

TEST(MeasureTest, AccessOutOfBounds) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);
  Measure m(1, {s1}, ts);

  EXPECT_THROW({ [[maybe_unused]] auto s = m[1]; }, std::out_of_range);
}

TEST(MeasureTest, Iteration) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);
  Measure m(1, {s1, s1, s1}, ts);

  int count = 0;
  for (const auto& slice : m) {
    ++count;
    [[maybe_unused]] size_t sz = slice.size();
  }
  EXPECT_EQ(count, 3);
}

TEST(MeasureTest, StreamOutput) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);
  Measure m(1, {s1}, ts);

  std::ostringstream oss;
  oss << m;
  std::string output = oss.str();
  EXPECT_TRUE(output.find("Measure") != std::string::npos);
  EXPECT_TRUE(output.find("1") != std::string::npos);
}

}  // namespace
}  // namespace piano_fingering::domain
