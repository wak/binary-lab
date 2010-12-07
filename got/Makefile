
all: shared noshared

shared: get_got.S shared.c print_got.c
	gcc -Wall -fPIC -shared -o shared.so get_got.S print_got.c
	gcc -Wall -o shared shared.c shared.so

run_shared: shared
	LD_LIBRARY_PATH=. ./shared

noshared: noshared.c get_got.S
	gcc -Wall -o noshared noshared.c get_got.S

clean:
	rm -f shared.so shared noshared
