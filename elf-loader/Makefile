# MEMO
#   -fvisibility=
#      http://gcc.gnu.org/onlinedocs/gcc-4.5.2/gcc/Code-Gen-Options.html#index-fvisibility-2129
CFLAGS := -Wall -g -O0 -std=gnu99 -fPIC -fvisibility=hidden -nostdlib -nostartfiles -Iinclude -MD -Wno-unused-but-set-variable
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

default: $(RTLD)
rebuild: clean $(RTLD)

# MEMO (http://sourceware.org/binutils/docs-2.19/ld/Options.html)
#  -z:
#    combreloc:
#       Combines multiple reloc sections and sorts them to -make
#       dynamic symbol lookup caching possible.
#    relro:
#     Disallows undefined symbols in object files. Undefined symbols
#     in shared libraries are still allowed.
#    defs:
#     Create an ELF PT_GNU_RELRO segment header in the object.
#
#  --hash-style=
#    Set the type of linker's hash table(s). style can be either sysv
#    for classic ELF .hash section, gnu for new style GNU .gnu.hash
#    section or both for both the classic ELF .hash and new style GNU
#    .gnu.hash hash tables. The default is sysv.
#
#  -shared
#    Create a shared library. This is currently only supported on ELF,
#    XCOFF and SunOS platforms. On SunOS, the linker will
#    automatically create a shared library if the -e option is not
#    used and there are undefined symbols in the link.
#
$(RTLD): $(obj) link.lds
	$(call message,LD,$@)
	$(Q)gcc -nostdlib -nostartfiles -shared -T link.lds \
		-Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both \
		-Wl,-z,defs -Wl,-soname=$(RTLD) \
		-o $(RTLD) $(obj)
	@ruby -e 'puts "","=" * 80'
	@readelf -r $(RTLD)
	@echo `readelf -l $(RTLD) | grep interpreter`
	@ruby -e 'puts "=" * 80'

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

# For Debian Squeeze
link.lds:
	$(call message,GEN,$@)
	$(Q)gcc -nostdlib -nostartfiles -shared  \
		-Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both \
		-Wl,-z,defs -Wl,--verbose 2>&1 |  \
		LC_ALL=C \
		sed -e '/^=========/,/^=========/!d;/^=========/d'    \
			-e 's/^}/  _begin = SEGMENT_START("text-segment", 0);\n}/' \
		> link.lds
#   For Debian Lenny
#			-e 's/\. = 0 + SIZEOF_HEADERS;/& _begin = . - SIZEOF_HEADERS;/' \

sample:
	make --no-print-directory -C sample

-include $(DEPENDS)

.PHONY: clean rebuild malloc sample
