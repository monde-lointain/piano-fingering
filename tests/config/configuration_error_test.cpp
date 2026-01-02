#include "config/configuration_error.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace piano_fingering::config {
namespace {

TEST(ConfigurationErrorTest, InheritsFromRuntimeError) {
  ConfigurationError err("test");
  const std::runtime_error* base = &err;
  EXPECT_NE(base, nullptr);
}

TEST(ConfigurationErrorTest, StoresMessage) {
  ConfigurationError err("invalid config");
  EXPECT_STREQ(err.what(), "invalid config");
}

TEST(ConfigurationErrorTest, CanBeThrownAndCaught) {
  EXPECT_THROW(throw ConfigurationError("fail"), ConfigurationError);
}

}  // namespace
}  // namespace piano_fingering::config
