CC=gcc

OUTPUT=tiny_wine

all: $(OUTPUT)

$(OUTPUT): *.c *.h
	@$(CC) \
		-Wall -Wextra -Wpedantic \
		-std=gnu2x \
		-masm=intel \
		-Wl,--section-start=.interp=0x900000 \
		-fno-stack-protector \
		-g \
		-o $(OUTPUT) *.c

clean:
	@rm -f $(OUTPUT)
