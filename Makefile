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

loader: loader.c
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
		-o loader loader.c

clean:
	@rm -f tiny_wine loader
