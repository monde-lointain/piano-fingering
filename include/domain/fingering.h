#ifndef PIANO_FINGERING_DOMAIN_FINGERING_H_
#define PIANO_FINGERING_DOMAIN_FINGERING_H_

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "domain/finger.h"
#include "domain/slice.h"

namespace piano_fingering::domain {

class Fingering {
 public:
  Fingering() = default;

  Fingering(std::initializer_list<std::optional<Finger>> assignments)
      : assignments_(assignments.begin(), assignments.end()) {}

  [[nodiscard]] size_t size() const noexcept { return assignments_.size(); }
  [[nodiscard]] bool empty() const noexcept { return assignments_.empty(); }

  [[nodiscard]] const std::optional<Finger>& operator[](size_t index) const {
    if (index >= assignments_.size()) {
      throw std::out_of_range("Fingering index out of range");
    }
    return assignments_[index];
  }

  [[nodiscard]] bool is_complete() const noexcept {
    return std::all_of(assignments_.begin(), assignments_.end(),
                       [](const auto& a) { return a.has_value(); });
  }

  [[nodiscard]] bool violates_hard_constraint(const Slice& slice) const {
    if (assignments_.size() != slice.size()) {
      throw std::invalid_argument(
          "Fingering size must match slice size for constraint check");
    }

    std::unordered_set<int> used_fingers;
    for (const auto& assignment : assignments_) {
      if (assignment.has_value()) {
        int finger_val = to_int(assignment.value());
        if (used_fingers.count(finger_val) > 0) {
          return true;  // Duplicate finger found
        }
        used_fingers.insert(finger_val);
      }
    }
    return false;
  }

  [[nodiscard]] auto begin() const noexcept { return assignments_.begin(); }
  [[nodiscard]] auto end() const noexcept { return assignments_.end(); }

 private:
  std::vector<std::optional<Finger>> assignments_;
};

inline std::ostream& operator<<(std::ostream& os, const Fingering& fingering) {
  os << "Fingering([";
  for (size_t i = 0; i < fingering.size(); ++i) {
    if (i > 0) os << ", ";
    if (fingering[i].has_value()) {
      os << to_int(fingering[i].value());
    } else {
      os << "null";
    }
  }
  return os << "])";
}

}  // namespace piano_fingering::domain

#endif  // PIANO_FINGERING_DOMAIN_FINGERING_H_
