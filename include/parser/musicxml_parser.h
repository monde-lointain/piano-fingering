#ifndef PIANO_FINGERING_PARSER_MUSICXML_PARSER_H_
#define PIANO_FINGERING_PARSER_MUSICXML_PARSER_H_

#include <filesystem>
#include <memory>

#include <pugixml.hpp>

#include "domain/piece.h"
#include "parser/parser_error.h"

namespace piano_fingering::parser {

class MusicXMLParser {
 public:
  struct ParseResult {
    domain::Piece piece;
    std::unique_ptr<pugi::xml_document> original_xml;
  };

  MusicXMLParser() = delete;

  [[nodiscard]] static ParseResult parse(const std::filesystem::path& xml_path);
};

}  // namespace piano_fingering::parser

#endif  // PIANO_FINGERING_PARSER_MUSICXML_PARSER_H_
