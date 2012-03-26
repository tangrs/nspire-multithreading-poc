#ifndef INTERRUPT_H
#define INTERRUPT_H

void set_interrupt_on();
void set_interrupt_off();

void init_interrupts(unsigned* (*func)(unsigned*,unsigned*));
void end_interrupts();

#endif