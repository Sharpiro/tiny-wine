# @todo: add fsanitize

ifeq ($(CC),cc)
  CC := clang
endif

OBJDUMP ?= objdump
WARNINGS = \
	-std=gnu99 \
	-Wall -Wextra -Wpedantic -Wno-varargs \
	-Wno-gnu-zero-variadic-macro-arguments \
	-Wno-gnu-statement-expression-from-macro-expansion \
	-Wconversion \
    -Werror=return-type \
    -Werror=incompatible-pointer-types \

all: \
	libtinyc.a \
	libtinyc.so \
	loader \
	programs/linux/unit_test \
	programs/linux/env \
	programs/linux/string \
	programs/linux/tinyfetch \
	programs/linux/static_pie \
	programs/linux/dynamic

.PHONY: echo_hi
echo_hi:
	@echo hi

tinyc_sys.o: src/tiny_c/tiny_c.c
	@$(CC) $(CFLAGS) \
		-g \
		-c \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-fPIC \
		-fno-stack-protector \
		-DARM32 \
		-o tinyc_sys.o src/tiny_c/tinyc_sys.c

tiny_c.o: src/tiny_c/tiny_c.c
	@$(CC) $(CFLAGS) \
		-g \
		-c \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-fPIC \
		-fno-stack-protector \
		-DARM32 \
		-o tiny_c.o src/tiny_c/tiny_c.c

libtinyc.a: tinyc_sys.o tiny_c.o
	@ar rcs libtinyc.a tinyc_sys.o tiny_c.o

libtinyc.so: tinyc_sys.o tiny_c.o
	@$(CC) $(CFLAGS) \
		-O0 \
		$(WARNINGS) \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-nostdlib -static \
		-shared \
		-o libtinyc.so \
		tinyc_sys.o \
		tiny_c.o
	@$(OBJDUMP) -D libtinyc.so > libtinyc.so.dump

loader: src/loader/loader_main.c src/tiny_c/tiny_c.c
	@$(CC) $(CFLAGS) \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-Wl,--section-start=.text=7d7d0000 \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-o loader \
		src/loader/loader_main.c \
		src/loader/loader_lib.c \
		src/loader/memory_map.c \
		src/loader/elf_tools.c \
		libtinyc.a

programs/linux/unit_test: libtinyc.a
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		-nostdlib -static \
		$(WARNINGS) \
		-o unit_test \
		src/programs/linux/unit_test/unit_test_main.c \
		src/loader/memory_map.c \
		src/loader/loader_lib.c \
		libtinyc.a

programs/linux/env: libtinyc.a
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		-nostdlib -static \
		$(WARNINGS) \
		-o env \
		src/programs/linux/env/env_main.c \
		libtinyc.a
	@$(OBJDUMP) -D env > env.dump

programs/linux/string:
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		-nostdlib -static \
		$(WARNINGS) \
		-o string \
		src/programs/linux/string/string_main.c \
		libtinyc.a
	@$(OBJDUMP) -D string > string.dump

programs/linux/tinyfetch:
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		-nostdlib -static \
		$(WARNINGS) \
		-o tinyfetch \
		src/programs/linux/tinyfetch/tinyfetch_main.c \
		libtinyc.a
	@$(OBJDUMP) -D tinyfetch > tinyfetch.dump

programs/linux/static_pie: libtinyc.a
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		-nostdlib \
		-static-pie \
		$(WARNINGS) \
		-o static_pie \
		src/programs/linux/static_pie/static_pie_main.c \
		libtinyc.a
	@$(OBJDUMP) -D static_pie > static_pie.dump

programs/linux/dynamic:
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		$(WARNINGS) \
		-S \
		-o dynamic.s \
		src/programs/linux/dynamic/dynamic_main.c
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		-nostdlib \
		-no-pie \
		$(WARNINGS) \
		-o dynamic \
		./libtinyc.so \
		src/programs/linux/dynamic/dynamic_main.c
	@$(OBJDUMP) -D dynamic > dynamic.dump

clean:
	@rm -f \
	*.dump *.o *.s *.a *.so \
	loader env string tinyfetch dynamic unit_test static_pie

install: libtinyc.a libtinyc.so
	cp src/tiny_c/tiny_c.h /usr/local/include
	cp libtinyc.a libtinyc.so /usr/local/lib
