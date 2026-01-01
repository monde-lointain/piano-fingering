#ifndef PIANO_FINGERING_DOMAIN_TIME_SIGNATURE_H_
#define PIANO_FINGERING_DOMAIN_TIME_SIGNATURE_H_

#include <bit>
#include <compare>
#include <ostream>
#include <stdexcept>

namespace piano_fingering::domain {

class TimeSignature {
 public:
  TimeSignature(int numerator, int denominator)
      : numerator_(validate_numerator(numerator)),
        denominator_(validate_denominator(denominator)) {}

  [[nodiscard]] int numerator() const noexcept { return numerator_; }
  [[nodiscard]] int denominator() const noexcept { return denominator_; }

  [[nodiscard]] auto operator<=>(const TimeSignature&) const noexcept = default;
  [[nodiscard]] bool operator==(const TimeSignature&) const noexcept = default;

 private:
  int numerator_;
  int denominator_;

  static int validate_numerator(int numerator) {
    if (numerator <= 0) {
      throw std::invalid_argument("Numerator must be > 0");
    }
    return numerator;
  }

  static int validate_denominator(int denominator) {
    if (denominator <= 0 ||
        !std::has_single_bit(static_cast<unsigned>(denominator))) {
      throw std::invalid_argument("Denominator must be a power of 2");
    }
    return denominator;
  }
};

[[nodiscard]] inline TimeSignature common_time() { return {4, 4}; }

[[nodiscard]] inline TimeSignature cut_time() { return {2, 2}; }

inline std::ostream& operator<<(std::ostream& os, const TimeSignature& ts) {
  return os << "TimeSignature(" << ts.numerator() << "/" << ts.denominator()
            << ")";
}

}  // namespace piano_fingering::domain

#endif  // PIANO_FINGERING_DOMAIN_TIME_SIGNATURE_H_
