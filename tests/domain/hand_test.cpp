#include "domain/hand.h"

#include <gtest/gtest.h>

#include <sstream>
#include <type_traits>

namespace piano_fingering::domain {
namespace {

TEST(HandTest, IsEnumClass) {
  static_assert(std::is_enum_v<Hand>);
  static_assert(!std::is_convertible_v<Hand, int>);
}

TEST(HandTest, HasLeftAndRightValues) {
  [[maybe_unused]] Hand left = Hand::kLeft;
  [[maybe_unused]] Hand right = Hand::kRight;
}

TEST(HandTest, ValuesAreDistinct) { EXPECT_NE(Hand::kLeft, Hand::kRight); }

TEST(HandTest, StreamOutputLeft) {
  std::ostringstream oss;
  oss << Hand::kLeft;
  EXPECT_EQ(oss.str(), "LEFT");
}

TEST(HandTest, StreamOutputRight) {
  std::ostringstream oss;
  oss << Hand::kRight;
  EXPECT_EQ(oss.str(), "RIGHT");
}

TEST(HandTest, OppositeLeft) { EXPECT_EQ(opposite(Hand::kLeft), Hand::kRight); }

TEST(HandTest, OppositeRight) {
  EXPECT_EQ(opposite(Hand::kRight), Hand::kLeft);
}

}  // namespace
}  // namespace piano_fingering::domain
