add_executable(kb1)
pico_add_extra_outputs(kb1)
target_link_libraries(kb1 PRIVATE pico_stdlib tinyusb_kb)
target_sources(kb1 PRIVATE kb1.c)

add_executable(kb2)
pico_add_extra_outputs(kb2)
target_link_libraries(kb2 PRIVATE pico_stdlib tinyusb_kb)
target_sources(kb2 PRIVATE kb2.c)

add_executable(kb3)
pico_add_extra_outputs(kb3)
target_link_libraries(kb3 PRIVATE pico_stdlib tinyusb_kb)
target_sources(kb3 PRIVATE kb3.c)

add_executable(kb4)
pico_add_extra_outputs(kb4)
target_link_libraries(kb4 PRIVATE pico_stdlib tinyusb_kb)
target_sources(kb4 PRIVATE kb4.c)
