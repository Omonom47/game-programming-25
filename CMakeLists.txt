cmake_minimum_required(VERSION 3.12)

project(gp25_exam_project)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

FILE(COPY data DESTINATION ${CMAKE_BINARY_DIR})


set(SDLMIXER_VENDORED OFF)

# 3rd party libraries
add_subdirectory(lib/SDL)
add_subdirectory(lib/SDL_mixer)
add_subdirectory(lib/SDL_ttf)
add_subdirectory(lib/box2d)
# changed imgui to be compiled as part of the game executable
add_subdirectory(lib/imgui)

# our libraries
include_directories(lib/stb)

# repo's executables
add_subdirectory(exam_project)
