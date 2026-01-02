#ifndef PIANO_FINGERING_CONFIG_RULE_WEIGHTS_H_
#define PIANO_FINGERING_CONFIG_RULE_WEIGHTS_H_

#include <algorithm>
#include <array>
#include <cstddef>

namespace piano_fingering::config {

inline constexpr std::size_t kRuleCount = 15;

struct RuleWeights {
  std::array<double, kRuleCount> values{};

  [[nodiscard]] constexpr bool is_valid() const noexcept {
    return std::all_of(values.begin(), values.end(),
                       [](double w) { return w >= 0.0; });
  }

  [[nodiscard]] static constexpr RuleWeights defaults() noexcept {
    // From SRS Appendix A.2
    return RuleWeights{{
        2.0,   // Rule 1: Below MinComf or above MaxComf
        1.0,   // Rule 2: Below MinRel or above MaxRel
        1.0,   // Rule 3: Hand position change (triplet)
        1.0,   // Rule 4: Distance exceeds comfort (triplet)
        1.0,   // Rule 5: Fourth finger usage
        1.0,   // Rule 6: Third and fourth finger consecutive
        1.0,   // Rule 7: Third on white, fourth on black
        0.5,   // Rule 8: Thumb on black key (base)
        1.0,   // Rule 9: Fifth finger on black key
        1.0,   // Rule 10: Thumb crossing (same level)
        2.0,   // Rule 11: Thumb on black crossed by finger on white
        1.0,   // Rule 12: Same finger reuse with position change
        10.0,  // Rule 13: Below MinPrac or above MaxPrac
        1.0,   // Rule 14: Rules 1,2,13 within chord (doubled)
        1.0    // Rule 15: Same pitch, different finger
    }};
  }

  [[nodiscard]] constexpr bool operator==(
      const RuleWeights& other) const noexcept = default;
};

}  // namespace piano_fingering::config

#endif  // PIANO_FINGERING_CONFIG_RULE_WEIGHTS_H_
