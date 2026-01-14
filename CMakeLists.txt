cmake_minimum_required(VERSION 3.6)
project(platformer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(OpenGL_GL_PREFERENCE GLVND)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

INCLUDE_DIRECTORIES(olcPixelGameEngine)
INCLUDE_DIRECTORIES(src)

set(SOURCE_DIR "${CMAKE_SOURCE_DIR}")
configure_file(config.h.in config.h)

# Miniaudio is used for sound and music.
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.23   # or latest stable tag
)
FetchContent_MakeAvailable(miniaudio)

# doctest is the unit testing framework used by this project.
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.12   # latest stable as of now
)
FetchContent_MakeAvailable(doctest)

if(WIN32)
    # In linux, nholmann json can be retrieved by a packet manager.
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
    INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR} ${SDL_MIXER_INCLUDE_DIRS})

    #LINUX
    #sudo apt install nlohmann-json3-dev libpng-dev libgl1-mesa-dev libglu1-mesa-dev

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

add_library(platformer_lib
  src/animation/animated_sprite.cc
  src/animation/simple_sprites.cc
  src/animation/sprite_manager.cc

  src/common_types/tileset.cc

  src/input/input_capture.cc
  src/input/input_processor.cc

  src/sound/sound_player.cc
  src/sound/sound_processor.cc

  src/systems/developer_console.cc
  src/systems/physics_system.cc
  src/systems/projectile_system.cc
  src/systems/player_logic_system.cc
  src/systems/rendering_system.cc

  src/utils/console_commands.cc

  src/load_game_configuration.cc
  src/platformer.cc
)

target_include_directories(platformer_lib
  PUBLIC 
  ${CMAKE_BINARY_DIR} 
  ${miniaudio_SOURCE_DIR}
)

target_link_libraries(platformer_lib
  olc
  nlohmann_json::nlohmann_json
)

add_executable(platformer src/main.cc)
target_link_libraries(platformer platformer_lib)

add_executable(test_test test/test_main.cc)
target_link_libraries(test_test PRIVATE doctest::doctest platformer_lib)
