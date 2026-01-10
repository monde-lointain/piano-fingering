#ifndef PIANO_FINGERING_DOMAIN_MEASURE_H_
#define PIANO_FINGERING_DOMAIN_MEASURE_H_

#include <initializer_list>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "domain/slice.h"
#include "domain/time_signature.h"

namespace piano_fingering::domain {

class Measure {
 public:
  Measure(int number, std::initializer_list<Slice> slices,
          TimeSignature time_signature)
      : number_(validate_number(number)),
        slices_(slices.begin(), slices.end()),
        time_signature_(time_signature) {
    if (slices_.empty()) {
      throw std::invalid_argument("Measure must contain at least one slice");
    }
  }

  Measure(int number, std::vector<Slice> slices, TimeSignature time_signature)
      : number_(validate_number(number)),
        slices_(std::move(slices)),
        time_signature_(time_signature) {
    if (slices_.empty()) {
      throw std::invalid_argument("Measure must contain at least one slice");
    }
  }

  [[nodiscard]] int number() const noexcept { return number_; }
  [[nodiscard]] size_t size() const noexcept { return slices_.size(); }
  [[nodiscard]] TimeSignature time_signature() const noexcept {
    return time_signature_;
  }

  [[nodiscard]] const Slice& operator[](size_t index) const {
    if (index >= slices_.size()) {
      throw std::out_of_range("Measure slice index out of range");
    }
    return slices_[index];
  }

  [[nodiscard]] auto begin() const noexcept { return slices_.begin(); }
  [[nodiscard]] auto end() const noexcept { return slices_.end(); }

 private:
  int number_;
  std::vector<Slice> slices_;
  TimeSignature time_signature_;

  static int validate_number(int number) {
    if (number <= 0) {
      throw std::invalid_argument("Measure number must be > 0");
    }
    return number;
  }
};

inline std::ostream& operator<<(std::ostream& os, const Measure& measure) {
  return os << "Measure(" << measure.number() << ", " << measure.size()
            << " slices, " << measure.time_signature() << ")";
}

}  // namespace piano_fingering::domain

#endif  // PIANO_FINGERING_DOMAIN_MEASURE_H_
