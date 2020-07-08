#include <kernel/proc/proc_task.h>
#include <kernel/proc/proc_thread.h>
#include <kernel/proc/proc_init.h>

void proc_init(void) {
    proc_task_init();
    proc_thread_init();
}
