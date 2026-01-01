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
