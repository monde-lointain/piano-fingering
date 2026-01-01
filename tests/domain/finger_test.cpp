#include "domain/finger.h"

#include <gtest/gtest.h>

#include <sstream>
#include <type_traits>

namespace piano_fingering::domain {
namespace {

TEST(FingerTest, IsEnumClass) {
  static_assert(std::is_enum_v<Finger>);
  static_assert(!std::is_convertible_v<Finger, int>);
}

TEST(FingerTest, HasAllFiveFingers) {
  [[maybe_unused]] Finger thumb = Finger::kThumb;
  [[maybe_unused]] Finger index = Finger::kIndex;
  [[maybe_unused]] Finger middle = Finger::kMiddle;
  [[maybe_unused]] Finger ring = Finger::kRing;
  [[maybe_unused]] Finger pinky = Finger::kPinky;
}

TEST(FingerTest, UnderlyingValues) {
  EXPECT_EQ(static_cast<int>(Finger::kThumb), 1);
  EXPECT_EQ(static_cast<int>(Finger::kIndex), 2);
  EXPECT_EQ(static_cast<int>(Finger::kMiddle), 3);
  EXPECT_EQ(static_cast<int>(Finger::kRing), 4);
  EXPECT_EQ(static_cast<int>(Finger::kPinky), 5);
}

TEST(FingerTest, ToIntConversion) {
  EXPECT_EQ(to_int(Finger::kThumb), 1);
  EXPECT_EQ(to_int(Finger::kPinky), 5);
}

TEST(FingerTest, FromIntValid) {
  EXPECT_EQ(finger_from_int(1), Finger::kThumb);
  EXPECT_EQ(finger_from_int(5), Finger::kPinky);
}

TEST(FingerTest, FromIntInvalidThrows) {
  EXPECT_THROW(
      { [[maybe_unused]] auto f = finger_from_int(0); }, std::invalid_argument);
  EXPECT_THROW(
      { [[maybe_unused]] auto f = finger_from_int(6); }, std::invalid_argument);
}

TEST(FingerTest, StreamOutput) {
  std::ostringstream oss;
  oss << Finger::kThumb << " " << Finger::kPinky;
  EXPECT_EQ(oss.str(), "1 5");
}

TEST(FingerTest, AllFingersIterable) {
  auto fingers = all_fingers();
  EXPECT_EQ(fingers.size(), 5);
  EXPECT_EQ(fingers[0], Finger::kThumb);
  EXPECT_EQ(fingers[4], Finger::kPinky);
}

}  // namespace
}  // namespace piano_fingering::domain
