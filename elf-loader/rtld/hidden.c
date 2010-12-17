#define H(symbol) asm(".hidden " #symbol "\n\r");

//H(errno)
H(strlen)
H(dputs)
H(__heap_alloc_at)
H(__heap_free)
H(__heap_alloc)
H(malloc)
H(free)
H(malloc_init)
H(print_mark)
H(print_mark_end)
