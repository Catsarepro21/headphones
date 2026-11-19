cmake_minimum_required(VERSION 3.20.0)
list(APPEND BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND EXTRA_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../common/radio.conf")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(headphone_headset)
target_sources(app PRIVATE
    src/main.c
    src/headset_logic.c
    src/battery.c
    src/bm83_link.c
    src/bm83_protocol.c
    src/codec.c
    src/rgb.c
    src/wireless_audio.c
    ../common/audio.c
    ../common/audio_fifo.c
    ../common/radio.c
    ../common/rgb_frame.c
)
target_include_directories(app PRIVATE ../common)
