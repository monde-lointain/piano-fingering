# src/CMakeLists.txt - Main executable

# Config library
add_library(config STATIC
  config/embedded_presets.cpp
  config/config_manager.cpp
)

target_include_directories(config
  PUBLIC ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(config
  PUBLIC nlohmann_json::nlohmann_json
)

# Main executable
add_executable(piano-fingering
  main.cpp
)

target_include_directories(piano-fingering
  PRIVATE ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(piano-fingering
  PRIVATE
    config
    pugixml
    nlohmann_json::nlohmann_json
)

install(TARGETS piano-fingering
  RUNTIME DESTINATION bin
)
