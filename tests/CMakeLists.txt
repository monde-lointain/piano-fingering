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
)
target_include_directories(domain_test
  PRIVATE ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(domain_test
  PRIVATE GTest::gtest GTest::gtest_main
)
gtest_discover_tests(domain_test)
