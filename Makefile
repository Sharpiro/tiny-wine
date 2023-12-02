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

# -Wl,--section-start=.text=0x7d7d00000000 \
# -std=gnu2x \
# -nostartfiles \
# -nodefaultlibs \

loader: loader.c
	@$(CC) \
		-O0 \
		-mno-sse \
		-nostdlib \
		-Wall -Wextra -Wpedantic \
		-masm=intel \
		-fno-stack-protector \
		-g \
		-o loader loader.c


clean:
	@rm -f tiny_wine loader
