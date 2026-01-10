// include/parser/pitch_mapping.h - Pitch mapping from MusicXML to modified
// system
#ifndef PFING_PARSER_PITCH_MAPPING_H_
#define PFING_PARSER_PITCH_MAPPING_H_

#include <array>
#include <stdexcept>

#include "domain/pitch.h"

namespace piano_fingering::parser {

// Maps MusicXML pitch (step + alter) to modified pitch class (0-13).
// step: 'A'-'G' (case-insensitive)
// alter: -2 (double flat) to +2 (double sharp)
// Returns: domain::Pitch with value 0-13
// Throws: std::invalid_argument if step is not A-G
inline domain::Pitch step_alter_to_pitch(const std::string& step, int alter) {
  static constexpr std::array<int, 7> kBase = {9, 11, 0, 2, 4, 6, 7};  // A-G

  if (step.empty()) {
    throw std::invalid_argument("Invalid step: empty string");
  }

  char ch = step[0];
  int idx = 0;
  if (ch >= 'a' && ch <= 'g') {
    idx = ch - 'a';
  } else if (ch >= 'A' && ch <= 'G') {
    idx = ch - 'A';
  } else {
    throw std::invalid_argument("Invalid step: must be A-G");
  }

  int base = kBase[idx];
  int result = base + alter;
  // Handle negative wrapping: -1 -> 13, -2 -> 12, etc.
  while (result < 0) {
    result += 14;
  }
  return domain::Pitch(result % 14);
}

}  // namespace piano_fingering::parser

#endif  // PFING_PARSER_PITCH_MAPPING_H_
