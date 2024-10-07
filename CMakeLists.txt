cmake_minimum_required(VERSION 3.13)

include(external/pico-sdk/pico_sdk_init.cmake)

project(esc_sproj3)

pico_sdk_init()

add_executable(esc_sproj3 src/main.c)

target_link_libraries(esc_sproj3 PRIVATE pico_stdlib)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(esc_sproj3 pico_cyw43_arch_none)
endif()

pico_add_extra_outputs(esc_sproj3)

pico_enable_stdio_usb(esc_sproj3 0)
pico_enable_stdio_uart(esc_sproj3 1)
