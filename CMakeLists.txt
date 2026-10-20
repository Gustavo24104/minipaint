cmake_minimum_required(VERSION 4.3)
project(miniPaint C)

set(CMAKE_C_STANDARD 23)

add_executable(miniPaint main.c
        stb_image_write.h)
target_link_libraries(miniPaint PRIVATE glfw)
target_link_libraries(miniPaint PRIVATE GL)
target_link_libraries(miniPaint PRIVATE m)
