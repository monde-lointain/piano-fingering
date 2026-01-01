#include "domain/time_signature.h"

#include <gtest/gtest.h>

#include <sstream>

namespace piano_fingering::domain {
namespace {

TEST(TimeSignatureTest, ConstructValid) {
  EXPECT_NO_THROW(TimeSignature(4, 4));
  EXPECT_NO_THROW(TimeSignature(3, 8));
  EXPECT_NO_THROW(TimeSignature(6, 16));
  EXPECT_NO_THROW(TimeSignature(1, 1));
}

TEST(TimeSignatureTest, ConstructInvalidNumerator) {
  EXPECT_THROW(
      { [[maybe_unused]] TimeSignature ts(0, 4); }, std::invalid_argument);
}

TEST(TimeSignatureTest, ConstructInvalidDenominator) {
  EXPECT_THROW(
      { [[maybe_unused]] TimeSignature ts(4, 0); }, std::invalid_argument);
  EXPECT_THROW(
      { [[maybe_unused]] TimeSignature ts(4, 3); }, std::invalid_argument);
  EXPECT_THROW(
      { [[maybe_unused]] TimeSignature ts(4, 7); }, std::invalid_argument);
}

TEST(TimeSignatureTest, Accessors) {
  TimeSignature ts(6, 8);
  EXPECT_EQ(ts.numerator(), 6);
  EXPECT_EQ(ts.denominator(), 8);
}

TEST(TimeSignatureTest, CommonTime) {
  auto ts = common_time();
  EXPECT_EQ(ts.numerator(), 4);
  EXPECT_EQ(ts.denominator(), 4);
}

TEST(TimeSignatureTest, CutTime) {
  auto ts = cut_time();
  EXPECT_EQ(ts.numerator(), 2);
  EXPECT_EQ(ts.denominator(), 2);
}

TEST(TimeSignatureTest, Comparison) {
  TimeSignature ts1(4, 4);
  TimeSignature ts2(3, 4);
  TimeSignature ts3(4, 4);

  EXPECT_TRUE(ts1 == ts3);
  EXPECT_FALSE(ts1 == ts2);
  EXPECT_TRUE(ts1 != ts2);
}

TEST(TimeSignatureTest, StreamOutput) {
  std::ostringstream oss;
  oss << TimeSignature(3, 8);
  EXPECT_EQ(oss.str(), "TimeSignature(3/8)");
}

}  // namespace
}  // namespace piano_fingering::domain
