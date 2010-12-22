#define H(symbol) asm(".hidden " #symbol "\n\r");

H(__heap_alloc_at)
H(__heap_free)
H(__heap_alloc)
H(malloc)
H(free)
H(malloc_init)
