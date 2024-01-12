CC=gcc

all: tiny_wine loader

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
		-Wl,--section-start=.text=0x7d7d00000000 \
		-fno-stack-protector \
		-g \
		-o loader loader.c tiny_c.c

clean:
	@rm -f tiny_wine loader tiny_c.o libtinyc.a
