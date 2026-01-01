#ifndef PIANO_FINGERING_DOMAIN_NOTE_H_
#define PIANO_FINGERING_DOMAIN_NOTE_H_

#include <cstdint>
#include <ostream>
#include <stdexcept>

#include "domain/pitch.h"

namespace piano_fingering::domain {

class Note {
 public:
  Note(Pitch pitch, int octave, uint32_t duration, bool is_rest, int staff,
       int voice)
      : pitch_(pitch),
        octave_(validate_octave(octave)),
        duration_(validate_duration(duration)),
        is_rest_(is_rest),
        staff_(validate_staff(staff)),
        voice_(validate_voice(voice)) {}

  [[nodiscard]] Pitch pitch() const noexcept { return pitch_; }
  [[nodiscard]] int octave() const noexcept { return octave_; }
  [[nodiscard]] uint32_t duration() const noexcept { return duration_; }
  [[nodiscard]] bool is_rest() const noexcept { return is_rest_; }
  [[nodiscard]] int staff() const noexcept { return staff_; }
  [[nodiscard]] int voice() const noexcept { return voice_; }

  [[nodiscard]] int absolute_pitch() const noexcept {
    return octave_ * 14 + pitch_.value();
  }

  [[nodiscard]] auto operator<=>(const Note& other) const noexcept {
    return absolute_pitch() <=> other.absolute_pitch();
  }

  [[nodiscard]] bool operator==(const Note& other) const noexcept {
    return absolute_pitch() == other.absolute_pitch();
  }

 private:
  Pitch pitch_;
  int octave_;
  uint32_t duration_;
  bool is_rest_;
  int staff_;
  int voice_;

  static int validate_octave(int octave) {
    if (octave < 0 || octave > 10) {
      throw std::invalid_argument("Octave must be in range [0, 10]");
    }
    return octave;
  }

  static uint32_t validate_duration(uint32_t duration) {
    if (duration == 0) {
      throw std::invalid_argument("Duration must be > 0");
    }
    return duration;
  }

  static int validate_staff(int staff) {
    if (staff < 1 || staff > 2) {
      throw std::invalid_argument("Staff must be 1 or 2");
    }
    return staff;
  }

  static int validate_voice(int voice) {
    if (voice < 1 || voice > 4) {
      throw std::invalid_argument("Voice must be in range [1, 4]");
    }
    return voice;
  }
};

inline std::ostream& operator<<(std::ostream& os, const Note& note) {
  return os << "Note(" << note.pitch() << ", oct=" << note.octave()
            << ", dur=" << note.duration() << ", rest=" << note.is_rest()
            << ", staff=" << note.staff() << ", voice=" << note.voice()
            << ", abs=" << note.absolute_pitch() << ")";
}

}  // namespace piano_fingering::domain

#endif  // PIANO_FINGERING_DOMAIN_NOTE_H_
