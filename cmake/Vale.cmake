# Vale.cmake - Vale documentation linting

find_program(VALE_EXECUTABLE NAMES vale)

if(VALE_EXECUTABLE)
  add_custom_target(vale
    COMMAND ${VALE_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running Vale on documentation"
  )
endif()
