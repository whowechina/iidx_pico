function(make_firmware board board_def)
    add_executable(${board}
                   main.c buttons.c rgb.c config.c turntable.c
                   tt_rainbow.c tt_blade.c
                   usb_descriptors.c)
    target_compile_definitions(${board} PUBLIC ${board_def})
    pico_enable_stdio_usb(${board} 1)
    pico_enable_stdio_uart(${board} 0)

    pico_generate_pio_header(${board} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
    target_compile_options(${board} PRIVATE -Wall -Werror -Wfatal-errors -O3)
    target_include_directories(${board} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_link_libraries(${board} PRIVATE
        pico_multicore pico_stdlib hardware_pio hardware_pwm hardware_flash
        hardware_i2c tinyusb_device tinyusb_board)

    # Make a UF2 binary
    pico_add_extra_outputs(${board})
endfunction()

make_firmware(iidx_pico BOARD_IIDX_PICO)
