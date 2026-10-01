cmake_minimum_required(VERSION 3.13)

add_library(pico_webhook STATIC
    src/pico_webhook.c
)

target_sources(pico_webhook PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/src/pico_webhook.c
)

target_include_directories(pico_webhook PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Pull in the required lwIP and mbedtls libraries exactly as the main project does
target_link_libraries(pico_webhook PUBLIC
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mbedtls
    pico_mbedtls
)
