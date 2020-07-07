add_subdirectory(arch)
add_subdirectory(vm)
add_subdirectory(vfs)

target_sources(${target}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/bitmap
        ${CMAKE_CURRENT_SOURCE_DIR}/devicetree
        ${CMAKE_CURRENT_SOURCE_DIR}/fdt
        ${CMAKE_CURRENT_SOURCE_DIR}/kmain
        ${CMAKE_CURRENT_SOURCE_DIR}/kmem
        ${CMAKE_CURRENT_SOURCE_DIR}/kstdio
        ${CMAKE_CURRENT_SOURCE_DIR}/list
        ${CMAKE_CURRENT_SOURCE_DIR}/panic
        ${CMAKE_CURRENT_SOURCE_DIR}/rbtree
        ${CMAKE_CURRENT_SOURCE_DIR}/slab
        ${CMAKE_CURRENT_SOURCE_DIR}/spinlock
)
