add_library(tinyusb_kb STATIC EXCLUDE_FROM_ALL)

include_directories( ${CMAKE_CURRENT_LIST_DIR} )

target_link_libraries(tinyusb_kb PUBLIC
    pico_stdlib
    pico_unique_id
    tinyusb_device
)

target_sources(tinyusb_kb PRIVATE
    main.c
    usb_descriptors.c
    get_serial.c
)
