target_sources(${target}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/proc_init.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proc_scheduler.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proc_task.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proc_thread.c
)
