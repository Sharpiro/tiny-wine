ifeq ($(CC),cc)
  CC := clang
endif

OBJDUMP ?= objdump

STANDARD_OPTIONS = \
	-std=gnu2x \
	-Wall -Wextra -Wpedantic -Wno-varargs \
	-Wno-gnu-zero-variadic-macro-arguments \
	-Wconversion \
	-Werror=return-type \
	-Werror=incompatible-pointer-types \
	-Wno-gnu-empty-initializer \
	-Wvla \
	-Wno-format-pedantic

all: \
	linux \
	windows

linux: \
	libtinyc.a \
	libtinyc.so \
	loader \
	unit_test \
	env \
	string \
	tinyfetch \
	static_pie \
	dynamic \
	readlin

windows: \
	libtinyc.a \
	libntdll.so \
	ntdll.dll \
	msvcrt.dll \
	KERNEL32.dll \
	winloader \
	readwin \
	windynamiclib.dll \
	windynamiclibfull.dll \
	windynamic.exe \
	windynamicfull.exe

tinyc_start.o: src/tinyc/tinyc_start.c
	@$(CC) $(CFLAGS) \
		-g \
		-c \
		-O0 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-fPIC \
		-DAMD64 \
		-masm=intel \
		-o tinyc_start.o \
		src/tinyc/tinyc_start.c

tinyc_temp: \
	src/dlls/msvcrt.h \
	src/dlls/msvcrt.c \
	src/dlls/msvcrt_linux.c
	@echo "building tinyc_temp..."
	@$(CC) $(CFLAGS) \
		-g \
		-c \
		-O0 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-fPIC \
		-DAMD64 \
		-masm=intel \
		src/dlls/msvcrt.c \
		src/dlls/msvcrt_linux.c

libtinyc.a: tinyc_temp
	@echo "building libtinyc.a..."
	@ar rcs libtinyc.a msvcrt.o msvcrt_linux.o
	@$(OBJDUMP) -M intel -D libtinyc.a > libtinyc.a.dump

libstatic.a: src/programs/linux/string/static_lib.c
	@$(CC) $(CFLAGS) \
		-g \
		-c \
		-O0 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-fPIC \
		-DAMD64 \
		-masm=intel \
		-o static_lib.o src/programs/linux/string/static_lib.c
	@ar rcs libstatic.a static_lib.o
	@$(OBJDUMP) -M intel -D libstatic.a > libstatic.a.dump

libtinyc.so: tinyc_temp
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		-g \
		-DAMD64 \
		-nostdlib -static \
		-shared \
		-o libtinyc.so \
		msvcrt.o \
		msvcrt_linux.o
	@$(OBJDUMP) -M intel -D libtinyc.so > libtinyc.so.dump

libdynamic.so:
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		-g \
		-DAMD64 \
		-nostdlib -static \
		-shared \
		-fPIC \
		-o libdynamic.so \
		src/programs/linux/dynamic/dynamic_lib.c
	@$(OBJDUMP) -M intel -D libdynamic.so > libdynamic.so.dump

libntdll.so: \
		src/dlls/ntdll.h \
		src/dlls/ntdll.c
	@echo "building libntdll.so..."
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		-g \
		-DAMD64 \
		-DDLL \
		-masm=intel \
		-nostdlib \
		-shared \
		-fPIC \
		-o libntdll.so \
		src/dlls/ntdll.c
	@$(OBJDUMP) -M intel -D libntdll.so > libntdll.so.dump

ntdll.dll: \
		src/dlls/ntdll.h \
		src/dlls/ntdll.c \
		libntdll.so
	@echo "building ntdll.dll..."
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
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

msvcrt.dll: \
		src/dlls/msvcrt.h \
		src/dlls/msvcrt.c \
		src/dlls/msvcrt_win.c \
		ntdll.dll
	@echo "building msvcrt.dll..."
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-g \
		-DAMD64 \
		-DDLL \
		-masm=intel \
		-nostdlib \
		-shared \
		-fPIC \
		-o msvcrt.dll \
		src/dlls/msvcrt.c \
		src/dlls/msvcrt_win.c \
		ntdll.dll
	@$(OBJDUMP) -M intel -D msvcrt.dll > msvcrt.dll.dump

KERNEL32.dll: \
		ntdll.dll \
		src/dlls/kernel32.c
	@echo "building KERNEL32.dll..."
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-g \
		-DAMD64 \
		-DDLL \
		-masm=intel \
		-nostdlib \
		-shared \
		-fPIC \
		-o KERNEL32.dll \
		ntdll.dll \
		src/dlls/kernel32.c 
	@$(OBJDUMP) -M intel -D KERNEL32.dll > KERNEL32.dll.dump

windynamiclib.dll: \
		ntdll.dll \
		src/programs/windows/win_dynamic/win_dynamic_lib.h \
		src/programs/windows/win_dynamic/win_dynamic_lib.c
	@echo "building windynamiclib.dll..."
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-g \
		-DAMD64 \
		-DDLL \
		-masm=intel \
		-nostdlib \
		-shared \
		-fPIC \
		-Wl,-e,DllMain \
		-o windynamiclib.dll \
		ntdll.dll \
		src/programs/windows/win_dynamic/win_dynamic_lib.c
	@$(OBJDUMP) -M intel -D windynamiclib.dll > windynamiclib.dll.dump

windynamiclibfull.dll: \
		src/programs/windows/win_dynamic/win_dynamic_lib.h \
		src/programs/windows/win_dynamic/win_dynamic_lib_full.c
	@echo "building windynamiclibfull.dll..."
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-g \
		-DAMD64 \
		-DDLL \
		-masm=intel \
		-shared \
		-fPIC \
		-L/usr/lib/gcc/x86_64-w64-mingw32/10-win32 \
		-o windynamiclibfull.dll \
		src/programs/windows/win_dynamic/win_dynamic_lib_full.c
	@$(OBJDUMP) -M intel -D windynamiclibfull.dll > windynamiclibfull.dll.dump

loader: \
	src/list.h \
	src/loader/memory_map.h \
	src/loader/memory_map.c \
	src/loader/linux/loader_main.c\
	src/loader/linux/loader_lib.h \
	src/loader/linux/loader_lib.c \
	src/loader/linux/elf_tools.h \
	src/loader/linux/elf_tools.c \
	tinyc_start.o \
	libtinyc.a
	@echo "building loader..."
	@$(CC) $(CFLAGS) \
		-O0 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-Wl,--section-start=.text=7d7d0000 \
		-g \
		-DAMD64 \
		-masm=intel \
		-o loader \
		src/loader/memory_map.c \
		src/loader/linux/loader_main.c \
		src/loader/linux/loader_lib.c \
		src/loader/linux/elf_tools.c \
		tinyc_start.o \
		libtinyc.a
	@$(OBJDUMP) -M intel -D loader > loader.dump

winloader: \
		src/list.h \
		src/loader/memory_map.h \
		src/loader/memory_map.c \
		src/loader/windows/win_loader_main.c \
		src/loader/windows/win_loader_lib.h \
		src/loader/windows/win_loader_lib.c \
		src/loader/windows/pe_tools.h \
		src/loader/windows/pe_tools.c \
		src/loader/linux/loader_lib.h \
		src/loader/linux/loader_lib.c \
		src/loader/linux/elf_tools.h \
		src/loader/linux/elf_tools.c \
		tinyc_start.o \
		libtinyc.a 
	@echo "building winloader..."
	@$(CC) $(CFLAGS) \
		-O0 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-Wl,--section-start=.text=7d7d0000 \
		-g \
		-DAMD64 \
		-masm=intel \
		-o winloader \
		src/loader/memory_map.c \
		src/loader/windows/win_loader_main.c \
		src/loader/windows/win_loader_lib.c \
		src/loader/windows/pe_tools.c \
		src/loader/linux/loader_lib.c \
		src/loader/linux/elf_tools.c \
		tinyc_start.o \
		libtinyc.a
	@$(OBJDUMP) -M intel -D winloader > winloader.dump

unit_test: \
		src/programs/linux/unit_test/unit_test_main.c \
		src/loader/memory_map.h \
		src/loader/memory_map.c \
		src/list.h \
		src/loader/linux/loader_lib.h \
		src/loader/linux/loader_lib.c \
		src/loader/windows/win_loader_lib.h \
		src/loader/windows/win_loader_lib.c \
		tinyc_start.o \
		libtinyc.a
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o unit_test \
		src/programs/linux/unit_test/unit_test_main.c \
		src/loader/memory_map.c \
		src/loader/linux/loader_lib.c \
		src/loader/windows/win_loader_lib.c \
		tinyc_start.o \
		libtinyc.a

env: \
		src/programs/linux/env/env_main.c \
		tinyc_start.o \
		libtinyc.a
	@echo "building env..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o env \
		src/programs/linux/env/env_main.c \
		tinyc_start.o \
		libtinyc.a
	@$(OBJDUMP) -M intel -D env > env.dump

string: \
		tinyc_start.o \
		libtinyc.a \
		libstatic.a \
		src/programs/linux/string/string_main.c
	@echo "building string..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o string \
		src/programs/linux/string/string_main.c \
		tinyc_start.o \
		libtinyc.a \
		libstatic.a
	@$(OBJDUMP) -M intel -D string > string.dump

tinyfetch: tinyc_start.o libtinyc.so libdynamic.so
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib \
		-no-pie \
		$(STANDARD_OPTIONS) \
		-o tinyfetch \
		./libtinyc.so \
		./libdynamic.so \
		src/programs/linux/tinyfetch/tinyfetch_main.c \
		tinyc_start.o
	@$(OBJDUMP) -M intel -D tinyfetch > tinyfetch.dump

static_pie: \
	libstatic.a \
	tinyc_start.o \
	libtinyc.a \
	src/programs/linux/string/string_main.c
	@echo "building static_pie..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-DPIE \
		-nostdlib \
		-fPIE -pie \
		$(STANDARD_OPTIONS) \
		-o static_pie \
		src/programs/linux/string/string_main.c \
		tinyc_start.o \
		libtinyc.a \
		libstatic.a
	@$(OBJDUMP) -M intel -D static_pie > static_pie.dump


dynamic: \
	tinyc_start.o \
	libtinyc.so \
	libdynamic.so \
	src/programs/linux/dynamic/dynamic_main.c
	@echo "building dynamic..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		$(STANDARD_OPTIONS) \
		-S \
		-o dynamic.s \
		src/programs/linux/dynamic/dynamic_main.c
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib \
		-no-pie \
		$(STANDARD_OPTIONS) \
		-o dynamic \
		./libdynamic.so \
		./libtinyc.so \
		src/programs/linux/dynamic/dynamic_main.c \
		tinyc_start.o
	@$(OBJDUMP) -M intel -D dynamic > dynamic.dump

windynamic.exe: \
		msvcrt.dll \
		ntdll.dll \
		windynamiclib.dll \
		src/dlls/macros.h \
		src/programs/windows/win_dynamic/win_dynamic_main.c \
		src/programs/windows/win_dynamic/runtime.c
	@echo "building windynamic.exe..."
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-g \
		-DAMD64 \
		-DNO_STDLIB \
		-masm=intel \
		-nostdlib \
		-fPIC \
		-o windynamic.exe \
		ntdll.dll \
		msvcrt.dll \
		windynamiclib.dll \
		src/programs/windows/win_dynamic/win_dynamic_main.c \
		src/programs/windows/win_dynamic/runtime.c
	@$(OBJDUMP) -M intel -D windynamic.exe > windynamic.exe.dump

windynamicfull.exe: \
		winloader \
		msvcrt.dll \
		KERNEL32.dll \
		windynamiclibfull.dll \
		src/programs/windows/win_dynamic/win_dynamic_full_main.c
	@echo "building windynamicfull.exe..."
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-g \
		-DAMD64 \
		-masm=intel \
		-fPIC \
		-L/usr/lib/gcc/x86_64-w64-mingw32/10-win32 \
		-o windynamicfull.exe \
		windynamiclibfull.dll \
		src/programs/windows/win_dynamic/win_dynamic_full_main.c
	@$(OBJDUMP) -M intel -D windynamicfull.exe > windynamicfull.exe.dump

readwin: \
		tools/readwin/readwin_main.c \
		src/list.h \
		src/loader/windows/pe_tools.h \
		src/loader/windows/pe_tools.c \
		tinyc_start.o \
		libtinyc.a
	@echo "building readwin..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-masm=intel \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o readwin \
		tools/readwin/readwin_main.c \
		src/loader/windows/pe_tools.c \
		tinyc_start.o \
		libtinyc.a

readlin: \
		tools/readlin/readlin_main.c \
		tinyc_start.o \
		libtinyc.a \
		src/loader/linux/elf_tools.h \
		src/loader/linux/elf_tools.c
	@echo "building readlin..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-masm=intel \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o readlin \
		tools/readlin/readlin_main.c \
		src/loader/linux/elf_tools.c \
		tinyc_start.o \
		libtinyc.a

clean:
	@rm -f \
		*.dump *.o *.s *.a *.so *.dll *.exe \
		loader env string tinyfetch dynamic unit_test static_pie winloader readwin \
		windynamic_linux readlin
