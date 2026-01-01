#ifndef PIANO_FINGERING_DOMAIN_PITCH_H_
#define PIANO_FINGERING_DOMAIN_PITCH_H_

#include <cmath>
#include <compare>
#include <ostream>
#include <stdexcept>

namespace piano_fingering::domain {

class Pitch {
 public:
  explicit Pitch(int value) : value_(validate(value)) {}

  [[nodiscard]] constexpr int value() const noexcept { return value_; }

  [[nodiscard]] constexpr bool is_black_key() const noexcept {
    int mod = value_ % 14;
    return mod == 1 || mod == 3 || mod == 7 || mod == 9 || mod == 11;
  }

  [[nodiscard]] int distance_to(Pitch other) const noexcept {
    return std::abs(value_ - other.value_);
  }

  [[nodiscard]] constexpr auto operator<=>(const Pitch&) const noexcept =
      default;
  [[nodiscard]] constexpr bool operator==(const Pitch&) const noexcept =
      default;

 private:
  int value_;

  static int validate(int value) {
    if (value < 0 || value > 13) {
      throw std::invalid_argument("Pitch value must be in range [0, 13]");
    }
    return value;
  }
};

inline std::ostream& operator<<(std::ostream& os, Pitch pitch) {
  return os << "Pitch(" << pitch.value() << ")";
}

}  // namespace piano_fingering::domain

#endif  // PIANO_FINGERING_DOMAIN_PITCH_H_
