add_executable(kb1)
pico_add_extra_outputs(kb1)
target_link_libraries(kb1 PRIVATE pico_stdlib tinyusb_kb)
target_sources(kb1 PRIVATE kb1.c)
