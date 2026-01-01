#include "domain/piece.h"

#include <gtest/gtest.h>

#include <sstream>

namespace piano_fingering::domain {
namespace {

TEST(MetadataTest, Construct) {
  EXPECT_NO_THROW(Metadata("Title", "Composer"));
  EXPECT_NO_THROW(Metadata("", ""));
}

TEST(MetadataTest, Accessors) {
  Metadata md("Moonlight Sonata", "Beethoven");
  EXPECT_EQ(md.title(), "Moonlight Sonata");
  EXPECT_EQ(md.composer(), "Beethoven");
}

TEST(MetadataTest, Comparison) {
  Metadata md1("Title", "Composer");
  Metadata md2("Title", "Composer");
  Metadata md3("Other", "Composer");

  EXPECT_TRUE(md1 == md2);
  EXPECT_FALSE(md1 == md3);
  EXPECT_TRUE(md1 != md3);
}

TEST(MetadataTest, StreamOutput) {
  Metadata md("Title", "Composer");
  std::ostringstream oss;
  oss << md;
  std::string output = oss.str();
  EXPECT_TRUE(output.find("Title") != std::string::npos);
  EXPECT_TRUE(output.find("Composer") != std::string::npos);
}

TEST(PieceTest, ConstructValid) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);
  Measure m1(1, {s1}, ts);

  Metadata md("Title", "Composer");

  EXPECT_NO_THROW(Piece(md, {m1}, {m1}));
  EXPECT_NO_THROW(Piece(md, {m1}, {}));
  EXPECT_NO_THROW(Piece(md, {}, {m1}));
}

TEST(PieceTest, ConstructEmpty) {
  Metadata md("Title", "Composer");
  EXPECT_THROW(
      { [[maybe_unused]] Piece p(md, {}, {}); }, std::invalid_argument);
}

TEST(PieceTest, Accessors) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);
  Measure m1(1, {s1}, ts);
  Measure m2(2, {s1}, ts);

  Metadata md("Title", "Composer");
  Piece p(md, {m1, m2}, {m1});

  EXPECT_EQ(p.metadata().title(), "Title");
  EXPECT_EQ(p.left_hand().size(), 2);
  EXPECT_EQ(p.right_hand().size(), 1);
}

TEST(PieceTest, TotalMeasures) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);
  Measure m1(1, {s1}, ts);

  Metadata md("Title", "Composer");

  Piece p1(md, {m1, m1}, {m1});
  EXPECT_EQ(p1.total_measures(), 3);

  Piece p2(md, {m1}, {m1, m1, m1});
  EXPECT_EQ(p2.total_measures(), 4);
}

TEST(PieceTest, Empty) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);
  Measure m1(1, {s1}, ts);

  Metadata md("Title", "Composer");

  Piece p1(md, {m1}, {});
  EXPECT_FALSE(p1.empty());

  Piece p2(md, {}, {m1});
  EXPECT_FALSE(p2.empty());
}

TEST(PieceTest, HandAccess) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);
  Measure m1(1, {s1}, ts);
  Measure m2(2, {s1}, ts);

  Metadata md("Title", "Composer");
  Piece p(md, {m1}, {m2});

  EXPECT_EQ(p.left_hand()[0].number(), 1);
  EXPECT_EQ(p.right_hand()[0].number(), 2);
}

TEST(PieceTest, StreamOutput) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s1({n1});
  TimeSignature ts(4, 4);
  Measure m1(1, {s1}, ts);

  Metadata md("Title", "Composer");
  Piece p(md, {m1}, {m1});

  std::ostringstream oss;
  oss << p;
  std::string output = oss.str();
  EXPECT_TRUE(output.find("Piece") != std::string::npos);
}

}  // namespace
}  // namespace piano_fingering::domain
