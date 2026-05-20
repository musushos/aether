#pragma once

#include <cstdint>

/* Forward declaration — full types come from aether.c unity build */
#ifndef AETHER_ARG_TYPEDEF
#define AETHER_ARG_TYPEDEF
typedef struct { int i; unsigned int ui; float f; const void *v; } Arg;
#endif

void switcherstep(const Arg *arg);
void switchercommit(void);
void switchercancel(void);
int  switcherisactive(void);
void switcherupdatemods(uint32_t mods);
