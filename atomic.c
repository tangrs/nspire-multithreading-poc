#include <os.h>
#include "interrupt.h"

void * atomic_memcpy(void * dest, void * src, size_t num) {
    set_interrupt_off();
    /* use builtin version just in case our interrupts did something dodgy */
    void* ret = __builtin_memcpy(dest, src, num);
    set_interrupt_on();
    return ret;
}