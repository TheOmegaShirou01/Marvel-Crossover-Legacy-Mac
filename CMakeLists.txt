# macOS (and Linux) build of the SDL3 front end. Windows uses Build_SDL.bat / Build_VS2022.bat.
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
# On macOS this makes "Marvel Crossover Legacy.app"; mac/bundle.sh then adds the libraries,
# MAME, Flycast and the icon (see .github/workflows/mac.yml).
cmake_minimum_required(VERSION 3.20)
project(MarvelCrossoverLegacy LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0)
endif()

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_mixer REQUIRED CONFIG)

set(MCL_SOURCES
  src/main_sdl.cpp src/title_screen.cpp src/gfx_sdl.cpp src/music_player_sdl.cpp
  src/gamepads_sdl.cpp src/earth_video_stub.cpp src/game_launcher.cpp src/mame_backend.cpp
  src/core.cpp src/cheats.cpp src/cheat_ex_settings.cpp src/external_roms.cpp src/emulator_bridge.cpp
  src/music.cpp src/pause_menu_lua.cpp src/achievement_lua.cpp src/accessibility_lua.cpp
  src/mvc2_onebutton.cpp src/autosave.cpp src/controls.cpp src/autoconfig.cpp src/app_settings.cpp
  src/lightning.cpp src/updater.cpp)

add_executable(mcl MACOSX_BUNDLE ${MCL_SOURCES})
target_link_libraries(mcl PRIVATE SDL3_ttf::SDL3_ttf SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer SDL3::SDL3)
if(APPLE)
  set_target_properties(mcl PROPERTIES
    OUTPUT_NAME "Marvel Crossover Legacy"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/mac/Info.plist.in")
else()
  set_target_properties(mcl PROPERTIES OUTPUT_NAME "MarvelCrossoverLegacy")
endif()
