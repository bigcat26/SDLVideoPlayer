cmake_minimum_required(VERSION 3.16)
project(VideoPlayer)

include(cmake/conan.cmake)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE)

option(CONFIG_ENABLE_UI "import sdl from conan" ON)
option(CONFIG_USE_CONAN_FFMPEG "import ffmpeg from conan" ON)

if(CONFIG_USE_CONAN_FFMPEG)
  set(CONAN_FFMPEG ffmpeg/4.4)
endif()
if(CONFIG_ENABLE_UI)
  set(CONAN_SDL sdl/2.0.20)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

conan_cmake_configure(
  REQUIRES
    ${CONAN_SDL}
    ${CONAN_FFMPEG}
    openssl/1.1.1o
    libiconv/1.17
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
                      # ffmpeg:with_libfdk_aac=True
                      ffmpeg:avdevice=False
                      ffmpeg:postproc=False
                      ffmpeg:avfilter=False
                      ffmpeg:swresample=False
                      ffmpeg:with_freetype=False
                      ffmpeg:with_xcb=False
                      ffmpeg:with_programs=False
                      ffmpeg:with_pulse=False
                      ffmpeg:with_libalsa=False
                      ffmpeg:with_libvpx=False
                      ffmpeg:with_vaapi=False
                      ffmpeg:with_vdpau=False
                      ffmpeg:with_vulkan=False
                      ffmpeg:with_libwebp=False
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

if(CONFIG_USE_CONAN_FFMPEG)
  find_package(ffmpeg REQUIRED)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(avcodec REQUIRED libavcodec IMPORTED_TARGET GLOBAL)
  pkg_check_modules(avformat REQUIRED libavformat IMPORTED_TARGET GLOBAL)
  pkg_check_modules(avutil REQUIRED libavutil IMPORTED_TARGET GLOBAL)
endif()

if(CONFIG_ENABLE_UI)
  find_package(SDL2 REQUIRED)
endif()

add_subdirectory(player)
