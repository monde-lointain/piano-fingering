#ifndef PIANO_FINGERING_DOMAIN_HAND_H_
#define PIANO_FINGERING_DOMAIN_HAND_H_

#include <ostream>

namespace piano_fingering::domain {

enum class Hand { kLeft, kRight };

[[nodiscard]] constexpr Hand opposite(Hand hand) noexcept {
  return hand == Hand::kLeft ? Hand::kRight : Hand::kLeft;
}

inline std::ostream& operator<<(std::ostream& os, Hand hand) {
  return os << (hand == Hand::kLeft ? "LEFT" : "RIGHT");
}

}  // namespace piano_fingering::domain

#endif  // PIANO_FINGERING_DOMAIN_HAND_H_
