# src/CMakeLists.txt - Main executable

add_executable(piano-fingering
  main.cpp
)

target_include_directories(piano-fingering
  PRIVATE ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(piano-fingering
  PRIVATE
    pugixml
    nlohmann_json::nlohmann_json
)

install(TARGETS piano-fingering
  RUNTIME DESTINATION bin
)
