#ifndef PIANO_FINGERING_CONFIG_CONFIG_MANAGER_H_
#define PIANO_FINGERING_CONFIG_CONFIG_MANAGER_H_

#include <filesystem>
#include <string>
#include <string_view>

#include "config/config.h"

namespace piano_fingering::config {

class ConfigManager {
 public:
  ConfigManager() = delete;

  [[nodiscard]] static Config load_preset(std::string_view name);

  [[nodiscard]] static Config load_custom(
      const std::filesystem::path& path,
      std::string_view base_preset = "Medium");

  [[nodiscard]] static bool validate(const Config& config,
                                     std::string& error_message);
};

}  // namespace piano_fingering::config

#endif  // PIANO_FINGERING_CONFIG_CONFIG_MANAGER_H_
