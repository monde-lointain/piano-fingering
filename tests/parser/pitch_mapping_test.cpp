// tests/parser/pitch_mapping_test.cpp - Tests for pitch mapping utility
#include "parser/pitch_mapping.h"

#include <gtest/gtest.h>

namespace pfing::parser {
namespace {

TEST(PitchMappingTest, BasicNaturalPitches) {
  EXPECT_EQ(map_musicxml_pitch_to_modified('C', 0), 0);
  EXPECT_EQ(map_musicxml_pitch_to_modified('D', 0), 2);
  EXPECT_EQ(map_musicxml_pitch_to_modified('E', 0), 4);
  EXPECT_EQ(map_musicxml_pitch_to_modified('F', 0), 6);
  EXPECT_EQ(map_musicxml_pitch_to_modified('G', 0), 7);
  EXPECT_EQ(map_musicxml_pitch_to_modified('A', 0), 9);
  EXPECT_EQ(map_musicxml_pitch_to_modified('B', 0), 11);
}

TEST(PitchMappingTest, SharpPitches) {
  EXPECT_EQ(map_musicxml_pitch_to_modified('C', 1), 1);   // C#
  EXPECT_EQ(map_musicxml_pitch_to_modified('F', 1), 7);   // F#
  EXPECT_EQ(map_musicxml_pitch_to_modified('G', 1), 8);   // G#
}

TEST(PitchMappingTest, FlatPitches) {
  EXPECT_EQ(map_musicxml_pitch_to_modified('D', -1), 1);  // Db
  EXPECT_EQ(map_musicxml_pitch_to_modified('E', -1), 3);  // Eb
  EXPECT_EQ(map_musicxml_pitch_to_modified('B', -1), 10); // Bb
}

TEST(PitchMappingTest, DoubleAlterations) {
  EXPECT_EQ(map_musicxml_pitch_to_modified('C', 2), 2);   // C## (D)
  EXPECT_EQ(map_musicxml_pitch_to_modified('D', -2), 0);  // Dbb (C)
  EXPECT_EQ(map_musicxml_pitch_to_modified('F', 2), 8);   // F##
}

TEST(PitchMappingTest, NegativeWrapping) {
  EXPECT_EQ(map_musicxml_pitch_to_modified('C', -1), 13); // Cb wraps to 13
  EXPECT_EQ(map_musicxml_pitch_to_modified('C', -2), 12); // Cbb wraps to 12
}

}  // namespace
}  // namespace pfing::parser
