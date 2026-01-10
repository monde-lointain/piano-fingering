# tests/CMakeLists.txt - Unit tests

include(GoogleTest)

add_executable(placeholder_test
  placeholder_test.cpp
)

target_link_libraries(placeholder_test
  PRIVATE
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(placeholder_test)

# Domain module tests
add_executable(domain_test
  domain/hand_test.cpp
  domain/finger_test.cpp
  domain/pitch_test.cpp
  domain/note_test.cpp
  domain/slice_test.cpp
  domain/time_signature_test.cpp
  domain/measure_test.cpp
  domain/piece_test.cpp
  domain/fingering_test.cpp
)
target_include_directories(domain_test
  PRIVATE ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(domain_test
  PRIVATE GTest::gtest GTest::gtest_main
)
gtest_discover_tests(domain_test)

# Config module tests
add_executable(config_test
  config/finger_pair_test.cpp
  config/configuration_error_test.cpp
  config/finger_pair_distances_test.cpp
  config/distance_matrix_test.cpp
  config/rule_weights_test.cpp
  config/algorithm_parameters_test.cpp
  config/config_test.cpp
  config/preset_test.cpp
  config/config_manager_test.cpp
)
target_include_directories(config_test
  PRIVATE ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(config_test
  PRIVATE
    config
    GTest::gtest
    GTest::gtest_main
)
gtest_discover_tests(config_test)

# Evaluator module tests
add_executable(evaluator_test
  evaluator/score_evaluator_test.cpp
  evaluator/rules_test.cpp
)
target_include_directories(evaluator_test
  PRIVATE ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(evaluator_test
  PRIVATE
    evaluator
    GTest::gtest
    GTest::gtest_main
)
gtest_discover_tests(evaluator_test)

# Parser module tests
add_executable(parser_test
  parser/pitch_mapping_test.cpp
  parser/musicxml_parser_test.cpp
  parser/golden_set_test.cpp
)
target_include_directories(parser_test
  PRIVATE ${CMAKE_SOURCE_DIR}/include
)
target_compile_definitions(parser_test
  PRIVATE BASELINE_DIR="${CMAKE_SOURCE_DIR}/tests/baseline"
)
target_link_libraries(parser_test
  PRIVATE
    parser
    GTest::gtest
    GTest::gtest_main
)
gtest_discover_tests(parser_test)
