add_executable(kb1)
pico_add_extra_outputs(kb1)
target_link_libraries(kb1 PRIVATE pico_stdlib tinyusb_kb)
target_sources(kb1 PRIVATE kb1.c)

add_executable(kb2)
pico_add_extra_outputs(kb2)
target_link_libraries(kb2 PRIVATE pico_stdlib tinyusb_kb)
target_sources(kb2 PRIVATE kb2.c)
