CC=gcc
WARNINGS = \
	-Wall -Wextra -Wpedantic -Wno-varargs \
	-Wno-builtin-declaration-mismatch

all: tiny_c tiny_c_shared loader

all_arm: tiny_c_arm \
	tiny_c_arm_shared \
	programs/linux/env \
	programs/linux/string \
	loader_arm

tiny_wine: main.c prctl.c *.h
	@$(CC) \
		$(WARNINGS) \
		-std=gnu2x \
		-masm=intel \
		-Wl,--section-start=.interp=0x900000 \
		-fno-stack-protector \
		-g \
		-o tiny_wine prctl.c main.c

# -std=gnu2x \
# -nostartfiles \
# -nodefaultlibs \
# -Wl,--section-start=.rodata=0x6d7d00000000 \

tiny_c_arm: src/tiny_c/tiny_c.c
	@$(CC) \
		-c \
		-O0 \
		-nostdlib \
		-nostartfiles -nodefaultlibs \
		$(WARNINGS) \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-o tiny_c.o src/tiny_c/tiny_c.c
	@ar rcs libtinyc.a tiny_c.o

tiny_c_arm_shared: src/tiny_c/tiny_c.c
	@$(CC) \
		-O0 \
		-nostdlib \
		-nostartfiles -nodefaultlibs \
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
		-nostdlib \
		$(WARNINGS) \
		-fPIE \
		-Wl,--section-start=.text=7d7d0000 \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-o loader src/loader/loader_main.c src/tiny_c/tiny_c.c

programs/linux/string:
	@$(CC) -g \
		-D ARM32 \
		-nostartfiles -nodefaultlibs \
		$(WARNINGS) \
		-o string src/programs/linux/string/string_main.c \
		src/tiny_c/tiny_c.c

programs/linux/env:
	@$(CC) -g \
		-D ARM32 \
		-nostartfiles -nodefaultlibs \
		$(WARNINGS) \
		-o env src/programs/linux/env/env_main.c \
		src/tiny_c/tiny_c.c

clean:
	@rm -f tiny_wine loader tiny_c.o libtinyc.a libtinyc.so env string

install: tiny_c_arm tiny_c_arm_shared
	cp src/tiny_c/tiny_c.h /usr/include
	cp libtinyc.a libtinyc.so /usr/lib
