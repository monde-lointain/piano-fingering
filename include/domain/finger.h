#ifndef PIANO_FINGERING_DOMAIN_FINGER_H_
#define PIANO_FINGERING_DOMAIN_FINGER_H_

#include <array>
#include <cstdint>
#include <ostream>
#include <stdexcept>

namespace piano_fingering::domain {

enum class Finger : std::uint8_t {
  kThumb = 1,
  kIndex = 2,
  kMiddle = 3,
  kRing = 4,
  kPinky = 5
};

[[nodiscard]] constexpr int to_int(Finger finger) noexcept {
  return static_cast<int>(finger);
}

[[nodiscard]] inline Finger finger_from_int(int value) {
  if (value < 1 || value > 5) {
    throw std::invalid_argument("Finger value must be in range [1, 5]");
  }
  return static_cast<Finger>(value);
}

[[nodiscard]] constexpr std::array<Finger, 5> all_fingers() noexcept {
  return {Finger::kThumb, Finger::kIndex, Finger::kMiddle, Finger::kRing,
          Finger::kPinky};
}

inline std::ostream& operator<<(std::ostream& os, Finger finger) {
  return os << to_int(finger);
}

}  // namespace piano_fingering::domain

#endif  // PIANO_FINGERING_DOMAIN_FINGER_H_
