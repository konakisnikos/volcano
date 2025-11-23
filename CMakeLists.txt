###############################################################################
# Modern CMake minimum version and project definition
cmake_minimum_required(VERSION 3.16...3.27 FATAL_ERROR)
project(ElementalVolcano VERSION 0.1 LANGUAGES C CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)

# Prevent in-source builds
if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(FATAL_ERROR "Please use a separate build directory (e.g. build/)")
endif()

# External dependencies
add_subdirectory(external)

# Include directories (use target_include_directories later, keep variable for brevity)
set(PROJECT_INCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/external/glfw-3.2.1/include/GLFW
  ${CMAKE_SOURCE_DIR}/external/glm-0.9.7.1
  ${CMAKE_SOURCE_DIR}/external/glew-1.13.0/include
  ${CMAKE_SOURCE_DIR}/external/Simple-OpenGL-Image-Library/include
  ${CMAKE_SOURCE_DIR}/external/tinyxml2/include
  ${CMAKE_SOURCE_DIR}/external/tinyobjloader/include
  ${CMAKE_SOURCE_DIR}
)

# Libraries
set(ALL_LIBS
  OpenGL::GL
  glfw
  GLEW_1130
  SOIL
  TINYXML2
)

###############################################################################
# Sources
set(SRC
  elemental/main.cpp
  common/util.cpp
  common/shader.cpp
  common/camera.cpp
  common/model.cpp
  common/texture.cpp
  common/Volcano.cpp
)

add_executable(elemental ${SRC})

# Headers & shader files (added for IDE visibility only)
set(SHADERS
  elemental/Volcano.vertexshader
  elemental/Volcano.fragmentshader
)
set_source_files_properties(${SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("shaders" FILES ${SHADERS})

# Include & definitions
target_include_directories(elemental PRIVATE ${PROJECT_INCLUDE_DIRS})

target_compile_definitions(elemental PRIVATE
  TW_STATIC TW_NO_LIB_PRAGMA TW_NO_DIRECT3D GLEW_STATIC _CRT_SECURE_NO_WARNINGS
)

# Link libraries
target_link_libraries(elemental PRIVATE ${ALL_LIBS})

# Copy shaders to build directory for runtime
add_custom_command(TARGET elemental POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/Volcano.vertexshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/Volcano.fragmentshader $<TARGET_FILE_DIR:elemental>
)

# Grouping for IDE organization (optional)
source_group(common REGULAR_EXPRESSION ".*/common/.*")
source_group(obj REGULAR_EXPRESSION ".*/.*obj$")

###############################################################################
# Print summary
message(STATUS "Configured ElementalVolcano ${PROJECT_VERSION}")
message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
