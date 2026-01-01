#ifndef PIANO_FINGERING_DOMAIN_SLICE_H_
#define PIANO_FINGERING_DOMAIN_SLICE_H_

#include <algorithm>
#include <initializer_list>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "domain/note.h"

namespace piano_fingering::domain {

constexpr size_t kMaxNotesPerSlice = 5;

class Slice {
 public:
  Slice() = default;

  Slice(std::initializer_list<Note> notes)
      : notes_(notes.begin(), notes.end()) {
    if (notes_.size() > kMaxNotesPerSlice) {
      throw std::invalid_argument("Slice cannot contain more than 5 notes");
    }
    std::sort(notes_.begin(), notes_.end());
  }

  [[nodiscard]] size_t size() const noexcept { return notes_.size(); }
  [[nodiscard]] bool empty() const noexcept { return notes_.empty(); }

  [[nodiscard]] const Note& operator[](size_t index) const {
    if (index >= notes_.size()) {
      throw std::out_of_range("Slice index out of range");
    }
    return notes_[index];
  }

  [[nodiscard]] auto begin() const noexcept { return notes_.begin(); }
  [[nodiscard]] auto end() const noexcept { return notes_.end(); }

 private:
  std::vector<Note> notes_;
};

inline std::ostream& operator<<(std::ostream& os, const Slice& slice) {
  return os << "Slice(" << slice.size() << " notes)";
}

}  // namespace piano_fingering::domain

#endif  // PIANO_FINGERING_DOMAIN_SLICE_H_
