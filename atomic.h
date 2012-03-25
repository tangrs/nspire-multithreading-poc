#include <stdint.h>

void atomic_w32(uint32_t*,uint32_t);
void atomic_w16(uint16_t*,uint16_t);
void atomic_w8(uint8_t*,uint8_t);

uint32_t atomic_r32(uint32_t*);
uint16_t atomic_r16(uint16_t*);
uint8_t atomic_r8(uint8_t*);
