# StaticAnalysis.cmake - Static analysis targets

# cppcheck target
find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)
if(CPPCHECK_EXECUTABLE)
  add_custom_target(cppcheck
    COMMAND ${CPPCHECK_EXECUTABLE}
      --enable=all
      --suppress=missingIncludeSystem
      --inline-suppr
      --std=c++20
      --error-exitcode=1
      ${CMAKE_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/include
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running cppcheck"
  )

  add_custom_target(cppcheck-xml
    COMMAND ${CPPCHECK_EXECUTABLE}
      --enable=all
      --suppress=missingIncludeSystem
      --inline-suppr
      --std=c++20
      --xml
      --xml-version=2
      ${CMAKE_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/include
      2> cppcheck-report.xml
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running cppcheck (XML output)"
  )
endif()

# clang-tidy target
find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
if(CLANG_TIDY_EXECUTABLE)
  add_custom_target(tidy
    COMMAND ${CLANG_TIDY_EXECUTABLE}
      -p ${CMAKE_BINARY_DIR}
      ${CMAKE_SOURCE_DIR}/src/*.cpp
      ${CMAKE_SOURCE_DIR}/include/*.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy"
  )
endif()

# scan-build target
find_program(SCAN_BUILD_EXECUTABLE NAMES scan-build)
if(SCAN_BUILD_EXECUTABLE)
  add_custom_target(scan-build
    COMMAND ${SCAN_BUILD_EXECUTABLE}
      -o ${CMAKE_BINARY_DIR}/scan-build-reports
      cmake --build ${CMAKE_BINARY_DIR} --clean-first
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running scan-build"
  )
endif()
