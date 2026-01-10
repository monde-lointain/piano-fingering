#include "evaluator/score_evaluator.h"

#include <gtest/gtest.h>

#include "config/config.h"
#include "config/preset.h"
#include "domain/finger.h"
#include "domain/fingering.h"
#include "domain/hand.h"
#include "domain/measure.h"
#include "domain/metadata.h"
#include "domain/note.h"
#include "domain/piece.h"
#include "domain/pitch.h"
#include "domain/slice.h"

namespace piano_fingering::evaluator {
namespace {

using config::Config;
using domain::Finger;
using domain::Fingering;
using domain::Hand;
using domain::Measure;
using domain::Metadata;
using domain::Note;
using domain::Piece;
using domain::Pitch;
using domain::Slice;
using domain::TimeSignature;

// Helper to create non-rest note
Note make_note(int pitch_val, int octave) {
  return Note(Pitch(pitch_val), octave, 480, false, 1, 1);
}

// Helper to create rest
Note make_rest() { return Note(Pitch(0), 4, 480, true, 1, 1); }

TEST(ScoreEvaluatorTest, EvaluateEmptyPieceReturnsZero) {
  Config config{};
  ScoreEvaluator evaluator(config);

  // Single measure with rest only (no notes to evaluate)
  Piece piece(Metadata("Test", "Composer"), {},  // no left hand
              {Measure(1, {Slice({make_rest()})}, TimeSignature(4, 4))});

  std::vector<Fingering> fingerings;  // empty since no notes

  double score = evaluator.evaluate(piece, fingerings, Hand::kRight);
  EXPECT_DOUBLE_EQ(score, 0.0);
}

TEST(ScoreEvaluatorTest, EvaluateSingleNoteReturnsZero) {
  Config config{};
  ScoreEvaluator evaluator(config);

  // Single note, no transitions
  Piece piece(Metadata("Test", "Composer"), {},
              {Measure(1, {Slice({make_note(0, 4)})}, TimeSignature(4, 4))});

  std::vector<Fingering> fingerings = {Fingering({Finger::kThumb})};

  double score = evaluator.evaluate(piece, fingerings, Hand::kRight);
  EXPECT_DOUBLE_EQ(score, 0.0);  // No transitions, no penalties
}

TEST(ScoreEvaluatorTest, EvaluateTwoNotesAppliesRules) {
  Config config{};
  config.right_hand = config::make_medium_right_hand();
  config.weights = config::RuleWeights::defaults();
  ScoreEvaluator evaluator(config);

  // Two consecutive notes with larger distance to trigger Rule 2
  // Pitch 0 (C) octave 4 -> Pitch 0 (C) octave 5 = 14 semitones
  // For thumb-index (1-2), max_comf=8, so 14 > 8 triggers penalties
  Piece piece(Metadata("Test", "Composer"), {},
              {Measure(1, {Slice({make_note(0, 4)}), Slice({make_note(0, 5)})},
                       TimeSignature(4, 4))});

  // Fingers 1 -> 2 (thumb -> index) with large span
  std::vector<Fingering> fingerings = {Fingering({Finger::kThumb}),
                                       Fingering({Finger::kIndex})};

  double score = evaluator.evaluate(piece, fingerings, Hand::kRight);
  // Should have penalty > 0 from distance-based rules
  EXPECT_GT(score, 0.0);
}

TEST(ScoreEvaluatorTest, EvaluateSkipsRests) {
  Config config{};
  config.right_hand = config::make_medium_right_hand();
  config.weights = config::RuleWeights::defaults();
  ScoreEvaluator evaluator(config);

  // Note - Rest - Note (fingering vector should have 2 entries, not 3)
  Piece piece(Metadata("Test", "Composer"), {},
              {Measure(1,
                       {Slice({make_note(0, 4)}), Slice({make_rest()}),
                        Slice({make_note(0, 5)})},
                       TimeSignature(4, 4))});

  // Only 2 fingerings for 2 notes (rest skipped), using wide span
  std::vector<Fingering> fingerings = {Fingering({Finger::kThumb}),
                                       Fingering({Finger::kIndex})};

  double score = evaluator.evaluate(piece, fingerings, Hand::kRight);
  EXPECT_GT(score, 0.0);  // Should evaluate transition, rest ignored
}

TEST(ScoreEvaluatorTest, EvaluateThreeNotesApplies3NoteRules) {
  Config config{};
  config.right_hand = config::make_medium_right_hand();
  config.weights = config::RuleWeights::defaults();
  ScoreEvaluator evaluator(config);

  // Three consecutive notes with 4th finger to trigger Rule 5
  Piece piece(Metadata("Test", "Composer"), {},
              {Measure(1,
                       {Slice({make_note(0, 4)}), Slice({make_note(2, 4)}),
                        Slice({make_note(4, 4)})},
                       TimeSignature(4, 4))});

  std::vector<Fingering> fingerings = {
      Fingering({Finger::kThumb}),
      Fingering({Finger::kRing}),  // 4th finger triggers Rule 5
      Fingering({Finger::kPinky})};

  double score = evaluator.evaluate(piece, fingerings, Hand::kRight);
  EXPECT_GT(score, 0.0);  // Rule 5 applies to 4th finger
}

TEST(ScoreEvaluatorTest, EvaluateLeftHandUsesLeftHandMatrix) {
  Config config{};
  config.left_hand =
      config::mirror_to_left_hand(config::make_medium_right_hand());
  config.weights = config::RuleWeights::defaults();
  ScoreEvaluator evaluator(config);

  // Two notes in left hand with 4th finger
  Piece piece(Metadata("Test", "Composer"),
              {Measure(1, {Slice({make_note(0, 3)}), Slice({make_note(2, 3)})},
                       TimeSignature(4, 4))},
              {});

  std::vector<Fingering> fingerings = {
      Fingering({Finger::kRing}),  // 4th finger
      Fingering({Finger::kIndex})};

  double score = evaluator.evaluate(piece, fingerings, Hand::kLeft);
  EXPECT_GT(score, 0.0);  // Rule 5 applies
}

TEST(ScoreEvaluatorTest, EvaluateChordAppliesRule14) {
  Config config{};
  config.right_hand = config::make_medium_right_hand();
  config.weights = config::RuleWeights::defaults();
  ScoreEvaluator evaluator(config);

  // Single chord with 3 notes spanning >1 octave (stretch)
  // C4, C5 (octave), G5 (more than octave)
  Piece piece(
      Metadata("Test", "Composer"), {},
      {Measure(1, {Slice({make_note(0, 4), make_note(0, 5), make_note(8, 5)})},
               TimeSignature(4, 4))});

  // Fingering for chord: thumb, index, middle (tight fingers for wide span)
  std::vector<Fingering> fingerings = {
      Fingering({Finger::kThumb, Finger::kIndex, Finger::kMiddle})};

  double score = evaluator.evaluate(piece, fingerings, Hand::kRight);
  EXPECT_GT(score, 0.0);  // Rule 14: wide span with tight fingers
}

TEST(ScoreEvaluatorTest, EvaluateDeltaReturnsScoreDifference) {
  Config config{};
  config.right_hand = config::make_medium_right_hand();
  config.weights = config::RuleWeights::defaults();
  ScoreEvaluator evaluator(config);

  // Two consecutive notes
  Piece piece(Metadata("Test", "Composer"), {},
              {Measure(1, {Slice({make_note(0, 4)}), Slice({make_note(0, 5)})},
                       TimeSignature(4, 4))});

  // Current: thumb -> index
  std::vector<Fingering> current = {Fingering({Finger::kThumb}),
                                    Fingering({Finger::kIndex})};

  // Proposed: thumb -> middle (change at slice location: measure 0, slice 1,
  // note 0)
  std::vector<Fingering> proposed = {Fingering({Finger::kThumb}),
                                     Fingering({Finger::kMiddle})};

  ScoreEvaluator::SliceLocation changed_loc{0, 1, 0, 1};
  double delta = evaluator.evaluate_delta(piece, current, proposed, changed_loc,
                                          Hand::kRight);
  double old_score = evaluator.evaluate(piece, current, Hand::kRight);
  double new_score = evaluator.evaluate(piece, proposed, Hand::kRight);

  EXPECT_DOUBLE_EQ(delta, new_score - old_score);
}

TEST(ScoreEvaluatorTest, EvaluateDeltaWithChangedMiddleNote) {
  Config config{};
  config.right_hand = config::make_medium_right_hand();
  config.weights = config::RuleWeights::defaults();
  ScoreEvaluator evaluator(config);

  // Three notes to test middle change
  Piece piece(Metadata("Test", "Composer"), {},
              {Measure(1,
                       {Slice({make_note(0, 4)}), Slice({make_note(2, 4)}),
                        Slice({make_note(4, 4)})},
                       TimeSignature(4, 4))});

  // Current: 1-2-3
  std::vector<Fingering> current = {Fingering({Finger::kThumb}),
                                    Fingering({Finger::kIndex}),
                                    Fingering({Finger::kMiddle})};

  // Proposed: 1-4-3 (change middle note at slice location: measure 0, slice 1,
  // note 0)
  std::vector<Fingering> proposed = {Fingering({Finger::kThumb}),
                                     Fingering({Finger::kRing}),
                                     Fingering({Finger::kMiddle})};

  ScoreEvaluator::SliceLocation changed_loc{0, 1, 0, 1};
  double delta = evaluator.evaluate_delta(piece, current, proposed, changed_loc,
                                          Hand::kRight);
  double old_score = evaluator.evaluate(piece, current, Hand::kRight);
  double new_score = evaluator.evaluate(piece, proposed, Hand::kRight);

  EXPECT_DOUBLE_EQ(delta, new_score - old_score);
}

TEST(ScoreEvaluatorTest, EvaluateDeltaWithFirstNoteChange) {
  Config config{};
  config.right_hand = config::make_medium_right_hand();
  config.weights = config::RuleWeights::defaults();
  ScoreEvaluator evaluator(config);

  // Two notes
  Piece piece(Metadata("Test", "Composer"), {},
              {Measure(1, {Slice({make_note(0, 4)}), Slice({make_note(2, 4)})},
                       TimeSignature(4, 4))});

  // Current: 1-2
  std::vector<Fingering> current = {Fingering({Finger::kThumb}),
                                    Fingering({Finger::kIndex})};

  // Proposed: 2-2 (change first note at slice location: measure 0, slice 0,
  // note 0)
  std::vector<Fingering> proposed = {Fingering({Finger::kIndex}),
                                     Fingering({Finger::kIndex})};

  ScoreEvaluator::SliceLocation changed_loc{0, 0, 0, 0};
  double delta = evaluator.evaluate_delta(piece, current, proposed, changed_loc,
                                          Hand::kRight);
  double old_score = evaluator.evaluate(piece, current, Hand::kRight);
  double new_score = evaluator.evaluate(piece, proposed, Hand::kRight);

  EXPECT_DOUBLE_EQ(delta, new_score - old_score);
}

TEST(ScoreEvaluatorTest, EvaluateDeltaWithLastNoteChange) {
  Config config{};
  config.right_hand = config::make_medium_right_hand();
  config.weights = config::RuleWeights::defaults();
  ScoreEvaluator evaluator(config);

  // Two notes
  Piece piece(Metadata("Test", "Composer"), {},
              {Measure(1, {Slice({make_note(0, 4)}), Slice({make_note(2, 4)})},
                       TimeSignature(4, 4))});

  // Current: 1-2
  std::vector<Fingering> current = {Fingering({Finger::kThumb}),
                                    Fingering({Finger::kIndex})};

  // Proposed: 1-3 (change last note at slice location: measure 0, slice 1, note
  // 0)
  std::vector<Fingering> proposed = {Fingering({Finger::kThumb}),
                                     Fingering({Finger::kMiddle})};

  ScoreEvaluator::SliceLocation changed_loc{0, 1, 0, 1};
  double delta = evaluator.evaluate_delta(piece, current, proposed, changed_loc,
                                          Hand::kRight);
  double old_score = evaluator.evaluate(piece, current, Hand::kRight);
  double new_score = evaluator.evaluate(piece, proposed, Hand::kRight);

  EXPECT_DOUBLE_EQ(delta, new_score - old_score);
}

TEST(ScoreEvaluatorTest, EvaluateDeltaWithChordChange) {
  Config config{};
  config.right_hand = config::make_medium_right_hand();
  config.weights = config::RuleWeights::defaults();
  ScoreEvaluator evaluator(config);

  // Chord followed by single note
  Piece piece(Metadata("Test", "Composer"), {},
              {Measure(1,
                       {Slice({make_note(0, 4), make_note(4, 4)}),
                        Slice({make_note(7, 4)})},
                       TimeSignature(4, 4))});

  // Current: chord 1-3, then 5
  std::vector<Fingering> current = {
      Fingering({Finger::kThumb, Finger::kMiddle}),
      Fingering({Finger::kPinky})};

  // Proposed: chord 1-2, then 5 (change second note in chord at slice location:
  // measure 0, slice 0, note 1)
  std::vector<Fingering> proposed = {
      Fingering({Finger::kThumb, Finger::kIndex}), Fingering({Finger::kPinky})};

  ScoreEvaluator::SliceLocation changed_loc{0, 0, 1, 0};
  double delta = evaluator.evaluate_delta(piece, current, proposed, changed_loc,
                                          Hand::kRight);
  double old_score = evaluator.evaluate(piece, current, Hand::kRight);
  double new_score = evaluator.evaluate(piece, proposed, Hand::kRight);

  EXPECT_DOUBLE_EQ(delta, new_score - old_score);
}

}  // namespace
}  // namespace piano_fingering::evaluator
