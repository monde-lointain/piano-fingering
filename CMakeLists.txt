cmake_minimum_required(VERSION 3.20)

project(piano-fingering
  VERSION 1.0.0
  LANGUAGES CXX
)

# C++20 standard, no extensions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(STATIC_LINK_RELEASE "Static link in release builds" OFF)

# Include cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(Dependencies)
include(CompilerFlags)
include(StaticAnalysis)
include(CodeCoverage)
include(Formatting)
include(Complexity)
include(Cloc)
include(Documentation)
include(Packaging)
include(Vale)

# Static linking for release builds
if(STATIC_LINK_RELEASE AND CMAKE_BUILD_TYPE STREQUAL "Release")
  if(NOT MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  endif()
endif()

# Subdirectories
add_subdirectory(src)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
