CC=gcc

all: tiny_c_arm tiny_c_arm_shared loader_arm

tiny_wine: main.c prctl.c *.h
	@$(CC) \
		-Wall -Wextra -Wpedantic \
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

tiny_c_arm: tiny_c.c
	@$(CC) \
		-c \
		-O0 \
		-nostdlib \
		-nostartfiles -nodefaultlibs \
		-Wall -Wextra \
		-Wno-varargs \
		-Wno-builtin-declaration-mismatch \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-o tiny_c.o tiny_c.c
	@ar rcs libtinyc.a tiny_c.o

tiny_c_arm_shared: tiny_c.c
	@$(CC) \
		-O0 \
		-nostdlib \
		-nostartfiles -nodefaultlibs \
		-Wall -Wextra \
		-Wno-varargs \
		-Wno-builtin-declaration-mismatch \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-shared \
		-fPIC \
		-o libtinyc.so tiny_c.c

tiny_c: tiny_c.c
	@$(CC) \
		-c \
		-O0 \
		-mno-sse \
		-nostdlib \
		-Wall -Wextra \
		-Wno-varargs \
		-masm=intel \
		-fno-stack-protector \
		-g \
		-DAMD64 \
		-o tiny_c.o tiny_c.c
	@ar rcs libtinyc.a tiny_c.o

loader: loader.c tiny_c.c
	@$(CC) \
		-O0 \
		-mno-sse \
		-nostdlib \
		-Wall -Wextra \
		-Wno-varargs \
		-masm=intel \
		-fPIE \
		-Wl,--section-start=.text=0x00007d7d00000000 \
		-fno-stack-protector \
		-g \
		-o loader loader.c tiny_c.c

loader_arm: loader.c tiny_c.c
	@$(CC) \
		-O0 \
		-nostdlib \
		-Wall -Wextra \
		-Wno-varargs \
		-Wno-builtin-declaration-mismatch \
		-fPIE \
		-Wl,--section-start=.text=7d7d0000 \
		-fno-stack-protector \
		-g \
		-DARM32 \
		-o loader loader.c tiny_c.c

clean:
	@rm -f tiny_wine loader tiny_c.o libtinyc.a libtinyc.so

install: all
	cp tiny_c.h /usr/include
	cp libtinyc.a libtinyc.so /usr/lib
