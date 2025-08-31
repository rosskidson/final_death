cmake_minimum_required(VERSION 3.6)
project(platformer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(OpenGL_GL_PREFERENCE GLVND)

include(FetchContent)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

INCLUDE_DIRECTORIES(olcPixelGameEngine)
INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(json/single_include)

set(SOURCE_DIR "${CMAKE_SOURCE_DIR}")
configure_file(config.h.in config.h)

if(WIN32)
    FetchContent_Declare(
       json
       URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(json)

    cmake_policy(SET CMP0015 NEW)
else()
    FIND_PACKAGE(PNG REQUIRED)
    FIND_PACKAGE(OpenGL REQUIRED)
    FIND_PACKAGE(nlohmann_json REQUIRED)

    #LINUX
    #sudo apt install nlohmann-json3-dev

    MESSAGE("Building for linux...")
    SET(OLC_LIBS
        X11
        pthread
        PNG::PNG
        stdc++fs
        ${OPENGL_LIBRARIES}
        )
endif()


add_library(olc src/olc/olcPixelGameEngine.cc)
target_link_libraries(olc ${OLC_LIBS})

add_executable(platformer
  src/load_game_configuration.cc
  src/main.cc
  src/physics_engine.cc
  src/platformer.cc
  src/rendering_engine.cc
  src/tileset.cc
)

target_include_directories(platformer PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(platformer
  olc
  nlohmann_json::nlohmann_json
)
