#ifndef CTR_SCRATCHPAD_H
#define CTR_SCRATCHPAD_H

#include <stdint.h>
#include <stddef.h>

#define CTR_SCRATCHPAD_ADDR                 0x1f800000u
#define CTR_SCRATCHPAD_SIZE                 0x400u
#define CTR_SCRATCHPAD_MAP_SIZE             0x1000u

// Backing storage for PS1 scratchpad emulation on modern 64-bit and 32-bit hosts
extern uint8_t g_scratchpadMemory[CTR_SCRATCHPAD_MAP_SIZE];

#define CTR_SCRATCHPAD_BASE                 (g_scratchpadMemory)
#define CTR_SCRATCHPAD_PTR(type, offset)    ((type *)(void *)(CTR_SCRATCHPAD_BASE + (offset)))
#define CTR_SCRATCHPAD_ADDR_PTR(type, addr) ((type *)(void *)(CTR_SCRATCHPAD_BASE + (((uintptr_t)(addr)) & 0xfffu)))

#endif
