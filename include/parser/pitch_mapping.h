// include/parser/pitch_mapping.h - Pitch mapping from MusicXML to modified system
#ifndef PFING_PARSER_PITCH_MAPPING_H_
#define PFING_PARSER_PITCH_MAPPING_H_

namespace pfing::parser {

// Maps MusicXML pitch (step + alter) to modified pitch class (0-13).
// step: 'A'-'G' (case-insensitive)
// alter: -2 (double flat) to +2 (double sharp)
// Returns: modified pitch class 0-13
inline int map_musicxml_pitch_to_modified(char step, int alter) {
  static constexpr int kBase[] = {9, 11, 0, 2, 4, 6, 7};  // A-G
  int idx = (step >= 'a') ? (step - 'a') : (step - 'A');
  int base = kBase[idx];
  int result = base + alter;
  // Handle negative wrapping: -1 -> 13, -2 -> 12, etc.
  while (result < 0) {
    result += 14;
  }
  return result % 14;
}

}  // namespace pfing::parser

#endif  // PFING_PARSER_PITCH_MAPPING_H_
