target_sources(${target}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vm_km.c
        ${CMAKE_CURRENT_SOURCE_DIR}/vm_map.c
        ${CMAKE_CURRENT_SOURCE_DIR}/vm_object.c
        ${CMAKE_CURRENT_SOURCE_DIR}/vm_page.c
        ${CMAKE_CURRENT_SOURCE_DIR}/vm_init.c
)
