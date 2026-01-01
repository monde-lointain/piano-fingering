# Documentation.cmake - Doxygen integration

find_package(Doxygen)

if(DOXYGEN_FOUND AND BUILD_DOCS)
  configure_file(
    ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in
    ${CMAKE_BINARY_DIR}/Doxyfile
    @ONLY
  )

  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating Doxygen documentation"
  )
endif()
