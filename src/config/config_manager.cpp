#include "config/config_manager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

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

FingerPair finger_pair_from_string(const std::string& str) {
  static const std::unordered_map<std::string, FingerPair> kMapping = {
      {"1-2", FingerPair::kThumbIndex},  {"1-3", FingerPair::kThumbMiddle},
      {"1-4", FingerPair::kThumbRing},   {"1-5", FingerPair::kThumbPinky},
      {"2-3", FingerPair::kIndexMiddle}, {"2-4", FingerPair::kIndexRing},
      {"2-5", FingerPair::kIndexPinky},  {"3-4", FingerPair::kMiddleRing},
      {"3-5", FingerPair::kMiddlePinky}, {"4-5", FingerPair::kRingPinky},
  };
  auto it = kMapping.find(str);
  if (it == kMapping.end()) {
    throw ConfigurationError("Unknown finger pair: " + str);
  }
  return it->second;
}

void apply_distance_overrides(DistanceMatrix& matrix,
                              const nlohmann::json& json) {
  for (const auto& [pair_str, values] : json.items()) {
    FingerPair pair = finger_pair_from_string(pair_str);
    auto& d = matrix.get_pair(pair);

    if (values.contains("MinPrac")) {
      d.min_prac = values["MinPrac"].get<int>();
    }
    if (values.contains("MinComf")) {
      d.min_comf = values["MinComf"].get<int>();
    }
    if (values.contains("MinRel")) {
      d.min_rel = values["MinRel"].get<int>();
    }
    if (values.contains("MaxRel")) {
      d.max_rel = values["MaxRel"].get<int>();
    }
    if (values.contains("MaxComf")) {
      d.max_comf = values["MaxComf"].get<int>();
    }
    if (values.contains("MaxPrac")) {
      d.max_prac = values["MaxPrac"].get<int>();
    }
  }
}

void apply_algorithm_overrides(AlgorithmParameters& algo_params,
                               const nlohmann::json& json) {
  if (!json.contains("algorithm")) {
    return;
  }
  const auto& algo = json["algorithm"];
  if (algo.contains("beam_width")) {
    algo_params.beam_width = algo["beam_width"].get<std::size_t>();
  }
  if (algo.contains("ils_iterations")) {
    algo_params.ils_iterations = algo["ils_iterations"].get<std::size_t>();
  }
  if (algo.contains("perturbation_strength")) {
    algo_params.perturbation_strength =
        algo["perturbation_strength"].get<std::size_t>();
  }
}

void apply_weights_overrides(RuleWeights& weights, const nlohmann::json& json) {
  if (!json.contains("rule_weights") || !json["rule_weights"].is_array()) {
    return;
  }
  const auto& weights_json = json["rule_weights"];
  for (std::size_t i = 0; i < weights_json.size() && i < kRuleCount; ++i) {
    if (!weights_json[i].is_null()) {
      weights.values[i] = weights_json[i].get<double>();  // NOLINT
    }
  }
}

void apply_distance_matrix_overrides(Config& config,
                                      const nlohmann::json& json) {
  if (!json.contains("distance_matrix")) {
    return;
  }
  const auto& dm = json["distance_matrix"];
  if (dm.contains("left_hand")) {
    apply_distance_overrides(config.left_hand, dm["left_hand"]);
  }
  if (dm.contains("right_hand")) {
    apply_distance_overrides(config.right_hand, dm["right_hand"]);
  }
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file) {
    throw ConfigurationError("Cannot open file: " + path.string());
  }

  nlohmann::json json;
  try {
    file >> json;
  } catch (const nlohmann::json::parse_error& e) {
    throw ConfigurationError("JSON parse error: " + std::string(e.what()));
  }
  return json;
}

}  // namespace

Config ConfigManager::load_preset(std::string_view name) {
  const std::string lower_name = to_lower(name);  // NOLINT

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

Config ConfigManager::load_custom(const std::filesystem::path& path,
                                  std::string_view base_preset) {
  Config config = load_preset(base_preset);
  nlohmann::json json = read_json_file(path);

  apply_algorithm_overrides(config.algorithm, json);
  apply_weights_overrides(config.weights, json);
  apply_distance_matrix_overrides(config, json);

  std::string error;
  if (!validate(config, error)) {
    throw ConfigurationError("Invalid configuration: " + error);
  }

  return config;
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
