#ifndef PIANO_FINGERING_CONFIG_ALGORITHM_PARAMETERS_H_
#define PIANO_FINGERING_CONFIG_ALGORITHM_PARAMETERS_H_

#include <cstddef>

namespace piano_fingering::config {

struct AlgorithmParameters {
  std::size_t beam_width = 100;
  std::size_t ils_iterations = 1000;
  std::size_t perturbation_strength = 3;

  [[nodiscard]] constexpr bool is_valid() const noexcept {
    return beam_width > 0 && ils_iterations > 0 && perturbation_strength > 0;
  }

  [[nodiscard]] constexpr bool operator==(
      const AlgorithmParameters& other) const noexcept = default;
};

}  // namespace piano_fingering::config

#endif  // PIANO_FINGERING_CONFIG_ALGORITHM_PARAMETERS_H_
