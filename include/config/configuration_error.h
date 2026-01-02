#ifndef PIANO_FINGERING_CONFIG_CONFIGURATION_ERROR_H_
#define PIANO_FINGERING_CONFIG_CONFIGURATION_ERROR_H_

#include <stdexcept>
#include <string>

namespace piano_fingering::config {

class ConfigurationError : public std::runtime_error {
 public:
  explicit ConfigurationError(const std::string& message)
      : std::runtime_error(message) {}
};

}  // namespace piano_fingering::config

#endif  // PIANO_FINGERING_CONFIG_CONFIGURATION_ERROR_H_
