# Module: CLI (Command-Line Interface)

## Responsibilities

1. **Parse command-line arguments**: Flags, positional arguments, validation
2. **Orchestrate pipeline**: Parse → Configure → Optimize → Generate
3. **Implement progress observer**: Display measure progress to stderr
4. **Handle exit codes**: Map errors to codes 0-7 per SRS
5. **Detect TTY for color output**: Enable/disable ANSI color codes

---

## Data Ownership

### Runtime Configuration

| Data | Type | Source |
|------|------|--------|
| `input_path_` | `std::filesystem::path` | Positional argument 1 |
| `output_path_` | `std::filesystem::path` | Positional argument 2 (optional) |
| `mode_` | `OptimizationMode` | `--mode` flag |
| `preset_name_` | `std::string` | `--preset` flag |
| `custom_config_path_` | `std::optional<fs::path>` | `--config` flag |
| `seed_` | `std::optional<unsigned>` | `--seed` flag |
| `force_overwrite_` | `bool` | `--force` flag |
| `verbose_` | `bool` | `--verbose` flag |
| `quiet_` | `bool` | `--quiet` flag |

---

## Communication

### Inbound API

```cpp
// Entry point
int main(int argc, char* argv[]);

class CLI {
public:
  explicit CLI(int argc, char* argv[]);

  int run();

private:
  // Argument parsing
  void parse_arguments();
  void validate_arguments();

  // Pipeline orchestration
  int execute_pipeline();

  // Progress observer implementation
  class ProgressObserver : public IProgressObserver {
  public:
    void on_measure_complete(int current, int total) override;
    void on_phase_change(Phase phase) override;
    void on_optimization_complete(Score final_score) override;
  };

  // Utilities
  void print_usage() const;
  void print_version() const;
  void print_summary(const Piece& piece, Score left_score, Score right_score) const;
  bool is_tty(FILE* stream) const;

  // Configuration
  int argc_;
  char** argv_;
  RuntimeConfig config_;
  ProgressObserver observer_;
};
```

### Outbound Dependencies

- **Parser**: `MusicXMLParser::parse()`
- **Config**: `ConfigManager::load_preset()`, `ConfigManager::load_custom()`
- **Optimizer**: `Optimizer::optimize()`
- **Generator**: `MusicXMLGenerator::generate()`
- **Domain**: All types (passed between modules)

---

## Coupling Analysis

### Afferent Coupling

- **main()**: Entry point calls CLI constructor

### Efferent Coupling

- **All other modules**: CLI is the orchestrator (highest coupling by design)

**Instability:** High (intentional - CLI is the volatile "glue" layer)

---

## Testing Strategy

### Unit Tests

#### 1. Argument Parsing

```cpp
TEST(CLITest, ParseValidArguments) {
  const char* argv[] = {
    "piano-fingering",
    "--mode=quality",
    "--preset=Large",
    "--seed=12345",
    "input.musicxml",
    "output.musicxml"
  };
  int argc = sizeof(argv) / sizeof(argv[0]);

  CLI cli(argc, const_cast<char**>(argv));
  auto config = cli.get_config();

  EXPECT_EQ(config.mode, OptimizationMode::QUALITY);
  EXPECT_EQ(config.preset_name, "Large");
  EXPECT_EQ(config.seed, 12345);
  EXPECT_EQ(config.input_path, "input.musicxml");
  EXPECT_EQ(config.output_path, "output.musicxml");
}
```

#### 2. Default Output Filename

```cpp
TEST(CLITest, DefaultOutputFilename) {
  const char* argv[] = {"piano-fingering", "input.musicxml"};
  CLI cli(2, const_cast<char**>(argv));

  auto config = cli.get_config();
  EXPECT_EQ(config.output_path, "input_fingered.musicxml");
}
```

#### 3. Mutually Exclusive Flags

```cpp
TEST(CLITest, VerboseAndQuietMutuallyExclusive) {
  const char* argv[] = {"piano-fingering", "--verbose", "--quiet", "input.musicxml"};

  EXPECT_THROW(CLI(4, const_cast<char**>(argv)), ArgumentError);
}
```

#### 4. Unknown Flag

```cpp
TEST(CLITest, UnknownFlagThrows) {
  const char* argv[] = {"piano-fingering", "--unknown-flag", "input.musicxml"};

  EXPECT_THROW(CLI(3, const_cast<char**>(argv)), ArgumentError);
}
```

#### 5. Missing Required Argument

```cpp
TEST(CLITest, MissingInputFileThrows) {
  const char* argv[] = {"piano-fingering", "--mode=fast"};

  EXPECT_THROW(CLI(2, const_cast<char**>(argv)), ArgumentError);
}
```

### Integration Tests

#### 1. End-to-End Pipeline

```cpp
TEST(CLIIntegrationTest, FullPipelineOnGoldenSet) {
  const char* argv[] = {
    "piano-fingering",
    "--mode=balanced",
    "--seed=0",
    "tests/baseline/czerny_op821_1.musicxml",
    "/tmp/output.musicxml"
  };

  CLI cli(5, const_cast<char**>(argv));
  int exit_code = cli.run();

  EXPECT_EQ(exit_code, 0);
  EXPECT_TRUE(fs::exists("/tmp/output.musicxml"));

  // Verify output is valid MusicXML
  EXPECT_NO_THROW(MusicXMLParser::parse("/tmp/output.musicxml"));
}
```

#### 2. Help Flag

```cpp
TEST(CLIIntegrationTest, HelpFlagPrintsUsage) {
  const char* argv[] = {"piano-fingering", "--help"};

  testing::internal::CaptureStdout();
  CLI cli(2, const_cast<char**>(argv));
  int exit_code = cli.run();
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(exit_code, 0);
  EXPECT_THAT(output, HasSubstr("USAGE:"));
  EXPECT_THAT(output, HasSubstr("--mode"));
}
```

#### 3. Version Flag

```cpp
TEST(CLIIntegrationTest, VersionFlagPrintsVersion) {
  const char* argv[] = {"piano-fingering", "--version"};

  testing::internal::CaptureStdout();
  CLI cli(2, const_cast<char**>(argv));
  int exit_code = cli.run();
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(exit_code, 0);
  EXPECT_THAT(output, MatchesRegex("Piano Fingering Generator v[0-9]+\\.[0-9]+\\.[0-9]+\\+[a-f0-9]{7}"));
}
```

---

## Design Constraints

1. **POSIX compliance**: Standard exit codes, stderr for errors
2. **80-character line width**: Help text formatted for terminal readability
3. **Color detection**: Disable ANSI codes when piped to file
4. **No global state**: CLI object owns all state

---

## File Structure

```
src/cli/
  main.cpp                   // Entry point
  args.h                     // Argument parsing
  args.cpp
  progress_observer.h        // IProgressObserver implementation
  progress_observer.cpp
  orchestrator.h             // Pipeline execution
  orchestrator.cpp
```

---

## Critical Implementation Details

### Argument Parsing (Using getopt_long)

```cpp
void CLI::parse_arguments() {
  static struct option long_options[] = {
    {"help",    no_argument,       0, 'h'},
    {"version", no_argument,       0, 'v'},
    {"mode",    required_argument, 0, 'm'},
    {"preset",  required_argument, 0, 'p'},
    {"config",  required_argument, 0, 'c'},
    {"seed",    required_argument, 0, 's'},
    {"force",   no_argument,       0, 'f'},
    {"verbose", no_argument,       0, 0  },
    {"quiet",   no_argument,       0, 'q'},
    {0, 0, 0, 0}
  };

  int option_index = 0;
  int c;

  while ((c = getopt_long(argc_, argv_, "hvm:p:c:s:fq", long_options, &option_index)) != -1) {
    switch (c) {
      case 'h':
        print_usage();
        std::exit(0);
      case 'v':
        print_version();
        std::exit(0);
      case 'm':
        config_.mode = parse_mode(optarg);
        break;
      case 'p':
        config_.preset_name = optarg;
        break;
      // ... other cases
      case '?':
        throw ArgumentError("Unknown option. Use --help for usage information.");
    }
  }

  // Parse positional arguments
  if (optind < argc_) {
    config_.input_path = argv_[optind++];
  } else {
    throw ArgumentError("Missing required argument: input file");
  }

  if (optind < argc_) {
    config_.output_path = argv_[optind];
  } else {
    // Default output filename
    config_.output_path = generate_default_output_path(config_.input_path);
  }
}
```

### Default Output Filename

```cpp
fs::path generate_default_output_path(const fs::path& input) {
  fs::path stem = input.stem();  // "beethoven_op27"
  fs::path ext = input.extension();  // ".musicxml"
  return input.parent_path() / (stem.string() + "_fingered" + ext.string());
}
```

### Progress Observer Implementation

```cpp
class CLI::ProgressObserver : public IProgressObserver {
public:
  void on_measure_complete(int current, int total) override {
    if (quiet_) return;

    auto now = std::chrono::steady_clock::now();
    if (now - last_update_ < std::chrono::seconds(1)) return;  // Update at most every 1s

    std::cerr << "\rProcessing Measure [" << current << " / " << total << "]..." << std::flush;
    last_update_ = now;
  }

  void on_phase_change(Phase phase) override {
    if (quiet_) return;

    switch (phase) {
      case Phase::BEAM_SEARCH:
        std::cerr << "\nPhase 1: Beam Search DP\n";
        break;
      case Phase::ILS:
        std::cerr << "\nPhase 2: Iterated Local Search\n";
        break;
    }
  }

  void on_optimization_complete(Score final_score) override {
    if (quiet_) return;
    std::cerr << "\nOptimization complete. Final score: " << final_score << "\n";
  }

private:
  bool quiet_ = false;
  std::chrono::steady_clock::time_point last_update_;
};
```

### TTY Detection for Color Output

```cpp
bool CLI::is_tty(FILE* stream) const {
#ifdef _WIN32
  return _isatty(_fileno(stream));
#else
  return isatty(fileno(stream));
#endif
}

void CLI::print_error(const std::string& message) const {
  if (is_tty(stderr)) {
    std::cerr << "\033[31m";  // Red color
  }
  std::cerr << "piano-fingering: Error: " << message;
  if (is_tty(stderr)) {
    std::cerr << "\033[0m";  // Reset color
  }
  std::cerr << "\n";
}
```

### Pipeline Orchestration

```cpp
int CLI::execute_pipeline() {
  try {
    // 1. Parse input
    std::cerr << "Parsing " << config_.input_path << "...\n";
    auto parse_result = MusicXMLParser::parse(config_.input_path);

    // 2. Load configuration
    Config cfg = config_.custom_config_path.has_value()
      ? ConfigManager::load_custom(*config_.custom_config_path, config_.preset_name)
      : ConfigManager::load_preset(config_.preset_name);

    // 3. Determine seed
    unsigned seed = config_.seed.value_or(std::random_device{}());
    std::cerr << "Running with Seed: " << seed << "\n";

    // 4. Optimize left hand
    std::cerr << "\nOptimizing Left Hand...\n";
    Optimizer optimizer_left(cfg, &observer_);
    auto left_result = optimizer_left.optimize(parse_result.piece, Hand::LEFT, seed);

    // 5. Optimize right hand
    std::cerr << "\nOptimizing Right Hand...\n";
    Optimizer optimizer_right(cfg, &observer_);
    auto right_result = optimizer_right.optimize(parse_result.piece, Hand::RIGHT, seed);

    // 6. Generate output
    std::cerr << "\nGenerating output...\n";
    MusicXMLGenerator::generate(
      config_.output_path,
      *parse_result.original_xml,
      parse_result.piece,
      left_result.fingering,
      right_result.fingering,
      config_.force_overwrite
    );

    // 7. Print summary
    print_summary(parse_result.piece, left_result.score, right_result.score);

    return 0;

  } catch (const FileNotFoundError& e) {
    print_error(e.what());
    return 2;
  } catch (const ParsingError& e) {
    print_error(e.what());
    return 3;
  } catch (const ConfigurationError& e) {
    print_error(e.what());
    return 4;
  } catch (const FileExistsError& e) {
    print_error(e.what());
    return 5;
  } catch (const FileWriteError& e) {
    print_error(e.what());
    return 6;
  } catch (const std::exception& e) {
    print_error("Internal error: " + std::string(e.what()));
    return 7;
  }
}
```

### Summary Report

```cpp
void CLI::print_summary(const Piece& piece, Score left_score, Score right_score) const {
  if (config_.quiet) return;

  std::cout << "\n=== Fingering Generation Complete ===\n";
  std::cout << "Input:  " << config_.input_path << "\n";
  std::cout << "Output: " << config_.output_path << "\n";
  std::cout << "Seed:   " << (config_.seed.has_value() ? std::to_string(*config_.seed) : "random") << "\n";
  std::cout << "Mode:   " << mode_to_string(config_.mode) << "\n";
  std::cout << "Left Hand Difficulty Score:  " << left_score << "\n";
  std::cout << "Right Hand Difficulty Score: " << right_score << "\n";
  std::cout << "Total Difficulty Score:      " << (left_score + right_score) << "\n";
}
```

### Usage Text

```cpp
void CLI::print_usage() const {
  std::cout << R"(
USAGE: piano-fingering [OPTIONS] <input_file> [output_file]

Automatically generate optimal piano fingerings for MusicXML files.

ARGUMENTS:
  <input_file>        Path to MusicXML file to process
  [output_file]       Output path (default: <input>_fingered.musicxml)

OPTIONS:
  -h, --help          Display this help message
  -v, --version       Display version information
  -m, --mode <mode>   Optimization mode (fast|balanced|quality) [default: balanced]
  -p, --preset <name> Hand size preset (Small|Medium|Large) [default: Medium]
  -c, --config <path> Custom configuration JSON file
  -s, --seed <int>    Random seed for reproducibility
  -f, --force         Overwrite existing output file
      --verbose       Enable detailed logging
  -q, --quiet         Suppress non-error output

EXAMPLES:
  # Basic usage (default settings)
  piano-fingering input.musicxml

  # Use Large hand preset
  piano-fingering --preset=Large input.musicxml output.musicxml

  # Reproducible generation with seed
  piano-fingering --seed=12345 input.musicxml

  # Quality mode with verbose logging
  piano-fingering --mode=quality --verbose input.musicxml

For more information, visit: https://github.com/yourname/piano-fingering
)";
}
```

---

## Dependencies

- **All modules**: CLI orchestrates the entire pipeline
- **getopt_long**: POSIX argument parsing (stdlib)

---

## Error Handling & Exit Codes

| Exit Code | Condition | Exception Type | Example Message |
|-----------|-----------|---------------|-----------------|
| 0 | Success | N/A | N/A |
| 1 | Invalid CLI arguments | `ArgumentError` | `Error: Unknown option '--foo'. Use --help for usage.` |
| 2 | Input file I/O error | `FileNotFoundError` | `Error: Cannot open input file 'missing.xml': No such file` |
| 3 | XML parsing error | `ParsingError` | `Error: Invalid MusicXML at line 42: Unexpected closing tag` |
| 4 | Configuration error | `ConfigurationError` | `Configuration Error: MinPrac must be < MinComf for pair 1-2` |
| 5 | Output file exists | `FileExistsError` | `Error: Output file 'out.xml' exists. Use --force to overwrite.` |
| 6 | Output file I/O error | `FileWriteError` | `Error: Cannot write output file 'out.xml': Permission denied` |
| 7 | Internal algorithm error | `std::exception` | `Internal error: Assertion failed in optimizer.cpp:123` |

---

## Future Extensibility

- **Interactive mode**: Prompt for hand size, mode if not specified
- **JSON output**: Machine-readable summary for scripting
- **Batch processing**: Glob patterns for multiple files
- **Shell completion**: Auto-complete for flags (bash/zsh)
