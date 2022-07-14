project(player)

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL2::SDL2
        ffmpeg::ffmpeg
)
