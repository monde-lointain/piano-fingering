#ifndef PIANO_FINGERING_DOMAIN_PIECE_H_
#define PIANO_FINGERING_DOMAIN_PIECE_H_

#include <initializer_list>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "domain/measure.h"
#include "domain/metadata.h"

namespace piano_fingering::domain {

class Piece {
 public:
  Piece(Metadata metadata, std::initializer_list<Measure> left_hand,
        std::initializer_list<Measure> right_hand)
      : metadata_(std::move(metadata)),
        left_hand_(left_hand.begin(), left_hand.end()),
        right_hand_(right_hand.begin(), right_hand.end()) {
    if (left_hand_.empty() && right_hand_.empty()) {
      throw std::invalid_argument("Piece must have at least one measure");
    }
  }

  Piece(Metadata metadata, std::vector<Measure> left_hand,
        std::vector<Measure> right_hand)
      : metadata_(std::move(metadata)),
        left_hand_(std::move(left_hand)),
        right_hand_(std::move(right_hand)) {
    if (left_hand_.empty() && right_hand_.empty()) {
      throw std::invalid_argument("Piece must have at least one measure");
    }
  }

  [[nodiscard]] const Metadata& metadata() const noexcept { return metadata_; }
  [[nodiscard]] const std::vector<Measure>& left_hand() const noexcept {
    return left_hand_;
  }
  [[nodiscard]] const std::vector<Measure>& right_hand() const noexcept {
    return right_hand_;
  }

  [[nodiscard]] size_t total_measures() const noexcept {
    return left_hand_.size() + right_hand_.size();
  }

  [[nodiscard]] bool empty() const noexcept {
    return left_hand_.empty() && right_hand_.empty();
  }

 private:
  Metadata metadata_;
  std::vector<Measure> left_hand_;
  std::vector<Measure> right_hand_;
};

inline std::ostream& operator<<(std::ostream& os, const Piece& piece) {
  return os << "Piece(" << piece.metadata()
            << ", left=" << piece.left_hand().size()
            << ", right=" << piece.right_hand().size() << ")";
}

}  // namespace piano_fingering::domain

#endif  // PIANO_FINGERING_DOMAIN_PIECE_H_
