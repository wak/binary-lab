#include <stdlib.h>
#include "resolv.h"
#include <lib.h>
#include <loader.h>

DEFINE_GLO_VAR(size_t, __pagesize) = 0x4000;

int * __attribute__ ((weak, __const__)) __errno_location (void)
{
    return &errno;
}
HIDDEN(__errno_location)

