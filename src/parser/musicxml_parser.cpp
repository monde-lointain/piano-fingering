// src/parser/musicxml_parser.cpp - MusicXML parser implementation

#include "parser/musicxml_parser.h"

#include <fstream>
#include <iostream>
#include <vector>

#include "domain/measure.h"
#include "domain/metadata.h"
#include "domain/note.h"
#include "domain/piece.h"
#include "domain/slice.h"
#include "domain/time_signature.h"
#include "parser/pitch_mapping.h"

namespace piano_fingering::parser {

namespace {

// Extract metadata (title, composer) from XML root
domain::Metadata extract_metadata(const pugi::xml_node& root) {
  auto work = root.child("work");
  auto title_node = work.child("work-title");
  std::string title = title_node ? title_node.text().as_string() : "Untitled";

  auto identification = root.child("identification");
  auto creator =
      identification.find_child_by_attribute("creator", "type", "composer");
  std::string composer = creator ? creator.text().as_string() : "Unknown";

  return domain::Metadata(std::move(title), std::move(composer));
}

// Extract time signature from attributes node
domain::TimeSignature extract_time_signature(const pugi::xml_node& attributes) {
  auto time = attributes.child("time");
  if (!time) {
    return domain::common_time();  // Default to 4/4
  }

  int numerator = time.child("beats").text().as_int(4);
  int denominator = time.child("beat-type").text().as_int(4);

  return domain::TimeSignature(numerator, denominator);
}

// Extract single note from XML node
domain::Note extract_note(const pugi::xml_node& note_node) {
  // Check if it's a rest
  bool is_rest = note_node.child("rest") != nullptr;

  // Extract pitch (or default for rests)
  domain::Pitch pitch(0);
  int octave = 4;

  if (!is_rest) {
    auto pitch_node = note_node.child("pitch");
    if (!pitch_node) {
      throw MissingElementError("pitch");
    }

    auto step_node = pitch_node.child("step");
    if (!step_node) {
      throw MissingElementError("step");
    }

    std::string step = step_node.text().as_string();
    int alter = pitch_node.child("alter").text().as_int(0);
    octave = pitch_node.child("octave").text().as_int(4);

    pitch = step_alter_to_pitch(step, alter);
  }

  // Extract duration
  auto duration_node = note_node.child("duration");
  if (!duration_node) {
    throw MissingElementError("duration");
  }
  uint32_t duration = duration_node.text().as_uint(0);

  // Extract staff (default to 1)
  int staff = note_node.child("staff").text().as_int(1);

  // Extract voice (default to 1)
  int voice = note_node.child("voice").text().as_int(1);

  return domain::Note(pitch, octave, duration, is_rest, staff, voice);
}

// Container for measure data (both hands + metadata)
struct MeasureData {
  std::vector<domain::Slice> rh_slices;
  std::vector<domain::Slice> lh_slices;
  domain::TimeSignature time_sig{4, 4};
  int number = 1;
};

// Extract slices from a measure for a specific staff (helper)
std::vector<domain::Slice> extract_slices_for_staff(
    const pugi::xml_node& measure_node, int staff_filter) {
  std::vector<domain::Slice> slices;
  std::vector<domain::Note> current_chord;

  for (auto note_node : measure_node.children("note")) {
    try {
      domain::Note note = extract_note(note_node);

      // Filter by staff
      if (note.staff() != staff_filter) {
        continue;
      }

      // Check if this note is part of a chord
      bool is_chord = note_node.child("chord") != nullptr;

      if (is_chord) {
        // Add to current chord
        current_chord.push_back(std::move(note));
      } else {
        // Finalize previous chord if exists
        if (!current_chord.empty()) {
          slices.emplace_back(std::move(current_chord));
          current_chord.clear();
        }
        // Start new chord with this note
        current_chord.push_back(std::move(note));
      }
    } catch (const std::exception& e) {
      std::cerr << "Warning: Skipping note: " << e.what() << "\n";
    }
  }

  // Finalize last chord
  if (!current_chord.empty()) {
    slices.emplace_back(std::move(current_chord));
  }

  return slices;
}

// Extract measure data (both staves + metadata)
MeasureData extract_measure(const pugi::xml_node& measure_node,
                            domain::TimeSignature current_time_sig) {
  MeasureData data;

  // Extract measure number
  data.number = measure_node.attribute("number").as_int(1);

  // Check for time signature change
  auto attributes = measure_node.child("attributes");
  if (attributes) {
    data.time_sig = extract_time_signature(attributes);
  } else {
    data.time_sig = current_time_sig;
  }

  // Extract slices for both staves
  data.rh_slices = extract_slices_for_staff(measure_node, 1);
  data.lh_slices = extract_slices_for_staff(measure_node, 2);

  return data;
}

}  // namespace

MusicXMLParser::ParseResult MusicXMLParser::parse(
    const std::filesystem::path& xml_path) {
  // Load XML document
  auto doc = std::make_unique<pugi::xml_document>();
  pugi::xml_parse_result result = doc->load_file(xml_path.c_str());

  if (!result) {
    if (!std::filesystem::exists(xml_path)) {
      throw FileNotFoundError(xml_path.string());
    }
    throw MalformedXMLError(result.offset, result.description());
  }

  // Get root element
  auto root = doc->child("score-partwise");
  if (!root) {
    throw MissingElementError("score-partwise");
  }

  // Extract metadata
  domain::Metadata metadata = extract_metadata(root);

  // Extract measures for both hands
  std::vector<domain::Measure> left_hand_measures;
  std::vector<domain::Measure> right_hand_measures;

  auto part = root.child("part");
  if (!part) {
    throw MissingElementError("part");
  }

  domain::TimeSignature current_time_sig = domain::common_time();

  for (auto measure_node : part.children("measure")) {
    auto data = extract_measure(measure_node, current_time_sig);
    current_time_sig = data.time_sig;

    if (!data.rh_slices.empty()) {
      right_hand_measures.emplace_back(data.number, std::move(data.rh_slices),
                                       data.time_sig);
    }
    if (!data.lh_slices.empty()) {
      left_hand_measures.emplace_back(data.number, std::move(data.lh_slices),
                                      data.time_sig);
    }
  }

  // Build Piece
  domain::Piece piece(std::move(metadata), std::move(left_hand_measures),
                      std::move(right_hand_measures));

  return ParseResult{std::move(piece), std::move(doc)};
}

}  // namespace piano_fingering::parser
