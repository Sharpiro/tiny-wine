# @todo: add fsanitize

CC ?= clang
OBJDUMP ?= objdump
WARNINGS = \
	-std=c99 \
	-Wall -Wextra -Wpedantic -Wno-varargs -Wno-gnu-zero-variadic-macro-arguments \
	-Wconversion \
    -Werror=return-type \
    -Werror=incompatible-pointer-types \

all: tiny_c \
	tiny_c_shared \
	loader \
	programs/linux/unit_test \
	programs/linux/env \
	programs/linux/string \
	programs/linux/tinyfetch \
	programs/linux/static_pie \
	programs/linux/dynamic

tiny_c: src/tiny_c/tiny_c.c
	@$(CC) $(CFLAGS) \
		-c \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-o tiny_c.o src/tiny_c/tiny_c.c
	@ar rcs libtinyc.a tiny_c.o

tiny_c_shared: src/tiny_c/tiny_c.c
	@$(CC) $(CFLAGS) \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-shared \
		-fPIC \
		-o libtinyc.so \
		src/tiny_c/tinyc_sys.c \
		src/tiny_c/tiny_c.c

loader: src/loader/loader_main.c src/tiny_c/tiny_c.c
	@$(CC) $(CFLAGS) \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-fPIE \
		-Wl,--section-start=.text=7d7d0000 \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-o loader \
		src/loader/loader_main.c \
		src/loader/loader_lib.c \
		src/loader/memory_map.c \
		src/tiny_c/tinyc_sys.c \
		src/tiny_c/tiny_c.c \
		src/loader/elf_tools.c

programs/linux/unit_test:
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		$(WARNINGS) \
		-o unit_test src/programs/linux/unit_test/unit_test_main.c \
		src/tiny_c/tinyc_sys.c \
		src/tiny_c/tiny_c.c \
		src/loader/memory_map.c \
		src/loader/loader_lib.c

programs/linux/env:
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		-nostdlib -static \
		$(WARNINGS) \
		-o env src/programs/linux/env/env_main.c \
		src/tiny_c/tinyc_sys.c \
		src/tiny_c/tiny_c.c
	@objdump -D env > env.dump

programs/linux/string:
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		-nostdlib -static \
		$(WARNINGS) \
		-o string src/programs/linux/string/string_main.c \
		src/tiny_c/tinyc_sys.c \
		src/tiny_c/tiny_c.c
	@$(OBJDUMP) -D string > string.dump

programs/linux/tinyfetch:
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		-nostdlib -static \
		$(WARNINGS) \
		-o tinyfetch src/programs/linux/tinyfetch/tinyfetch_main.c \
		src/tiny_c/tinyc_sys.c \
		src/tiny_c/tiny_c.c
	@$(OBJDUMP) -D tinyfetch > tinyfetch.dump

programs/linux/static_pie:
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		-nostdlib -static-pie \
		$(WARNINGS) \
		-o static_pie src/programs/linux/static_pie/static_pie_main.c \
		src/tiny_c/tinyc_sys.c \
		src/tiny_c/tiny_c.c
	@$(OBJDUMP) -D static_pie > static_pie.dump

programs/linux/dynamic:
	@$(CC) \
		-D ARM32 \
		$(WARNINGS) \
		-S \
		-o dynamic.s \
		src/programs/linux/dynamic/dynamic_main.c
	@$(CC) -g \
		-D ARM32 \
		-nostdlib -no-pie \
		$(WARNINGS) \
		-o dynamic \
		./libtinyc.so \
		src/programs/linux/dynamic/dynamic_main.c
	@objdump -D dynamic > dynamic.dump

clean:
	@rm -f tiny_wine loader tiny_c.o libtinyc.a libtinyc.so *.dump \
		env string tinyfetch dynamic *.s

install: tiny_c tiny_c_shared
	cp src/tiny_c/tiny_c.h /usr/local/include
	cp libtinyc.a libtinyc.so /usr/local/lib
