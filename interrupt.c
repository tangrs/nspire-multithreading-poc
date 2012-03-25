#include <os.h>
#include "interrupt.h"


static unsigned* (*handler_func)(unsigned*,unsigned*);
static unsigned orig_addr, orig_int_mask;
static void* stack_base;

/* should be reentrant... hopefully */
static void __attribute__((naked)) int_handler() {
    /* interrupts disabled as of here */
    extern unsigned * saved_reg_ptr;
    extern unsigned saved_cpsr;
        asm(
            "b 0f \n"
            "saved_pc: .word 0 \n"
            "saved_reg_ptr: .word 0 \n"
            "saved_cpsr: .word 0 \n"
            "handler_stack: .word 0 \n"
            "0: \n"
            "push {r0} \n"
            "sub r0, lr, #4 \n"
            "str r0, saved_pc \n" /* save return address of interrupt handler */
            "mrs r0, spsr \n"
            "str r0, saved_cpsr \n"
            "orr r0, r0, #0x80 \n" /* disable interrupts on return to svc mode */
            "msr spsr_c, r0 \n"
            "pop {r0} \n"
            "subs pc, pc, #4 \n"
        /* back to original context */
        /* leech off the stack of the context */
            "sub sp, sp, #64 \n"
            "str r0, [sp] \n" /* save r0 first to get a temporary register */
            "mov r0, lr \n"
            "str r0, [sp, #56] \n" /* save lr */
            "ldr r0, saved_pc \n"
            "str r0, [sp, #60] \n" /* save pc */
            "add r0, sp, #64 \n"
            "str r0, [sp, #52] \n" /* save sp */
            "add sp, sp, #4 \n"
            "stm sp, {r1-r12} \n" /* save the rest */
            "sub sp, sp, #4 \n"
            "mov r0, sp \n"
            /*  stack should have registers all in order ready
                to restore state by a simple ldm call */
            "str r0, saved_reg_ptr \n"
            "ldr r0, handler_stack \n"
            "mov sp, r0 \n"
        );
        saved_reg_ptr = handler_func(saved_reg_ptr, &saved_cpsr);
        asm(
            "ldr r1, saved_reg_ptr \n"
            "ldr r0, saved_cpsr \n"
            "msr cpsr_cxsf, r0 \n" /* reenable interrupts */
    /* interrupts enabled as of here */
        /* restore context */
        "ldm r1,{r0-r15} \n"
    );
    __builtin_unreachable();
}

static void set_interrupt_mask() {
    volatile unsigned *intmask = IO(0xDC000008, 0xDC000010);
    orig_int_mask = intmask[0];
    intmask[0] = 0xFFFFFFFF;
    intmask[1] = ~(1 << 19); // Disable all IRQs except timer
}
static void restore_interrupt_mask() {
    volatile unsigned *intmask = IO(0xDC000008, 0xDC000010);
    intmask[1] = 0xFFFFFFFF; // Disable all IRQs
	intmask[0] = orig_int_mask; // renable IRQs
}

void set_interrupt_on() {
    asm(
        "mrs r0, cpsr \n"
        "bic r0, r0, #0x80 \n"
        "msr cpsr_c, r0 \n"
    );
}
void set_interrupt_off() {
    asm(
        "mrs r0, cpsr \n"
        "orr r0, r0, #0x80 \n"
        "msr cpsr_c, r0 \n"
    );
}

void init_interrupts(unsigned* (*func)(unsigned*,unsigned*)) {
    /* setup handler stack */
    extern void *handler_stack;
    stack_base = malloc(0x80000);
    if (!stack_base) return;

    handler_stack = (char*)stack_base + 0x80000;

    /* set interrupt mask */
    set_interrupt_mask();

    /* set handler function to handle the interrupt */
    handler_func = func;

    /* patch vector */
    volatile unsigned *vector_jump = (volatile unsigned *)0x38;
    orig_addr = *vector_jump;
    *vector_jump = (unsigned)&int_handler;

    /* enable interrupts in cpsr */
    set_interrupt_on();
}

void end_interrupts() {
    /* disable interrupts in cpsr */
    set_interrupt_off();

    /* set interrupt mask */
    restore_interrupt_mask();

    free(stack_base);

    /* patch vector */
    volatile unsigned *vector_jump = (volatile unsigned *)0x38;
    *vector_jump = orig_addr;

}