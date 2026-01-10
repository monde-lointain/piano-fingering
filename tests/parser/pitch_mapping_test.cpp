// tests/parser/pitch_mapping_test.cpp - Tests for pitch mapping utility
#include "parser/pitch_mapping.h"

#include <gtest/gtest.h>

namespace piano_fingering::parser {
namespace {

TEST(PitchMappingTest, NaturalNotes) {
  EXPECT_EQ(step_alter_to_pitch("C", 0).value(), 0);
  EXPECT_EQ(step_alter_to_pitch("D", 0).value(), 2);
  EXPECT_EQ(step_alter_to_pitch("E", 0).value(), 4);
  EXPECT_EQ(step_alter_to_pitch("F", 0).value(), 6);
  EXPECT_EQ(step_alter_to_pitch("G", 0).value(), 7);
  EXPECT_EQ(step_alter_to_pitch("A", 0).value(), 9);
  EXPECT_EQ(step_alter_to_pitch("B", 0).value(), 11);
}

TEST(PitchMappingTest, Sharps) {
  EXPECT_EQ(step_alter_to_pitch("C", 1).value(), 1);  // C#
  EXPECT_EQ(step_alter_to_pitch("F", 1).value(), 7);  // F#
  EXPECT_EQ(step_alter_to_pitch("G", 1).value(), 8);  // G#
}

TEST(PitchMappingTest, Flats) {
  EXPECT_EQ(step_alter_to_pitch("D", -1).value(), 1);   // Db
  EXPECT_EQ(step_alter_to_pitch("E", -1).value(), 3);   // Eb
  EXPECT_EQ(step_alter_to_pitch("B", -1).value(), 10);  // Bb
}

TEST(PitchMappingTest, DoubleAlterations) {
  EXPECT_EQ(step_alter_to_pitch("C", 2).value(), 2);   // C## (D)
  EXPECT_EQ(step_alter_to_pitch("D", -2).value(), 0);  // Dbb (C)
  EXPECT_EQ(step_alter_to_pitch("F", 2).value(), 8);   // F##
}

TEST(PitchMappingTest, WrapAround) {
  EXPECT_EQ(step_alter_to_pitch("C", -1).value(), 13);  // Cb wraps to 13
  EXPECT_EQ(step_alter_to_pitch("C", -2).value(), 12);  // Cbb wraps to 12
}

TEST(PitchMappingTest, InvalidStep) {
  EXPECT_THROW(step_alter_to_pitch("X", 0), std::invalid_argument);
}

}  // namespace
}  // namespace piano_fingering::parser
