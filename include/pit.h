#ifndef PIT_H
#define PIT_H

#include "types.h"

#define PIT_FREQ 1193180
#define PIT_CMD  0x43
#define PIT_CH0  0x40

void pit_init(uint32_t frequency);
uint32_t pit_get_ticks(void);
void pit_sleep(uint32_t ms);

#endif
