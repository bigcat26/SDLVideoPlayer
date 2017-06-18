cmake_minimum_required(VERSION 3.6)
set(name video_player)
project (${name} C)

set ( CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
include( Universe )
include( FindSDL )
include( FindFFmpeg )

#
# set SDLDIR=E:\code\h264reader\sdl
# set FFMPEG_DIR=e:\code\ffmpeg
#

if (${SDL_FOUND})
  message("Found SDL: ${SDL_VERSION_STRING}")
  message("  SDL_LIBRARY:     ${SDL_LIBRARY}")
  message("  SDL_INCLUDE_DIR: ${SDL_INCLUDE_DIR}")
  include_directories(${SDL_INCLUDE_DIR})
else ()
  message(FATAL_ERROR "SDL not found!")
endif ()


if (${FFMPEG_FOUND})
  message("Found FFmpeg:")
  message("  FFMPEG_LIBRARIES:   ${FFMPEG_LIBRARIES}")
  message("  FFMPEG_INCLUDE_DIR: ${FFMPEG_INCLUDE_DIR}")
  include_directories(${SDL_INCLUDE_DIR})
else ()
  message(FATAL_ERROR "FFmpeg not found!")
endif ()

file (GLOB_RECURSE src src/*.c inc/*.h)

include_directories(${SDL_INCLUDE_DIR})
include_directories(${FFMPEG_INCLUDE_DIR})

add_executable(${name} ${src})
target_link_libraries(${name} ${SDL_LIBRARY} ${FFMPEG_LIBRARIES})

if (WIN32)
  target_link_libraries(${name} ws2_32 secur32)
endif ()

#add_library(${name} STATIC ${src} ${hdr})
#set_target_properties(${name} PROPERTIES PREFIX "lib")
