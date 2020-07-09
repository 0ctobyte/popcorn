#include <kernel/panic.h>
#include <kernel/kstdio.h>
#include <kernel/arch/arch_exceptions.h>

#define STRINGIFY(s) #s

#define EXC_CLASS_STR(exc_class) STRINGIFY(exc_class)
#define GET_EXC_CLASS(esr) (((esr) >> 26) & 0x3f)
#define GET_EXC_ISS(esr) ((esr) & 0x1ffffff)

typedef enum {
    EXC_CLASS_UNKNOWN_REASON               = 0x0,
    EXC_CLASS_TRAP_WFI_WFE                 = 0x1,
    EXC_CLASS_TRAP_CP15_MRC_MCR_AARCH32    = 0x2,
    EXC_CLASS_TRAP_CP15_MRRC_MCRR_AARCH32  = 0x4,
    EXC_CLASS_TRAP_CP14_MRC_MCR_AARCH32    = 0x5,
    EXC_CLASS_TRAP_LDC_STC_AARCH32         = 0x6,
    EXC_CLASS_TRAP_SVE_SIMD_FP             = 0x7,
    EXC_CLASS_TRAP_CP14_MRRC_AARCH32       = 0xc,
    EXC_CLASS_BRANCH_TARGET                = 0xd,
    EXC_CLASS_ILLEGAL_EXECUTION_STATE      = 0xe,
    EXC_CLASS_SVC_AARCH32                  = 0x11,
    EXC_CLASS_SVC_AARCH64                  = 0x15,
    EXC_CLASS_TRAP_MRS_MSR_SYS             = 0x18,
    EXC_CLASS_TRAP_SVE                     = 0x19,
    EXC_CLASS_PAC_FAILURE                  = 0x1c,
    EXC_CLASS_INSTRUCTION_ABORT_LOWER_EL   = 0x20,
    EXC_CLASS_INSTRUCTION_ABORT_CURRENT_EL = 0x21,
    EXC_CLASS_PC_ALIGNMENT_FAULT           = 0x22,
    EXC_CLASS_DATA_ABORT_LOWER_EL          = 0x24,
    EXC_CLASS_DATA_ABORT_CURRENT_EL        = 0x25,
    EXC_CLASS_TRAP_FP_EXCEPTION_AARCH32    = 0x28,
    EXC_CLASS_TRAP_FP_EXCEPTION_AARCH64    = 0x2c,
    EXC_CLASS_SERROR                       = 0x2f,
    EXC_CLASS_BREAKPOINT_LOWER_EL          = 0x30,
    EXC_CLASS_BREAKPOINT_CURRENT_EL        = 0x31,
    EXC_CLASS_SOFTWARE_STEP_LOWER_EL       = 0x32,
    EXC_CLASS_SOFTWARE_STEP_CURRENT_EL     = 0x33,
    EXC_CLASS_WATCHPOINT_LOWER_EL          = 0x33,
    EXC_CLASS_WATCHPOINT_CURRENT_EL        = 0x34,
    EXC_CLASS_BKPT_INSTRUCTION_AARCH32     = 0x38,
    EXC_CLASS_BKPT_INSTRUCTION_AARCH64     = 0x3c,
} arch_exception_class_type_t;

void _arch_exception_iss_decode_error(arch_exception_state_t *exc_state) {
    unsigned int exc_iss = GET_EXC_ISS(exc_state->esr) & 0x3f;
    switch (exc_iss) {
        case 0x00: kprintf("Address size fault, level 0\n"); break;
        case 0x01: kprintf("Address size fault, level 1\n"); break;
        case 0x02: kprintf("Address size fault, level 2\n"); break;
        case 0x03: kprintf("Address size fault, level 3\n"); break;
        case 0x04: kprintf("Translation fault, level 0\n"); break;
        case 0x05: kprintf("Translation fault, level 1\n"); break;
        case 0x06: kprintf("Translation fault, level 2\n"); break;
        case 0x07: kprintf("Translation fault, level 3\n"); break;
        case 0x09: kprintf("Access flag fault, level 1\n"); break;
        case 0x0a: kprintf("Access flag fault, level 2\n"); break;
        case 0x0b: kprintf("Access flag fault, level 3\n"); break;
        case 0x0d: kprintf("Permission fault, level 1\n"); break;
        case 0x0e: kprintf("Permission fault, level 2\n"); break;
        case 0x0f: kprintf("Permission fault, level 3\n"); break;
        case 0x10: kprintf("Synchronous external abort\n"); break;
        case 0x14: kprintf("Synchronous external abort, level 0\n"); break;
        case 0x15: kprintf("Synchronous external abort, level 1\n"); break;
        case 0x16: kprintf("Synchronous external abort, level 2\n"); break;
        case 0x17: kprintf("Synchronous external abort, level 3\n"); break;
        case 0x18: kprintf("Synchronous parity or ECC error\n"); break;
        case 0x1c: kprintf("Synchronous parity or ECC error, level 0\n"); break;
        case 0x1d: kprintf("Synchronous parity or ECC error, level 1\n"); break;
        case 0x1e: kprintf("Synchronous parity or ECC error, level 2\n"); break;
        case 0x1f: kprintf("Synchronous parity or ECC error, level 3\n"); break;
        case 0x21: kprintf("Alignment fault\n"); break;
        case 0x30: kprintf("TLB conflict abort\n"); break;
        default: kprintf("\n"); break;
    }
}

bool _arch_exception_class_decode_error(arch_exception_state_t *exc_state) {
    bool fail = true;
    unsigned int exc_class = GET_EXC_CLASS(exc_state->esr);
    switch (exc_class) {
        case EXC_CLASS_UNKNOWN_REASON:
        {
            kprintf(EXC_CLASS_STR(EXC_CLASS_UNKNOWN_REASON\n));
            break;
        }
        case EXC_CLASS_ILLEGAL_EXECUTION_STATE:
        {
            kprintf(EXC_CLASS_STR(EXC_CLASS_ILLEGAL_EXECUTION_STATE\n));
            break;
        }
        case EXC_CLASS_INSTRUCTION_ABORT_LOWER_EL:
        {
            kprintf(EXC_CLASS_STR(EXC_CLASS_INSTRUCTION_ABORT_LOWER_EL\n));
            _arch_exception_iss_decode_error(exc_state);
            break;
        }
        case EXC_CLASS_INSTRUCTION_ABORT_CURRENT_EL:
        {
            kprintf(EXC_CLASS_STR(EXC_CLASS_INSTRUCTION_ABORT_CURRENT_EL\n));
            _arch_exception_iss_decode_error(exc_state);
            break;
        }
        case EXC_CLASS_PC_ALIGNMENT_FAULT:
        {
            kprintf(EXC_CLASS_STR(EXC_CLASS_PC_ALIGNMENT_FAULT\n));
            break;
        }
        case EXC_CLASS_DATA_ABORT_LOWER_EL:
        {
            kprintf(EXC_CLASS_STR(EXC_CLASS_DATA_ABORT_LOWER_EL\n));
            _arch_exception_iss_decode_error(exc_state);
            break;
        }
        case EXC_CLASS_DATA_ABORT_CURRENT_EL:
        {
            kprintf(EXC_CLASS_STR(EXC_CLASS_DATA_ABORT_CURRENT_EL\n));
            _arch_exception_iss_decode_error(exc_state);
            break;
        }
        case EXC_CLASS_SERROR:
        {
            kprintf(EXC_CLASS_STR(EXC_CLASS_SERROR\n));
            break;
        }
        default: fail = false; break;
    }

    return fail;
}

void arch_exceptions_dump_state(arch_exception_state_t *exc_state) {
    // Dump the general purpose registers
    for (unsigned int i0 = 0, i1 = 1, i2 = 2, i3 = 3; i0 < 32; i0 += 4, i1 += 4, i2 += 4, i3 += 4) {
        if (i0 == 28) {
            kprintf("x%u:\t0x%016lx\tx%u:\t0x%016lx\tlr:\t0x%016lx\tsp:\t0x%016lx\n", i0, exc_state->x[i0], i1, exc_state->x[i1], exc_state->lr, exc_state->sp);
        } else {
            kprintf("x%u:\t0x%016lx\tx%u:\t0x%016lx\tx%u:\t0x%016lx\tx%u:\t0x%016lx\n", i0, exc_state->x[i0], i1, exc_state->x[i1], i2, exc_state->x[i2], i3, exc_state->x[i3]);
        }
    }

    // Dump the PC, SPSR, FAR and ESR
    kprintf("pc:\t0x%016lx\tspsr:\t0x%016lx\tfar:\t0x%016lx\tesr:\t0x%016lx\n", exc_state->elr, exc_state->spsr, exc_state->far, exc_state->esr);

    // Decode the Mode
    kprintf("mode:\tEL%u%c\n", (exc_state->spsr >> 2) & 0x3, (exc_state->spsr & 0x1) ? 'h' : 't');
}

void arch_exception_handler(arch_exception_type_t exc_type, arch_exception_state_t *exc_state) {
    switch (exc_type) {
        case EXCEPTION_SYNC_SP_EL0:
        case EXCEPTION_SYNC_SP_ELX:
        case EXCEPTION_SYNC_LL_AARCH64:
        {
            kprintf("Synchronous Exception!\n");
            if (!_arch_exception_class_decode_error(exc_state)) return;
            break;
        }
        case EXCEPTION_SERR_SP_EL0:
        case EXCEPTION_SERR_SP_ELX:
        case EXCEPTION_SERR_LL_AARCH64:
        {
            kprintf("SError Exception!\n");
            if (!_arch_exception_class_decode_error(exc_state)) return;
            break;
        }
        case EXCEPTION_IRQ_SP_EL0:
        case EXCEPTION_IRQ_SP_ELX:
        case EXCEPTION_IRQ_LL_AARCH64:
        {
            return;
        }
        case EXCEPTION_FIQ_SP_EL0:
        case EXCEPTION_FIQ_SP_ELX:
        case EXCEPTION_FIQ_LL_AARCH64:
        {
            return;
        }
        default:
        {
            panic("Running in AARCH32 mode is not supported!");
            break;
        }
    }

    arch_exceptions_dump_state(exc_state);

    panic("HALTING\n");
}
