set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
set(LWIP_ROOT ${PICO_SDK_PATH}/lib/lwip)

function(make_firmware board board_def)
    pico_sdk_init()
    add_executable(${board}
                   main.c cli.c commands.c buttons.c hebtn.c rgb.c savedata.c
                   config.c setup.c turntable.c as5600.c mt6701.c tmag5273.c
                   tt_rainbow.c tt_blade.c tt_heatbar.c
                   usb_descriptors.c bt.c)


    target_compile_definitions(${board} PUBLIC ${board_def}
                               PICO_XOSC_STARTUP_DELAY_MULTIPLIER=20)

    target_compile_definitions(${board} PUBLIC ${board_def})
    pico_enable_stdio_usb(${board} 1)
    pico_enable_stdio_uart(${board} 0)

    pico_generate_pio_header(${board} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

    target_compile_options(${board} PRIVATE -Wfatal-errors -O3)
    target_include_directories(${board} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_include_directories(${board} PRIVATE 
                               ${BTSTACK_ROOT}/src
                               ${LWIP_ROOT}/src/include)
    
    target_link_libraries(${board} PRIVATE
        pico_multicore pico_stdlib hardware_pio hardware_pwm hardware_flash
        hardware_adc hardware_i2c hardware_watchdog
        tinyusb_device tinyusb_board)

    # Only boards with a CYW43 (Pico W) provide these targets, the same
    # firmware still runs on a plain Pico, see bt_init()
    if (TARGET pico_cyw43_arch_none AND TARGET pico_btstack_ble
        AND TARGET pico_btstack_cyw43)
        target_compile_definitions(${board} PRIVATE
            BT_SUPPORT=1
            # BTstack storage sits right below the savedata sector, the
            # offset is provided by bt_flash_storage_offset() in bt.c
            pico_flash_bank_get_storage_offset_func=bt_flash_storage_offset)
        target_link_libraries(${board} PRIVATE
            pico_cyw43_arch_none pico_btstack_ble pico_btstack_cyw43
            pico_btstack_flash_bank pico_btstack_run_loop_async_context)
        pico_btstack_make_gatt_header(${board} PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/bt_profile.gatt)
    endif()

    pico_add_extra_outputs(${board})

    add_custom_command(TARGET ${board} PRE_BUILD
    COMMAND touch ${CMAKE_CURRENT_SOURCE_DIR}/cli.c)

    add_custom_command(TARGET ${board} POST_BUILD
                       COMMAND cp ${board}.uf2 ${CMAKE_CURRENT_LIST_DIR}/..)
endfunction()

make_firmware(iidx_pico BOARD_IIDX_PICO)

