#include "domain/fingering.h"

#include <gtest/gtest.h>

#include <sstream>

namespace piano_fingering::domain {
namespace {

TEST(FingeringTest, ConstructEmpty) {
  EXPECT_NO_THROW(Fingering());
  Fingering f;
  EXPECT_EQ(f.size(), 0);
  EXPECT_TRUE(f.empty());
}

TEST(FingeringTest, ConstructWithAssignments) {
  EXPECT_NO_THROW(Fingering({Finger::kThumb, std::nullopt, Finger::kPinky}));
  Fingering f({Finger::kThumb, std::nullopt, Finger::kPinky});
  EXPECT_EQ(f.size(), 3);
  EXPECT_FALSE(f.empty());
}

TEST(FingeringTest, Access) {
  Fingering f({Finger::kThumb, std::nullopt, Finger::kPinky});
  EXPECT_TRUE(f[0].has_value());
  EXPECT_EQ(f[0].value(), Finger::kThumb);
  EXPECT_FALSE(f[1].has_value());
  EXPECT_TRUE(f[2].has_value());
  EXPECT_EQ(f[2].value(), Finger::kPinky);
}

TEST(FingeringTest, ConstAccess) {
  const Fingering f({Finger::kThumb});
  EXPECT_TRUE(f[0].has_value());
}

TEST(FingeringTest, AccessOutOfBounds) {
  Fingering f({Finger::kThumb});
  EXPECT_THROW({ [[maybe_unused]] auto a = f[1]; }, std::out_of_range);
}

TEST(FingeringTest, IsCompleteAllAssigned) {
  Fingering f({Finger::kThumb, Finger::kIndex, Finger::kMiddle});
  EXPECT_TRUE(f.is_complete());
}

TEST(FingeringTest, IsCompleteNotAllAssigned) {
  Fingering f({Finger::kThumb, std::nullopt, Finger::kMiddle});
  EXPECT_FALSE(f.is_complete());
}

TEST(FingeringTest, IsCompleteEmpty) {
  Fingering f;
  EXPECT_TRUE(f.is_complete());
}

TEST(FingeringTest, ViolatesHardConstraintNoDuplicates) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Note n2(Pitch(7), 4, 240, false, 1, 1);
  Slice s({n1, n2});

  // Different fingers - no violation
  Fingering f1({Finger::kThumb, Finger::kIndex});
  EXPECT_FALSE(f1.violates_hard_constraint(s));

  // Same finger used twice - violation
  Fingering f2({Finger::kThumb, Finger::kThumb});
  EXPECT_TRUE(f2.violates_hard_constraint(s));
}

TEST(FingeringTest, ViolatesHardConstraintPartialAssignment) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Note n2(Pitch(7), 4, 240, false, 1, 1);
  Slice s({n1, n2});

  // One assigned, one null - no violation
  Fingering f1({Finger::kThumb, std::nullopt});
  EXPECT_FALSE(f1.violates_hard_constraint(s));

  // Two null - no violation
  Fingering f2({std::nullopt, std::nullopt});
  EXPECT_FALSE(f2.violates_hard_constraint(s));
}

TEST(FingeringTest, ViolatesHardConstraintSizeMismatch) {
  Note n1(Pitch(0), 4, 240, false, 1, 1);
  Slice s({n1});

  Fingering f({Finger::kThumb, Finger::kIndex});
  EXPECT_THROW(
      { [[maybe_unused]] bool v = f.violates_hard_constraint(s); },
      std::invalid_argument);
}

TEST(FingeringTest, Iteration) {
  Fingering f({Finger::kThumb, std::nullopt, Finger::kPinky});

  int count = 0;
  for (const auto& assignment : f) {
    ++count;
    [[maybe_unused]] bool has_val = assignment.has_value();
  }
  EXPECT_EQ(count, 3);
}

TEST(FingeringTest, StreamOutput) {
  Fingering f({Finger::kThumb, std::nullopt, Finger::kPinky});

  std::ostringstream oss;
  oss << f;
  std::string output = oss.str();
  EXPECT_TRUE(output.find("Fingering") != std::string::npos);
}

}  // namespace
}  // namespace piano_fingering::domain
