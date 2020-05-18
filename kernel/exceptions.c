#include <kernel/exceptions.h>
#include <kernel/panic.h>
#include <kernel/kstdio.h>

void exception_handler(exception_type_t exc_type, exception_state_t *exc_state) {
    kprintf("\nSYNCHRONOUS ABORT FROM CURRENT EL\n");

    // Dump the general purpose registers
    for (unsigned int i0 = 0, i1 = 1, i2 = 2, i3 = 3; i0 < 32; i0 += 4, i1 += 4, i2 += 4, i3 += 4) {
        if (i0 == 28) {
            kprintf("x%u:\t0x%016lx\tx%u:\t0x%016lx\tlr:\t0x%016lx\tsp:\t0x%016lx\n", i0, exc_state->x[i0], i1, exc_state->x[i1], exc_state->x[i2], exc_state->sp);
        } else {
            kprintf("x%u:\t0x%016lx\tx%u:\t0x%016lx\tx%u:\t0x%016lx\tx%u:\t0x%016lx\n", i0, exc_state->x[i0], i1, exc_state->x[i1], i2, exc_state->x[i2], i3, exc_state->x[i3]);
        }
    }

    // Dump the PC, SPSR, FAR and ESR
    kprintf("pc:\t0x%016lx\tspsr:\t0x%016lx\tfar:\t0x%016lx\tesr:\t0x%016lx\n", exc_state->pc, exc_state->spsr, exc_state->far, exc_state->esr);

    panic("HALTING\n");
}
