# Complexity.cmake - Code complexity analysis with lizard

find_program(LIZARD_EXECUTABLE NAMES lizard)

if(LIZARD_EXECUTABLE)
  add_custom_target(complexity
    COMMAND ${LIZARD_EXECUTABLE}
      -C 10
      -L 60
      -i 2
      ${CMAKE_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/include
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running lizard (CCN 10, length 60)"
  )

  add_custom_target(complexity-full
    COMMAND ${LIZARD_EXECUTABLE}
      -C 10
      -L 60
      -w
      ${CMAKE_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/include
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running lizard with warnings"
  )

  add_custom_target(complexity-xml
    COMMAND ${LIZARD_EXECUTABLE}
      -C 10
      -L 60
      --xml
      ${CMAKE_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/include
      > ${CMAKE_BINARY_DIR}/complexity-report.xml
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running lizard (XML output)"
  )
endif()
