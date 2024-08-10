CC ?= clang
OBJDUMP ?= objdump
WARNINGS = \
	-std=gnu99 \
	-Wall -Wextra -Wpedantic -Wno-varargs -Wno-gnu-zero-variadic-macro-arguments \
    -Werror=return-type \
    -Werror=incompatible-pointer-types \

all: tiny_c \
	tiny_c_shared \
	loader \
	programs/linux/env \
	programs/linux/string \
	programs/linux/tinyfetch

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
		-o libtinyc.so src/tiny_c/tiny_c.c

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
		src/tiny_c/tinyc_sys.c \
		src/tiny_c/tiny_c.c \
		src/elf_tools.c

programs/linux/env:
	@$(CC) $(CFLAGS) -g \
		-D ARM32 \
		-nostdlib -static \
		$(WARNINGS) \
		-o env src/programs/linux/env/env_main.c \
		src/tiny_c/tinyc_sys.c \
		src/tiny_c/tiny_c.c

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

clean:
	@rm -f tiny_wine loader tiny_c.o libtinyc.a libtinyc.so *.dump \
		env string tinyfetch

install: tiny_c tiny_c_shared
	cp src/tiny_c/tiny_c.h /usr/local/include
	cp libtinyc.a libtinyc.so /usr/local/lib
