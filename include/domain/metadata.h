#ifndef PIANO_FINGERING_DOMAIN_METADATA_H_
#define PIANO_FINGERING_DOMAIN_METADATA_H_

#include <compare>
#include <ostream>
#include <string>

namespace piano_fingering::domain {

class Metadata {
 public:
  Metadata(std::string title, std::string composer)
      : title_(std::move(title)), composer_(std::move(composer)) {}

  [[nodiscard]] const std::string& title() const noexcept { return title_; }
  [[nodiscard]] const std::string& composer() const noexcept {
    return composer_;
  }

  [[nodiscard]] auto operator<=>(const Metadata&) const noexcept = default;
  [[nodiscard]] bool operator==(const Metadata&) const noexcept = default;

 private:
  std::string title_;
  std::string composer_;
};

inline std::ostream& operator<<(std::ostream& os, const Metadata& metadata) {
  return os << "Metadata(title=\"" << metadata.title() << "\", composer=\""
            << metadata.composer() << "\")";
}

}  // namespace piano_fingering::domain

#endif  // PIANO_FINGERING_DOMAIN_METADATA_H_
