#define H(symbol) asm(".hidden " #symbol "\n\r")

H(stdout);
H(stdin);
H(stderr);
H(errno);
H(puts);
H(_stdio_openlist);
H(__GI___errno_location);
H(__GI___h_errno_location);
H(malloc);
H(free);
H(_dl_aux_init);
H(setbuf);
