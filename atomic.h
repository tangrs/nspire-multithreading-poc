#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>
#include "interrupt.h"

void atomic_w32(uint32_t*,uint32_t);
void atomic_w16(uint16_t*,uint16_t);
void atomic_w8(uint8_t*,uint8_t);

uint32_t atomic_r32(uint32_t*);
uint16_t atomic_r16(uint16_t*);
uint8_t atomic_r8(uint8_t*);

#define ATOMIC_FUNC_CALL(x) do { \
        set_interrupt_off(); \
        x; \
        set_interrupt_on(); \
    } while (0) \

#endif