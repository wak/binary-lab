#define H(symbol) asm(".hidden " #symbol "\n\r");

//H(errno)
H(strlen)
H(dputs)
H(__heap_alloc_at)
H(__heap_free)
H(__heap_alloc)
H(malloc)
H(__malloc_heap)
H(free)
