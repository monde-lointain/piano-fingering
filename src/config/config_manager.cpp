#include "config/config_manager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "config/configuration_error.h"
#include "config/preset.h"

namespace piano_fingering::config {
namespace {

std::string to_lower(std::string_view str) {
  std::string result(str);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

}  // namespace

Config ConfigManager::load_preset(std::string_view name) {
  const std::string lower_name = to_lower(name);

  if (lower_name == "small") {
    return get_small_preset().to_config();
  }
  if (lower_name == "medium") {
    return get_medium_preset().to_config();
  }
  if (lower_name == "large") {
    return get_large_preset().to_config();
  }

  throw ConfigurationError("Unknown preset: " + std::string(name));
}

Config ConfigManager::load_custom(const std::filesystem::path& /*path*/,
                                  std::string_view /*base_preset*/) {
  // Implemented in Task 10
  throw ConfigurationError("Not implemented");
}

bool ConfigManager::validate(const Config& config, std::string& error_message) {
  error_message.clear();

  if (!config.left_hand.is_valid()) {
    error_message = "Invalid left_hand distance matrix";
    return false;
  }

  if (!config.right_hand.is_valid()) {
    error_message = "Invalid right_hand distance matrix";
    return false;
  }

  if (!config.weights.is_valid()) {
    error_message = "Invalid rule weight (negative value)";
    return false;
  }

  if (!config.algorithm.is_valid()) {
    error_message = "Invalid algorithm parameters (zero value)";
    return false;
  }

  return true;
}

}  // namespace piano_fingering::config
