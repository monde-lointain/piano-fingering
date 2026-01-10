#ifndef PIANO_FINGERING_PARSER_PARSER_ERROR_H_
#define PIANO_FINGERING_PARSER_PARSER_ERROR_H_

#include <stdexcept>
#include <string>

namespace piano_fingering::parser {

class ParserError : public std::runtime_error {
 public:
  explicit ParserError(const std::string& message)
      : std::runtime_error(message) {}
};

class FileNotFoundError : public ParserError {
 public:
  explicit FileNotFoundError(const std::string& path)
      : ParserError("Cannot open input file '" + path + "'") {}
};

class MalformedXMLError : public ParserError {
 public:
  MalformedXMLError(int line, const std::string& detail)
      : ParserError("Invalid MusicXML at line " + std::to_string(line) + ": " +
                    detail) {}
};

class MissingElementError : public ParserError {
 public:
  explicit MissingElementError(const std::string& element)
      : ParserError("Missing required element: " + element) {}
};

}  // namespace piano_fingering::parser

#endif  // PIANO_FINGERING_PARSER_PARSER_ERROR_H_
