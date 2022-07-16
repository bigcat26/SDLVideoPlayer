cmake_minimum_required(VERSION 3.21)
project(VideoPlayer)

include(cmake/conan.cmake)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

conan_cmake_configure(
  REQUIRES
    sdl/2.0.20
    ffmpeg/4.3.2
  GENERATORS 
    cmake_find_package
  IMPORTS
    lib,*.so*->${CMAKE_BINARY_DIR}/lib
)

conan_cmake_autodetect(settings)
conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    SETTINGS 
                      build_type=Release
                    OPTIONS
                      ffmpeg:with_libvpx=False
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

find_package(SDL2 REQUIRED)
find_package(ffmpeg REQUIRED)

add_subdirectory(player)