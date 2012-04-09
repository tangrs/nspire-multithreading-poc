#include <os.h>
#include "interrupt.h"


extern unsigned* (*int_handler_func)(unsigned*,unsigned*);
static unsigned orig_addr, orig_int_mask;
static void* stack_base;

/* should be reentrant... hopefully */
static void __attribute__((naked,noreturn)) int_handler() {
    /* interrupts disabled as of here */
    asm(
        "b 0f \n"
        "int_restore_cpsr: .word 0 \n"
        "int_handler_stack: .word 0 \n"
        "int_handler_func: .word 0 \n"
        "0: \n"
        /* begin saving state */
        "push {r12} \n"
        "ldr r12, int_handler_stack \n"
        "sub r12, r12, #64 \n"
        /* save register r15 (stored in lr in interrupt mode) */
        "sub lr, lr, #4 \n"
        "str lr, [r12, #60] \n"
        /* save registers r0-r11 */
        "stm r12, {r0-r11} \n"
        /* save r12 */
        "pop {r0} \n"
        "str r0, [r12, #48] \n"
        /* save cpsr */
        "mrs r0, spsr \n"
        "str r0, int_restore_cpsr \n"
        /* enter a interrupt-disabled state on return to svc mode */
        "mov r0, #0xD3 \n"
        "msr spsr_cxsf, r0 \n"
        /* go back to svc mode to save remaining registers */
        "subs pc, pc, #4 \n"
        /* save r13 and r14 */
        "add r12, r12, #52 \n"
        "stm r12, {r13, r14} \n"
        /* all registers should be stored in sequential order on the stack now */
        /* set up interrupt environment */
        "sub sp, r12, #52 \n"
        /* call handler function */
        "mov r0, sp \n" /* (unsigned *saved_regs) */
        "adr r1, int_restore_cpsr \n" /* (unsigned *saved_cpsr) */
        "mov lr, pc \n"
        "ldr pc, int_handler_func \n"
        /* handler run, now restore state */
        "ldr r1, int_restore_cpsr \n"
        "msr cpsr_cxsf, r1 \n" /* reenable interrupts */
        /* restore context */
        "ldm r0,{r0-r15} \n"
        /* if interrupted here, it should restart from the beginning and cause no big dramas */
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
    extern void *int_handler_stack;
    stack_base = malloc(0x80000);
    if (!stack_base) return;

    int_handler_stack = (char*)stack_base + 0x80000;

    /* set interrupt mask */
    set_interrupt_mask();

    /* set handler function to handle the interrupt */
    int_handler_func = func;

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