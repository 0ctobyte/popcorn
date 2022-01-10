add_subdirectory(qemu-virt)
add_subdirectory(raspberry-pi-3)
add_subdirectory(raspberry-pi-4)

target_sources(${target}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/platform.c
)
