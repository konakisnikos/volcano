###############################################################################
# Modern CMake minimum version and project definition
cmake_minimum_required(VERSION 3.16...3.27 FATAL_ERROR)
project(ElementalVolcano VERSION 0.1 LANGUAGES C CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ELEMENTAL_ENABLE_IMGUI "Enable Dear ImGui overlay" ON)

find_package(OpenGL REQUIRED)

# Prevent in-source builds
if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(FATAL_ERROR "Please use a separate build directory (e.g. build/)")
endif()

# External dependencies
add_subdirectory(external)

###############################################################################
# Dear ImGui (optional)
if (ELEMENTAL_ENABLE_IMGUI)
  include(FetchContent)
  FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
  )
  FetchContent_MakeAvailable(imgui)

  add_library(imgui_lib
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  )
  target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${CMAKE_SOURCE_DIR}/external/glew-1.13.0/include
    ${CMAKE_SOURCE_DIR}/external/glfw-3.2.1/include
  )
  # The executable already links GLFW. Keeping it off imgui_lib's transitive
  # interface avoids passing the same static archive to the final linker twice.
  target_link_libraries(imgui_lib PRIVATE OpenGL::GL)
  target_compile_definitions(imgui_lib PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
endif()

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
  common/light.cpp
  elemental/elements/Volcano.cpp
  elemental/elements/VolcanoNoise.cpp
  elemental/elements/Skybox.cpp
  elemental/SceneDirector.cpp
  elemental/CrackSystem.cpp
  elemental/IntParticleEmitter.cpp
  elemental/elements/SmokeEmitter.cpp
  elemental/elements/CloudEmitter.cpp
  elemental/elements/RainEmitter.cpp
  elemental/LightningSystem.cpp
  elemental/GeometryFactory.cpp
  elemental/InstancedPropRenderer.cpp

)

set(HDR
  common/util.h
  common/shader.h
  common/camera.h
  common/model.h
  common/texture.h
  common/light.h
  elemental/CrackSystem.h
  elemental/SceneDirector.h
  elemental/elements/Skybox.h
  elemental/elements/Volcano.h
  elemental/elements/VolcanoNoise.h
  elemental/IntParticleEmitter.h
  elemental/elements/SmokeEmitter.h
  elemental/elements/CloudEmitter.h
  elemental/elements/RainEmitter.h
  elemental/LightningSystem.h
  elemental/GeometryFactory.h
  elemental/InstancedPropRenderer.h
)



# Headers & shader files (added for IDE visibility only)
set(SHADERS
  elemental/shaders/Volcano.vertexshader
  elemental/shaders/Volcano.fragmentshader
  elemental/shaders/Skybox.vertexshader
  elemental/shaders/Skybox.fragmentshader
  elemental/shaders/ParticleShader.vertexshader
  elemental/shaders/ParticleShader.fragmentshader
  elemental/shaders/LightningFullscreen.vertexshader
  elemental/shaders/LightningBlur.fragmentshader
  elemental/shaders/LightningComposite.fragmentshader
  elemental/shaders/Depth.vertexshader
  elemental/shaders/Depth.fragmentshader
  elemental/shaders/Prop.vertexshader
  elemental/shaders/Prop.fragmentshader
  elemental/shaders/TreeBillboard.vertexshader
  elemental/shaders/TreeBillboard.fragmentshader
  elemental/shaders/Lightning.vertexshader
  elemental/shaders/Lightning.fragmentshader
)
add_executable(elemental ${SRC} ${HDR} ${SHADERS})

set_source_files_properties(${SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("shaders" FILES ${SHADERS})

source_group("Header Files" FILES ${HDR})

# Include & definitions
target_include_directories(elemental PRIVATE ${PROJECT_INCLUDE_DIRS})

target_compile_definitions(elemental PRIVATE
  TW_STATIC TW_NO_LIB_PRAGMA TW_NO_DIRECT3D GLEW_STATIC _CRT_SECURE_NO_WARNINGS
  ELEMENTAL_SHADER_DIR="${CMAKE_SOURCE_DIR}/elemental/shaders"
  ELEMENTAL_ASSET_DIR="${CMAKE_SOURCE_DIR}/elemental/assets"
  ELEMENTAL_SKYBOX_DIR="${CMAKE_SOURCE_DIR}/skybox_blue"
)

# Link libraries
target_link_libraries(elemental PRIVATE ${ALL_LIBS})

if (ELEMENTAL_ENABLE_IMGUI)
  target_sources(elemental PRIVATE
    elemental/StopwatchWidget.cpp
    elemental/StopwatchWidget.h
  )
  target_link_libraries(elemental PRIVATE imgui_lib)
  target_compile_definitions(elemental PRIVATE ELEMENTAL_ENABLE_IMGUI=1)
endif()

# Copy shaders to build directory for runtime
add_custom_command(TARGET elemental POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/Volcano.vertexshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/Volcano.fragmentshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/Skybox.vertexshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/Skybox.fragmentshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/ParticleShader.vertexshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/ParticleShader.fragmentshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/Depth.vertexshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/Depth.fragmentshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/Prop.vertexshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/Prop.fragmentshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/TreeBillboard.vertexshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/TreeBillboard.fragmentshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/Lightning.vertexshader $<TARGET_FILE_DIR:elemental>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/elemental/shaders/Lightning.fragmentshader $<TARGET_FILE_DIR:elemental>
)

# Grouping for IDE organization (optional)
source_group(common REGULAR_EXPRESSION ".*/common/.*")
source_group(obj REGULAR_EXPRESSION ".*/.*obj$")

###############################################################################
# Print summary
message(STATUS "Configured ElementalVolcano ${PROJECT_VERSION}")
message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
