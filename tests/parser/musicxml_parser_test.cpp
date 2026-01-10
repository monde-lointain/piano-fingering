// tests/parser/musicxml_parser_test.cpp - Unit tests for MusicXMLParser

#include "parser/musicxml_parser.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "domain/piece.h"
#include "parser/parser_error.h"

namespace piano_fingering::parser {
namespace {

// Fixture for parser tests
class MusicXMLParserTest : public ::testing::Test {
 protected:
  // Create temp directory for test files
  void SetUp() override {
    temp_dir_ = std::filesystem::temp_directory_path() / "parser_test";
    std::filesystem::create_directories(temp_dir_);
  }

  // Clean up temp files
  void TearDown() override {
    if (std::filesystem::exists(temp_dir_)) {
      std::filesystem::remove_all(temp_dir_);
    }
  }

  // Write XML content to temporary file
  std::filesystem::path write_temp_xml(const std::string& filename,
                                       const std::string& content) {
    auto path = temp_dir_ / filename;
    std::ofstream file(path);
    file << content;
    return path;
  }

  std::filesystem::path temp_dir_;
};

// Test: File not found
TEST_F(MusicXMLParserTest, FileNotFound) {
  auto nonexistent_path = temp_dir_ / "nonexistent.xml";
  EXPECT_THROW(MusicXMLParser::parse(nonexistent_path), FileNotFoundError);
}

// Test: Malformed XML
TEST_F(MusicXMLParserTest, MalformedXML) {
  auto xml = R"(<?xml version="1.0"?>
<score-partwise version="4.0">
  <part id="P1">
    <measure number="1">
      <note>
        <pitch>
          <step>C</step>
          <octave>4
        </pitch>
      </note>
    </measure>
  </part>
</score-partwise>
)";

  auto path = write_temp_xml("malformed.xml", xml);
  EXPECT_THROW(MusicXMLParser::parse(path), MalformedXMLError);
}

// Test: Missing score-partwise root
TEST_F(MusicXMLParserTest, MissingScorePartwise) {
  auto xml = R"(<?xml version="1.0"?>
<score-timewise version="4.0">
  <part id="P1">
  </part>
</score-timewise>
)";

  auto path = write_temp_xml("no_partwise.xml", xml);
  EXPECT_THROW(MusicXMLParser::parse(path), MissingElementError);
}

// Test: Single note right hand
TEST_F(MusicXMLParserTest, SingleNoteRightHand) {
  auto xml = R"(<?xml version="1.0"?>
<score-partwise version="4.0">
  <work>
    <work-title>Test Piece</work-title>
  </work>
  <identification>
    <creator type="composer">Test Composer</creator>
  </identification>
  <part id="P1">
    <measure number="1">
      <attributes>
        <time>
          <beats>4</beats>
          <beat-type>4</beat-type>
        </time>
      </attributes>
      <note>
        <pitch>
          <step>C</step>
          <octave>4</octave>
        </pitch>
        <duration>4</duration>
        <voice>1</voice>
        <staff>1</staff>
      </note>
    </measure>
  </part>
</score-partwise>
)";

  auto path = write_temp_xml("single_note.xml", xml);
  auto result = MusicXMLParser::parse(path);

  // Check metadata
  EXPECT_EQ(result.piece.metadata().title(), "Test Piece");
  EXPECT_EQ(result.piece.metadata().composer(), "Test Composer");

  // Check right hand has one measure
  const auto& rh_measures = result.piece.right_hand();
  ASSERT_EQ(rh_measures.size(), 1);
  EXPECT_EQ(rh_measures[0].number(), 1);

  // Check measure has one slice with one note
  ASSERT_EQ(rh_measures[0].size(), 1);
  const auto& slice = rh_measures[0][0];
  ASSERT_EQ(slice.size(), 1);

  const auto& note = slice[0];
  EXPECT_EQ(note.octave(), 4);
  EXPECT_EQ(note.duration(), 4);
  EXPECT_FALSE(note.is_rest());
  EXPECT_EQ(note.staff(), 1);

  // Check left hand is empty
  EXPECT_TRUE(result.piece.left_hand().empty());

  // Check original XML is preserved
  ASSERT_NE(result.original_xml, nullptr);
  EXPECT_TRUE(result.original_xml->child("score-partwise"));
}

// Test: Chord parsing
TEST_F(MusicXMLParserTest, ChordParsing) {
  auto xml = R"(<?xml version="1.0"?>
<score-partwise version="4.0">
  <part id="P1">
    <measure number="1">
      <note>
        <pitch>
          <step>C</step>
          <octave>4</octave>
        </pitch>
        <duration>4</duration>
        <staff>1</staff>
      </note>
      <note>
        <chord/>
        <pitch>
          <step>E</step>
          <octave>4</octave>
        </pitch>
        <duration>4</duration>
        <staff>1</staff>
      </note>
      <note>
        <chord/>
        <pitch>
          <step>G</step>
          <octave>4</octave>
        </pitch>
        <duration>4</duration>
        <staff>1</staff>
      </note>
    </measure>
  </part>
</score-partwise>
)";

  auto path = write_temp_xml("chord.xml", xml);
  auto result = MusicXMLParser::parse(path);

  const auto& rh_measures = result.piece.right_hand();
  ASSERT_EQ(rh_measures.size(), 1);

  // Should have one slice with three notes
  ASSERT_EQ(rh_measures[0].size(), 1);
  const auto& slice = rh_measures[0][0];
  ASSERT_EQ(slice.size(), 3);

  // All notes should have same duration
  for (const auto& note : slice) {
    EXPECT_EQ(note.duration(), 4);
    EXPECT_FALSE(note.is_rest());
  }
}

// Test: Rest handling
TEST_F(MusicXMLParserTest, RestHandling) {
  auto xml = R"(<?xml version="1.0"?>
<score-partwise version="4.0">
  <part id="P1">
    <measure number="1">
      <note>
        <rest/>
        <duration>4</duration>
        <staff>1</staff>
      </note>
    </measure>
  </part>
</score-partwise>
)";

  auto path = write_temp_xml("rest.xml", xml);
  auto result = MusicXMLParser::parse(path);

  const auto& rh_measures = result.piece.right_hand();
  ASSERT_EQ(rh_measures.size(), 1);

  ASSERT_EQ(rh_measures[0].size(), 1);
  const auto& slice = rh_measures[0][0];
  ASSERT_EQ(slice.size(), 1);

  const auto& note = slice[0];
  EXPECT_TRUE(note.is_rest());
  EXPECT_EQ(note.duration(), 4);
}

// Test: Left hand (staff 2)
TEST_F(MusicXMLParserTest, LeftHandStaff2) {
  auto xml = R"(<?xml version="1.0"?>
<score-partwise version="4.0">
  <part id="P1">
    <measure number="1">
      <note>
        <pitch>
          <step>C</step>
          <octave>3</octave>
        </pitch>
        <duration>4</duration>
        <staff>2</staff>
      </note>
    </measure>
  </part>
</score-partwise>
)";

  auto path = write_temp_xml("left_hand.xml", xml);
  auto result = MusicXMLParser::parse(path);

  // Left hand should have one measure
  const auto& lh_measures = result.piece.left_hand();
  ASSERT_EQ(lh_measures.size(), 1);

  ASSERT_EQ(lh_measures[0].size(), 1);
  const auto& slice = lh_measures[0][0];
  ASSERT_EQ(slice.size(), 1);

  const auto& note = slice[0];
  EXPECT_EQ(note.staff(), 2);
  EXPECT_EQ(note.octave(), 3);

  // Right hand should be empty
  EXPECT_TRUE(result.piece.right_hand().empty());
}

// Test: Original XML preservation
TEST_F(MusicXMLParserTest, PreservesOriginalXML) {
  auto xml = R"(<?xml version="1.0"?>
<score-partwise version="4.0">
  <work>
    <work-title>Original Title</work-title>
  </work>
  <part id="P1">
    <measure number="1">
      <note>
        <pitch>
          <step>C</step>
          <octave>4</octave>
        </pitch>
        <duration>4</duration>
        <staff>1</staff>
      </note>
    </measure>
  </part>
</score-partwise>
)";

  auto path = write_temp_xml("original.xml", xml);
  auto result = MusicXMLParser::parse(path);

  ASSERT_NE(result.original_xml, nullptr);

  // Verify we can access original XML structure
  auto root = result.original_xml->child("score-partwise");
  ASSERT_TRUE(root);

  auto work = root.child("work");
  ASSERT_TRUE(work);

  auto title = work.child("work-title");
  ASSERT_TRUE(title);
  EXPECT_STREQ(title.text().as_string(), "Original Title");
}

}  // namespace
}  // namespace piano_fingering::parser
