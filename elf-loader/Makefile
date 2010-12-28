
CFLAGS := -Wall -g -O0 -std=gnu99 -fPIC -nostdlib -nostartfiles -Iinclude -MD
#CFLAGS += -I/home/wak/src/uClibc-0.9.31/include


rtld.obj   := rtld hidden lib base reloc load lookup trampoline
rtld.obj   := $(patsubst %,rtld/%.o,$(rtld.obj))
malloc.obj := malloc calloc free realloc memalign \
					heap_alloc heap_alloc_at heap_free resolv
malloc.obj := $(patsubst %,malloc/%.o,$(malloc.obj))

obj := $(rtld.obj) $(malloc.obj)
DEPENDS := $(patsubst %.o,%.d,$(obj))

RTLD := rtld.so

ifeq ("$V", "")
  Q := @
endif
define message
	@if [ "$Q" = "@" ] ; then \
		echo "  "$1 $2 ;\
	fi
endef

rtld: $(obj) $(RTLD)
	@ruby -e 'puts "","=" * 80'
	readelf -r $(RTLD)
	@ruby -e 'puts "=" * 80'

rebuild: clean rtld.so

$(RTLD): $(obj) link.lds
	$(call message,LD,$@)
	$(Q)gcc -nostdlib -nostartfiles -shared -T link.lds \
		-Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both \
		-Wl,-z,defs -Wl,-soname=$(RTLD) \
		-o $(RTLD) $(obj)

%.o: %.S
	$(call message,CC,$@)
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.c
	$(call message,CC,$@)
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<
#	cpp $(CFLAGS) $< $(<:.c=.cpp)

clean:
	$(call message,CLEAN)
	$(Q)rm -f $(RTLD) link.lds $(obj) $(DEPENDS)
	$(call message,CLEAN,sample)
	$(Q)make Q=$Q --no-print-directory -C sample clean

link.lds:
	$(call message,GEN,$@)
	$(Q)gcc -nostdlib -nostartfiles -shared  \
		-Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both \
		-Wl,-z,defs -Wl,--verbose 2>&1 |  \
		LC_ALL=C \
		sed -e '/^=========/,/^=========/!d;/^=========/d'    \
			-e 's/\. = 0 + SIZEOF_HEADERS;/& _begin = . - SIZEOF_HEADERS;/' \
		> link.lds

sample: rtld
	make --no-print-directory -C sample

-include $(DEPENDS)

.PHONY: clean rebuild malloc sample
