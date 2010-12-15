#include <stdlib.h>
#include "resolv.h"
#include "../lib.h"

size_t __pagesize = 0x1000;
HIDDEN(__pagesize);

int *__errno_location (void)
{
    return &errno;
}
HIDDEN(__errno_location)
