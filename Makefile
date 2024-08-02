CC ?= cc
WARNINGS = \
	-std=gnu17 \
	-Wall -Wextra -Wpedantic -Wno-varargs -Wno-gnu-zero-variadic-macro-arguments

all: tiny_c tiny_c_shared loader

all_arm: tiny_c_arm \
	tiny_c_arm_shared \
	programs/linux/env \
	programs/linux/string \
	loader_arm

tiny_wine: main.c prctl.c *.h
	@$(CC) \
		$(WARNINGS) \
		-masm=intel \
		-Wl,--section-start=.interp=0x900000 \
		-fno-stack-protector \
		-g \
		-o tiny_wine prctl.c main.c

tiny_c_arm: src/tiny_c/tiny_c.c
	@$(CC) \
		-c \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-o tiny_c.o src/tiny_c/tiny_c.c
	@ar rcs libtinyc.a tiny_c.o

tiny_c_arm_shared: src/tiny_c/tiny_c.c
	@$(CC) \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-shared \
		-fPIC \
		-o libtinyc.so src/tiny_c/tiny_c.c

tiny_c: src/tiny_c/tiny_c.c
	@$(CC) \
		-c \
		-O0 \
		-mno-sse \
		-nostdlib \
		$(WARNINGS) \
		-masm=intel \
		-fno-stack-protector \
		-g \
		-DAMD64 \
		-o tiny_c.o src/tiny_c/tiny_c.c
	@ar rcs libtinyc.a tiny_c.o

loader: loader.c src/tiny_c/tiny_c.c
	@$(CC) \
		-O0 \
		-mno-sse \
		-nostdlib \
		$(WARNINGS) \
		-masm=intel \
		-fPIE \
		-Wl,--section-start=.text=0x00007d7d00000000 \
		-fno-stack-protector \
		-g \
		-o loader loader.c src/tiny_c/tiny_c.c

loader_arm: src/loader/loader_main.c src/tiny_c/tiny_c.c
	@$(CC) \
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
		src/tiny_c/tiny_c.c \
		src/elf_tools.c

programs/linux/env:
	@$(CC) -g \
		-D ARM32 \
		-nostdlib -static \
		$(WARNINGS) \
		-o env src/programs/linux/env/env_main.c \
		src/tiny_c/tiny_c.c

programs/linux/string:
	@$(CC) -g \
		-D ARM32 \
		-nostdlib -static \
		$(WARNINGS) \
		-o string src/programs/linux/string/string_main.c \
		src/tiny_c/tiny_c.c
	@objdump -D string > string.dump

clean:
	@rm -f tiny_wine loader tiny_c.o libtinyc.a libtinyc.so env string *.dump

install: tiny_c_arm tiny_c_arm_shared
	cp src/tiny_c/tiny_c.h /usr/local/include
	cp libtinyc.a libtinyc.so /usr/local/lib
