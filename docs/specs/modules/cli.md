# Module Specification: CLI

## Responsibilities

- Parse command-line arguments (flags, positional args)
- Validate argument combinations (mutually exclusive flags)
- Orchestrate the full pipeline (Config → Parse → Optimize → Write)
- Return appropriate exit codes (0-7)
- Handle `--help` and `--version` flags
- Report errors to stderr

---

## Data Ownership

### Primary Data Structure: `Options`

```cpp
struct Options {
    std::string input_file;          // Required positional arg
    std::string output_file;         // Optional (default: input + "_fingered")

    OptimizationMode mode;           // fast | balanced | quality
    std::string preset;              // Small | Medium | Large
    std::optional<std::string> config_path;  // Custom JSON config

    std::optional<uint32_t> seed;    // RNG seed (random if not set)

    bool force;                      // Overwrite existing output
    bool verbose;                    // Detailed logging
    bool quiet;                      // Suppress all output except errors

    bool help;                       // Print usage and exit
    bool version;                    // Print version and exit
};
```

### Exit Codes (from SRS §3.5, FR-5.10)

```cpp
enum class ExitCode : int {
    Success = 0,
    InvalidArguments = 1,
    InputFileError = 2,
    XMLParseError = 3,
    ConfigError = 4,
    OutputFileExists = 5,
    OutputFileError = 6,
    InternalAlgorithmError = 7
};
```

---

## Communication

### Incoming Dependencies (Afferent)

| Caller | Input |
|--------|-------|
| `main()` | `int argc, char** argv` |

### Outgoing Dependencies (Efferent)

| Module | API Call |
|--------|----------|
| `config` | `Config LoadConfig(preset, config_path)` |
| `musicxml` | `Piece ParseMusicXML(input_file)` |
| `engine` | `Solution Optimize(piece, config, mode, seed, reporter)` |
| `musicxml` | `WriteMusicXML(output_file, piece, solution, force)` |

**Dependency Direction**: CLI → All modules (CLI is the orchestrator, no module depends on CLI)

---

## Coupling

### Afferent Coupling (Incoming)
- `main()` only

**Count**: 1 (low)

### Efferent Coupling (Outgoing)
- `config` module
- `musicxml` module (parser + writer)
- `engine` module
- `common` types (`Hand`, `Finger`, `Pitch`)

**Count**: 4 (acceptable for orchestrator)

**Note**: This is expected for the "Orchestrator" layer in a layered architecture. CLI has high efferent coupling but zero afferent coupling (only called by main).

---

## API Specification

### Primary Entry Point

```cpp
// In src/cli/cli.h
namespace cli {

// Parse command-line arguments
// Returns: Options struct (throws std::invalid_argument on error)
Options ParseArguments(int argc, char** argv);

// Main orchestration function
// Returns: ExitCode
ExitCode Run(const Options& options);

}  // namespace cli
```

### Helper Functions

```cpp
namespace cli {

// Print usage instructions to stdout (FR-5.3, FR-5.4)
void PrintUsage(const char* program_name);

// Print version information to stdout (FR-5.5, FR-5.6)
void PrintVersion();

// Generate default output filename (FR-4.6, FR-4.7)
// Example: "input.musicxml" → "input_fingered.musicxml"
std::string GenerateOutputFilename(const std::string& input_file);

// Validate argument combinations (FR-5.8)
// Throws: std::invalid_argument if conflicts detected
void ValidateOptions(const Options& options);

}  // namespace cli
```

---

## Testing Strategy

### Unit Tests

| Test Case | Scenario | Expected Outcome |
|-----------|----------|------------------|
| `ParseHelp` | `--help` flag present | `options.help == true`, no exceptions |
| `ParseVersion` | `--version` flag present | `options.version == true` |
| `ParseMinimalArgs` | Only input file provided | Output = input + "_fingered", mode = balanced, preset = Medium |
| `ParseAllFlags` | All flags specified | All options set correctly |
| `ParseShortFlags` | `-h -v -m fast -p Small` | Equivalent to long forms |
| `ParseSeedFlag` | `--seed=12345` | `options.seed == 12345` |
| `ParseMutexFlags` | `--verbose --quiet` | Throws `std::invalid_argument` |
| `ParseUnknownFlag` | `--invalid-flag` | Throws `std::invalid_argument` |
| `GenerateOutputFilename` | `input.musicxml` | `input_fingered.musicxml` |
| `GenerateOutputFilename` | `path/to/file.xml` | `path/to/file_fingered.xml` |
| `ValidateConflicts` | `verbose=true, quiet=true` | Throws exception |

**Implementation**: Google Test framework

```cpp
TEST(CLITest, ParseHelp) {
    char* argv[] = {(char*)"piano-fingering", (char*)"--help"};
    auto options = cli::ParseArguments(2, argv);
    EXPECT_TRUE(options.help);
}

TEST(CLITest, ParseMutexFlagsThrows) {
    char* argv[] = {(char*)"piano-fingering", (char*)"--verbose", (char*)"--quiet", (char*)"input.xml"};
    EXPECT_THROW(cli::ParseArguments(4, argv), std::invalid_argument);
}
```

### Integration Tests

| Test Case | Scenario | Verification |
|-----------|----------|--------------|
| `GoldenPipeline` | Full run on Czerny Op. 821 No. 1 | Exit code 0, output file exists, parseable MusicXML |
| `InvalidInputFile` | Non-existent file | Exit code 2, stderr contains "Cannot open input file" |
| `MalformedXML` | Corrupted MusicXML | Exit code 3, stderr contains "Invalid MusicXML" |
| `InvalidConfig` | MinPrac > MinComf | Exit code 4, stderr contains "Configuration Error" |
| `OutputExists` | Target file exists, no --force | Exit code 5, stderr contains "already exists. Use --force" |
| `OutputExistsForce` | Target exists, --force present | Exit code 0, file overwritten |
| `HelpExits` | `--help` flag | Exit code 0, stdout contains usage instructions |
| `VersionExits` | `--version` flag | Exit code 0, stdout contains version + commit hash |

**Implementation**: Shell script test harness

```bash
#!/bin/bash
# test_integration.sh

# Test: Help flag exits cleanly
./piano-fingering --help > /tmp/out.txt
if [ $? -ne 0 ]; then
    echo "FAIL: --help should exit 0"
    exit 1
fi
if ! grep -q "USAGE:" /tmp/out.txt; then
    echo "FAIL: --help should print usage"
    exit 1
fi
echo "PASS: --help"

# Test: Invalid file returns code 2
./piano-fingering nonexistent.xml 2> /tmp/err.txt
if [ $? -ne 2 ]; then
    echo "FAIL: Nonexistent file should exit 2"
    exit 1
fi
if ! grep -q "Cannot open input file" /tmp/err.txt; then
    echo "FAIL: Should print error message"
    exit 1
fi
echo "PASS: Invalid input file"
```

---

## Implementation Notes

### Argument Parsing Library

**Options Considered**:
1. **Hand-rolled parser** (simple, no dependencies)
2. **GNU getopt** (C-style, platform-specific)
3. **CLI11** (modern C++, header-only)

**Decision**: Hand-rolled parser using a simple state machine.

**Rationale**:
- Small number of flags (~10 options)
- No complex subcommands
- Avoid adding another dependency
- Minimal code (~150 lines)

### Error Message Format (UI-1.6, UI-1.7)

```cpp
// Errors:  piano-fingering: Error: <component>: <message>
std::cerr << program_name << ": Error: " << component << ": " << message << '\n';

// Warnings: piano-fingering: Warning: <message>
std::cerr << program_name << ": Warning: " << message << '\n';
```

### Color Support (UI-1.4, UI-1.5)

```cpp
bool IsTerminal() {
#ifdef _WIN32
    return _isatty(_fileno(stderr));
#else
    return isatty(fileno(stderr));
#endif
}

void PrintError(const std::string& message) {
    if (IsTerminal()) {
        std::cerr << "\033[31mError:\033[0m " << message << '\n';  // Red
    } else {
        std::cerr << "Error: " << message << '\n';  // No color
    }
}
```

### Progress Reporting (FR-6.1, FR-6.2, FR-6.3)

```cpp
class ConsoleProgressReporter : public ProgressReporter {
public:
    ConsoleProgressReporter(bool quiet) : quiet_(quiet), last_update_(0) {}

    void ReportProgress(int current, int total) override {
        if (quiet_) return;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_).count();

        // Update at most once per second (FR-6.3)
        if (elapsed >= 1000) {
            std::cerr << "Processing Measure [" << current << " / " << total << "]...\n";
            last_update_ = now;
        }
    }

private:
    bool quiet_;
    std::chrono::steady_clock::time_point last_update_;
};
```

---

## File Structure

```
src/cli/
├── cli.h              // Public API (ParseArguments, Run)
├── cli.cpp            // Argument parsing logic
├── orchestrator.cpp   // Pipeline orchestration (Run function)
└── progress.h         // ConsoleProgressReporter implementation
```

---

## Dependencies on Other Modules

| Module | Header Included | Usage |
|--------|-----------------|-------|
| `config` | `config/config.h` | Load presets, parse JSON |
| `musicxml` | `musicxml/parser.h`, `musicxml/writer.h` | Parse/write MusicXML |
| `engine` | `engine/optimizer.h` | Run Beam DP + ILS |
| `common` | `common/types.h` | `Hand`, `Finger`, `Pitch` enums |

---

## Non-Functional Requirements

### Performance
- Argument parsing must complete in <10ms
- No significant overhead (pipeline orchestration is I/O bound)

### Usability (USE-1.1)
- Users must successfully generate first fingering within 5 minutes of reading `--help`
- Help text formatted for 80-column terminals
- At least 3 example invocations (FR-5.3, USE-1.3)

**Example Help Output**:

```
USAGE:
  piano-fingering [OPTIONS] <input_file> [output_file]

OPTIONS:
  -h, --help            Display this help message
  -v, --version         Display version information
  -m, --mode=MODE       Optimization mode: fast | balanced | quality (default: balanced)
  -p, --preset=PRESET   Hand size: Small | Medium | Large (default: Medium)
  -c, --config=PATH     Custom JSON configuration file
  -s, --seed=SEED       RNG seed for reproducible results
  -f, --force           Overwrite existing output file
      --verbose         Enable detailed logging
  -q, --quiet           Suppress all non-error output

EXAMPLES:
  # Basic usage (output: input_fingered.musicxml)
  piano-fingering input.musicxml

  # Custom output file with small hand preset
  piano-fingering input.musicxml output.musicxml --preset=Small

  # Reproducible generation with fixed seed
  piano-fingering input.musicxml --seed=12345 --mode=quality
```

---

## Critical Paths

1. **Argument Parsing**: `main() → ParseArguments() → ValidateOptions()`
2. **Pipeline Execution**: `Run() → LoadConfig() → ParseMusicXML() → Optimize() → WriteMusicXML()`
3. **Error Handling**: Any module exception → Catch in Run() → Map to ExitCode → Print to stderr

---

## Success Criteria

- ✅ 100% of error conditions return correct exit code
- ✅ `--help` output includes 3+ examples
- ✅ Mutually exclusive flags detected and rejected
- ✅ Default output filename follows `<input>_fingered.<ext>` convention
- ✅ Progress updates printed to stderr every ≤1 second
- ✅ Color codes disabled when stderr is redirected to file
