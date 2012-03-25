#include <stdint.h>

/* these are pretty disgusting. better put them into a asm file */

void __attribute__((naked)) atomic_w32(uint32_t * addr, uint32_t val) {
    asm(
        "mrs r2, cpsr \n"
        "bic r3, r2, #0x80 \n"
        "msr cpsr_c, r3 \n"
        "str r1, [r2] \n"
        "msr cpsr_c, r2 \n"
        "mov pc, lr \n"
    );
}

void __attribute__((naked)) atomic_w16(uint16_t * addr, uint16_t val) {
    asm(
        "mrs r2, cpsr \n"
        "bic r3, r2, #0x80 \n"
        "msr cpsr_c, r3 \n"
        "strh r1, [r2] \n"
        "msr cpsr_c, r2 \n"
        "mov pc, lr \n"
    );
}

void __attribute__((naked)) atomic_w8(uint8_t * addr, uint8_t val) {
    asm(
        "mrs r2, cpsr \n"
        "bic r3, r2, #0x80 \n"
        "msr cpsr_c, r3 \n"
        "strb r1, [r2] \n"
        "msr cpsr_c, r2 \n"
        "mov pc, lr \n"
    );
}

uint32_t __attribute__((naked)) atomic_r32(uint32_t * addr) {
    asm(
        "mov r1, r0 \n"
        "mrs r2, cpsr \n"
        "bic r3, r2, #0x80 \n"
        "msr cpsr_c, r3 \n"
        "ldr r0, [r1] \n"
        "msr cpsr_c, r2 \n"
        "mov pc, lr \n"
    );
}

uint16_t __attribute__((naked)) atomic_r16(uint32_t * addr) {
    asm(
        "mov r1, r0 \n"
        "mrs r2, cpsr \n"
        "bic r3, r2, #0x80 \n"
        "msr cpsr_c, r3 \n"
        "ldrh r0, [r1] \n"
        "msr cpsr_c, r2 \n"
        "mov pc, lr \n"
    );
}

uint8_t __attribute__((naked)) atomic_r8(uint32_t * addr) {
    asm(
        "mov r1, r0 \n"
        "mrs r2, cpsr \n"
        "bic r3, r2, #0x80 \n"
        "msr cpsr_c, r3 \n"
        "ldrb r0, [r1] \n"
        "msr cpsr_c, r2 \n"
        "mov pc, lr \n"
    );
}