#ifndef ATOMIC_H
#define ATOMIC_H

#include <os.h>
#include "interrupt.h"

void * atomic_memcpy(void * dest, void * src, size_t num);

#define ATOMIC_DO(x) do { \
        set_interrupt_off(); \
        x; \
        set_interrupt_on(); \
    } while (0) \

#endif