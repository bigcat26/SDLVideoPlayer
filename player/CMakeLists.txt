project(player)

add_executable(${PROJECT_NAME} 
    main.c
    player.h
    media_decoder.c
    media_decoder.h
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<BOOL:${CONFIG_ENABLE_UI}>:CONFIG_ENABLE_UI=1>
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        $<$<BOOL:${CONFIG_ENABLE_UI}>:SDL2::SDL2>
        $<$<BOOL:${CONFIG_USE_CONAN_FFMPEG}>:ffmpeg::ffmpeg>
        # PkgConfig::avcodec
        # PkgConfig::avformat
        # PkgConfig::avutil
)
