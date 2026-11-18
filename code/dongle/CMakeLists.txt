cmake_minimum_required(VERSION 3.20.0)
list(APPEND BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND EXTRA_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../common/radio.conf")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(headphones_dongle)

target_compile_definitions(app PRIVATE HEADPHONES_DONGLE)
target_include_directories(app PRIVATE ../common)
target_sources(app PRIVATE
    src/main.c
    src/usb_audio.c
    src/usb_rgb.c
    ../common/audio.c
    ../common/audio_fifo.c
    ../common/radio.c
    ../common/rgb_frame.c
)
