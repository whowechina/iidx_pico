set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
set(LWIP_ROOT ${PICO_SDK_PATH}/lib/lwip)

function(make_firmware board board_def)
    pico_sdk_init()
    add_executable(${board}
                   main.c buttons.c rgb.c save.c config.c setup.c
                   turntable.c tt_rainbow.c tt_blade.c
                   usb_descriptors.c ble.c)
    target_compile_definitions(${board} PUBLIC ${board_def})
    pico_enable_stdio_usb(${board} 1)
    pico_enable_stdio_uart(${board} 0)

    pico_generate_pio_header(${board} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
    pico_btstack_make_gatt_header(${board} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/gamepad.gatt)

    target_compile_options(${board} PRIVATE -Wfatal-errors -O3)
    target_include_directories(${board} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_include_directories(${board} PRIVATE 
                               ${BTSTACK_ROOT}/src
                               ${LWIP_ROOT}/src/include)
    
    target_link_libraries(${board} PRIVATE
        pico_multicore pico_stdlib hardware_pio hardware_pwm hardware_flash
        hardware_adc hardware_i2c hardware_watchdog
        tinyusb_device tinyusb_board
        pico_cyw43_arch_none pico_btstack_ble pico_btstack_cyw43)

    pico_add_extra_outputs(${board})

    add_custom_command(TARGET ${board} POST_BUILD
                       COMMAND cp ${board}.uf2 /mnt/d/Code/iidx_pico/firmware/build)
endfunction()

make_firmware(iidx_pico BOARD_IIDX_PICO)

