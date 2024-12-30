ifeq ($(CC),cc)
  CC := clang
endif

OBJDUMP ?= objdump
WARNINGS = \
	-std=gnu2x \
	-Wall -Wextra -Wpedantic -Wno-varargs \
	-Wno-gnu-zero-variadic-macro-arguments \
	-Wno-gnu-statement-expression-from-macro-expansion \
	-Wconversion \
    -Werror=return-type \
    -Werror=incompatible-pointer-types \
	-Wvla

all: \
	libtinyc.a \
	libtinyc.so \
	libntdll.so \
	libmsvcrt.so \
	ntdll.dll \
	msvcrt.dll \
	loader \
	winloader \
	programs/linux/unit_test \
	programs/linux/env \
	programs/linux/string \
	programs/linux/tinyfetch \
	programs/linux/static_pie \
	programs/linux/dynamic \
	programs/windows/win_dynamic \
	programs/windows/win_dynamic_linux \
	tools/readwin

tinyc_start.o: src/tiny_c/tinyc_start.c
	@$(CC) $(CFLAGS) \
		-g \
		-c \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-fPIC \
		-fno-stack-protector \
		-DAMD64 \
		-masm=intel \
		-mno-sse \
		-o tinyc_start.o \
		src/tiny_c/tinyc_start.c

tinyc_sys.o: src/tiny_c/tiny_c.c
	@$(CC) $(CFLAGS) \
		-g \
		-c \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-fPIC \
		-fno-stack-protector \
		-DAMD64 \
		-masm=intel \
		-mno-sse \
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
		-DAMD64 \
		-masm=intel \
		-mno-sse \
		-o tiny_c.o src/tiny_c/tiny_c.c

libtinyc.a: tinyc_sys.o tiny_c.o
	@ar rcs libtinyc.a tinyc_sys.o tiny_c.o
	@$(OBJDUMP) -M intel -D libtinyc.a > libtinyc.a.dump

libstatic.a: src/programs/linux/string/static_lib.c
	@$(CC) $(CFLAGS) \
		-g \
		-c \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-fPIC \
		-fno-stack-protector \
		-DAMD64 \
		-masm=intel \
		-mno-sse \
		-o static_lib.o src/programs/linux/string/static_lib.c
	@ar rcs libstatic.a static_lib.o
	@$(OBJDUMP) -M intel -D libstatic.a > libstatic.a.dump

libtinyc.so: tinyc_sys.o tiny_c.o
	@$(CC) $(CFLAGS) \
		-O0 \
		$(WARNINGS) \
		-fno-stack-protector \
		-g \
		-DAMD64 \
		-nostdlib -static \
		-shared \
		-o libtinyc.so \
		tinyc_sys.o \
		tiny_c.o
	@$(OBJDUMP) -M intel -D libtinyc.so > libtinyc.so.dump

libdynamic.so:
	@$(CC) $(CFLAGS) \
		-O0 \
		$(WARNINGS) \
		-fno-stack-protector \
		-g \
		-DAMD64 \
		-nostdlib -static \
		-shared \
		-fPIC \
		-o libdynamic.so \
		src/programs/linux/dynamic/dynamic_lib.c
	@$(OBJDUMP) -M intel -D libdynamic.so > libdynamic.so.dump

libntdll.so:
	@$(CC) $(CFLAGS) \
		-O0 \
		$(WARNINGS) \
		-fno-stack-protector \
		-g \
		-DAMD64 \
		-masm=intel \
		-nostdlib \
		-shared \
		-fPIC \
		-o libntdll.so \
		src/dlls/ntdll.c
	@$(OBJDUMP) -M intel -D libntdll.so > libntdll.so.dump

libmsvcrt.so: libntdll.so
	@$(CC) $(CFLAGS) \
		-O0 \
		$(WARNINGS) \
		-fno-stack-protector \
		-g \
		-DAMD64 \
		-masm=intel \
		-nostdlib \
		-shared \
		-fPIC \
		-o libmsvcrt.so \
		./libntdll.so \
		src/dlls/msvcrt.c
	@$(OBJDUMP) -M intel -D libmsvcrt.so > libmsvcrt.so.dump

ntdll.dll:
	@$(CC) $(CFLAGS) \
		-O0 \
		$(WARNINGS) \
		-fno-stack-protector \
		--target=x86_64-w64-windows-gnu \
		-g \
		-DAMD64 \
		-masm=intel \
		-nostdlib \
		-shared \
		-fPIC \
		-o ntdll.dll \
		src/dlls/ntdll.c
	@$(OBJDUMP) -M intel -D ntdll.dll > ntdll.dll.dump

msvcrt.dll: ntdll.dll
	@$(CC) $(CFLAGS) \
		-O0 \
		$(WARNINGS) \
		-fno-stack-protector \
		--target=x86_64-w64-windows-gnu \
		-g \
		-DAMD64 \
		-masm=intel \
		-nostdlib \
		-shared \
		-fPIC \
		-o msvcrt.dll \
		src/dlls/msvcrt.c \
		ntdll.dll
	@$(OBJDUMP) -M intel -D msvcrt.dll > msvcrt.dll.dump

loader: tinyc_start.o libtinyc.a src/loader/loader_main.c
	@$(CC) $(CFLAGS) \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-Wl,--section-start=.text=7d7d0000 \
		-fno-stack-protector \
		-g \
		-DAMD64 \
		-masm=intel \
		-mno-sse \
		-o loader \
		src/loader/loader_main.c \
		src/loader/loader_lib.c \
		src/loader/memory_map.c \
		src/loader/elf_tools.c \
		tinyc_start.o \
		libtinyc.a
	@$(OBJDUMP) -M intel -D loader > loader.dump

winloader: tinyc_start.o libtinyc.a src/loader/win_loader_main.c
	@$(CC) $(CFLAGS) \
		-O0 \
		-nostdlib -static \
		$(WARNINGS) \
		-Wl,--section-start=.text=7d7d0000 \
		-fno-stack-protector \
		-g \
		-DAMD64 \
		-masm=intel \
		-mno-sse \
		-o winloader \
		src/loader/win_loader_main.c \
		src/loader/loader_lib.c \
		src/loader/memory_map.c \
		src/loader/pe_tools.c \
		src/loader/elf_tools.c \
		tinyc_start.o \
		libtinyc.a
	@$(OBJDUMP) -M intel -D winloader > winloader.dump

programs/linux/unit_test: tinyc_start.o libtinyc.a
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(WARNINGS) \
		-o unit_test \
		src/programs/linux/unit_test/unit_test_main.c \
		src/loader/memory_map.c \
		src/loader/loader_lib.c \
		src/loader/pe_tools.c \
		tinyc_start.o \
		libtinyc.a

programs/linux/env: tinyc_start.o libtinyc.a
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(WARNINGS) \
		-o env \
		src/programs/linux/env/env_main.c \
		tinyc_start.o \
		libtinyc.a
	@$(OBJDUMP) -M intel -D env > env.dump

programs/linux/string: libtinyc.a libstatic.a
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(WARNINGS) \
		-o string \
		src/programs/linux/string/string_main.c \
		tinyc_start.o \
		libtinyc.a \
		libstatic.a
	@$(OBJDUMP) -M intel -D string > string.dump

programs/linux/tinyfetch: tinyc_start.o libtinyc.so libdynamic.so
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib \
		-no-pie \
		$(WARNINGS) \
		-o tinyfetch \
		./libtinyc.so \
		./libdynamic.so \
		src/programs/linux/tinyfetch/tinyfetch_main.c \
		tinyc_start.o
	@$(OBJDUMP) -M intel -D tinyfetch > tinyfetch.dump

programs/linux/static_pie: tinyc_start.o libtinyc.a
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib \
		-static-pie \
		$(WARNINGS) \
		-o static_pie \
		src/programs/linux/static_pie/static_pie_main.c \
		tinyc_start.o \
		libtinyc.a
	@$(OBJDUMP) -M intel -D static_pie > static_pie.dump

programs/linux/dynamic: libtinyc.so libdynamic.so
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		$(WARNINGS) \
		-S \
		-o dynamic.s \
		src/programs/linux/dynamic/dynamic_main.c
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-mno-sse \
		-nostdlib \
		-no-pie \
		$(WARNINGS) \
		-o dynamic \
		./libdynamic.so \
		./libtinyc.so \
		src/programs/linux/dynamic/dynamic_main.c \
		tinyc_start.o
	@$(OBJDUMP) -M intel -D dynamic > dynamic.dump

programs/windows/win_dynamic: msvcrt.dll ntdll.dll
	@$(CC) $(CFLAGS) \
		-O0 \
		$(WARNINGS) \
		-fno-stack-protector \
		--target=x86_64-w64-windows-gnu \
		-g \
		-DAMD64 \
		-masm=intel \
		-nostdlib \
		-fPIC \
		-o windynamic.exe \
		ntdll.dll \
		msvcrt.dll \
		src/programs/windows/win_dynamic/win_dynamic_main.c
	@$(OBJDUMP) -D windynamic.exe > windynamic.exe.dump

programs/windows/win_dynamic_linux: libmsvcrt.so libntdll.so
	@$(CC) $(CFLAGS) \
		-O0 \
		$(WARNINGS) \
		-fno-stack-protector \
		-g \
		-DAMD64 \
		-masm=intel \
		-nostdlib \
		-fPIC \
		-o windynamic_linux \
		-Wl,-rpath,. \
		./libntdll.so \
		./libmsvcrt.so \
		src/programs/windows/win_dynamic/win_dynamic_main.c
	@$(OBJDUMP) -D windynamic_linux > windynamic_linux.dump

tools/readwin: tools/readwin/readwin_main.c tinyc_start.o libtinyc.a
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(WARNINGS) \
		-o readwin \
		tools/readwin/readwin_main.c \
		src/loader/loader_lib.c \
		src/loader/pe_tools.c \
		src/loader/memory_map.c \
		tinyc_start.o \
		libtinyc.a

clean:
	@rm -f \
		*.dump *.o *.s *.a *.so *.dll *.exe \
		loader env string tinyfetch dynamic unit_test static_pie winloader readwin \
		windynamic_linux

install: libtinyc.a libtinyc.so
	cp src/tiny_c/tiny_c.h /usr/local/include
	cp libtinyc.a libtinyc.so /usr/local/lib
