// tests/parser/golden_set_test.cpp - Integration tests for baseline files

#include "parser/musicxml_parser.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

namespace piano_fingering::parser {
namespace {

// Parameterized test for all baseline MusicXML files
class GoldenSetTest : public ::testing::TestWithParam<std::string> {};

TEST_P(GoldenSetTest, ParsesBaselineFile) {
  std::string filename = GetParam();
  std::filesystem::path baseline_path =
      std::filesystem::path(BASELINE_DIR) / filename;

  // Verify file exists
  ASSERT_TRUE(std::filesystem::exists(baseline_path))
      << "Baseline file not found: " << baseline_path;

  // Parse should succeed without throwing
  EXPECT_NO_THROW({
    auto result = MusicXMLParser::parse(baseline_path);

    // Basic sanity checks
    EXPECT_FALSE(result.piece.metadata().title().empty());
    EXPECT_NE(result.original_xml, nullptr);
  });
}

INSTANTIATE_TEST_SUITE_P(
    BaselineFiles, GoldenSetTest,
    ::testing::Values("czerny_op821_1.musicxml", "czerny_op821_37.musicxml",
                      "czerny_op821_38.musicxml", "czerny_op821_54.musicxml",
                      "czerny_op821_62.musicxml", "czerny_op821_66.musicxml",
                      "czerny_op821_96.musicxml", "poly_test.musicxml"));

}  // namespace
}  // namespace piano_fingering::parser
