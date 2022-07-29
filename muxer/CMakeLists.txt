project(muxer)

add_executable(${PROJECT_NAME} 
    main.c
    muxer.c
    muxer.h
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        $<$<BOOL:${CONFIG_USE_CONAN_FFMPEG}>:ffmpeg::ffmpeg>
)
