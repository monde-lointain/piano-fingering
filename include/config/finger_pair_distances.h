#ifndef PIANO_FINGERING_CONFIG_FINGER_PAIR_DISTANCES_H_
#define PIANO_FINGERING_CONFIG_FINGER_PAIR_DISTANCES_H_

namespace piano_fingering::config {

inline constexpr int kMinDistanceValue = -20;
inline constexpr int kMaxDistanceValue = 20;

struct FingerPairDistances {
  int min_prac;
  int min_comf;
  int min_rel;
  int max_rel;
  int max_comf;
  int max_prac;

  [[nodiscard]] constexpr bool is_valid() const noexcept {
    return are_values_in_range() && is_ordering_valid();
  }

  [[nodiscard]] constexpr bool operator==(
      const FingerPairDistances& other) const noexcept = default;

 private:
  [[nodiscard]] constexpr bool are_values_in_range() const noexcept {
    return min_prac >= kMinDistanceValue && max_prac <= kMaxDistanceValue &&
           min_comf >= kMinDistanceValue && max_comf <= kMaxDistanceValue &&
           min_rel >= kMinDistanceValue && max_rel <= kMaxDistanceValue;
  }

  [[nodiscard]] constexpr bool is_ordering_valid() const noexcept {
    // MinPrac <= MinComf <= MinRel < MaxRel <= MaxComf <= MaxPrac
    return min_prac <= min_comf && min_comf <= min_rel && min_rel < max_rel &&
           max_rel <= max_comf && max_comf <= max_prac;
  }
};

}  // namespace piano_fingering::config

#endif  // PIANO_FINGERING_CONFIG_FINGER_PAIR_DISTANCES_H_
