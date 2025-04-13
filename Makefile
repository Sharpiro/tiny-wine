ifeq ($(CC),cc)
  CC := clang
endif

# @todo: migrate to build dir
# @todo: generate header deps -MMD and -MP
# @todo: move objdump to script or enable w/ env var?
# @todo: merge tinyc and msvcrt?
# @todo: reduce number of linux test program binaries
# @todo: dedicated include directory?

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
	loader \
	unit_test \
	env \
	string \
	tinyfetch \
	static_pie \
	dynamic \
	readlin

windows: \
	winloader \
	readwin \
	windynamic.exe \
	windynamicfull.exe

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	@echo "building $@..."
	@$(CC) $(CFLAGS) \
		-g \
		-c \
		-O0 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-fPIC \
		-DAMD64 \
		-masm=intel \
		$< \
		-o $@

build/libtinyc.a: build/dlls/msvcrt.o build/dlls/msvcrt_linux.o
	@echo "building libtinyc.a..."
	@ar rcs build/libtinyc.a build/dlls/msvcrt.o build/dlls/msvcrt_linux.o
	@# @$(OBJDUMP) -M intel -D libtinyc.a > libtinyc.a.dump

build/libtinyc.so: build/dlls/msvcrt.o build/dlls/msvcrt_linux.o
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		-g \
		-DAMD64 \
		-nostdlib -static \
		-shared \
		-o build/libtinyc.so \
		build/dlls/msvcrt.o \
		build/dlls/msvcrt_linux.o
	@# @$(OBJDUMP) -M intel -D libtinyc.so > libtinyc.so.dump

build/libdynamic.so:
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		-g \
		-DAMD64 \
		-nostdlib -static \
		-shared \
		-fPIC \
		-o build/libdynamic.so \
		src/programs/linux/dynamic/dynamic_lib.c
	@# @$(OBJDUMP) -M intel -D libdynamic.so > libdynamic.so.dump

build/libntdll.so: \
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
		-o build/libntdll.so \
		src/dlls/ntdll.c
	@# @$(OBJDUMP) -M intel -D libntdll.so > libntdll.so.dump

ntdll.dll: \
		src/dlls/ntdll.h \
		src/dlls/ntdll.c \
		build/libntdll.so
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
		-o build/ntdll.dll \
		src/dlls/ntdll.c
	@# @$(OBJDUMP) -M intel -D ntdll.dll > ntdll.dll.dump

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
		-o build/msvcrt.dll \
		src/dlls/msvcrt.c \
		src/dlls/msvcrt_win.c \
		build/ntdll.dll
	@# @$(OBJDUMP) -M intel -D msvcrt.dll > msvcrt.dll.dump

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
		-o build/KERNEL32.dll \
		build/ntdll.dll \
		src/dlls/kernel32.c 
	@# @$(OBJDUMP) -M intel -D KERNEL32.dll > KERNEL32.dll.dump

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
		-o build/windynamiclib.dll \
		build/ntdll.dll \
		src/programs/windows/win_dynamic/win_dynamic_lib.c
	@# @$(OBJDUMP) -M intel -D windynamiclib.dll > windynamiclib.dll.dump

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
		-o build/windynamiclibfull.dll \
		src/programs/windows/win_dynamic/win_dynamic_lib_full.c
	@# @$(OBJDUMP) -M intel -D windynamiclibfull.dll > windynamiclibfull.dll.dump

loader: \
	src/list.h \
	src/loader/memory_map.h \
	src/loader/memory_map.c \
	src/loader/linux/loader_main.c\
	src/loader/linux/loader_lib.h \
	src/loader/linux/loader_lib.c \
	src/loader/linux/elf_tools.h \
	src/loader/linux/elf_tools.c \
	build/tinyc/linux_runtime.o \
	build/libtinyc.a
	@echo "building loader..."
	@$(CC) $(CFLAGS) \
		-O0 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-Wl,--section-start=.text=7d7d0000 \
		-g \
		-DAMD64 \
		-masm=intel \
		-o build/loader \
		src/loader/memory_map.c \
		src/loader/linux/loader_main.c \
		src/loader/linux/loader_lib.c \
		src/loader/linux/elf_tools.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a
	@# @$(OBJDUMP) -M intel -D loader > loader.dump

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
		build/tinyc/linux_runtime.o \
		build/libtinyc.a 
	@echo "building winloader..."
	@$(CC) $(CFLAGS) \
		-O0 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-Wl,--section-start=.text=7d7d0000 \
		-g \
		-DAMD64 \
		-masm=intel \
		-o build/winloader \
		src/loader/memory_map.c \
		src/loader/windows/win_loader_main.c \
		src/loader/windows/win_loader_lib.c \
		src/loader/windows/pe_tools.c \
		src/loader/linux/loader_lib.c \
		src/loader/linux/elf_tools.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a
	@# @$(OBJDUMP) -M intel -D winloader > winloader.dump

unit_test: \
		src/programs/linux/unit_test/unit_test_main.c \
		src/loader/memory_map.h \
		src/loader/memory_map.c \
		src/list.h \
		src/loader/linux/loader_lib.h \
		src/loader/linux/loader_lib.c \
		src/loader/windows/win_loader_lib.h \
		src/loader/windows/win_loader_lib.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o build/unit_test \
		src/programs/linux/unit_test/unit_test_main.c \
		src/loader/memory_map.c \
		src/loader/linux/loader_lib.c \
		src/loader/windows/win_loader_lib.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a

env: \
		src/programs/linux/env/env_main.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a
	@echo "building env..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o build/env \
		src/programs/linux/env/env_main.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a
	@# @$(OBJDUMP) -M intel -D env > env.dump

string: \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a \
		build/programs/linux/string/static_lib.o \
		src/programs/linux/string/string_main.c
	@echo "building string..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o build/string \
		src/programs/linux/string/string_main.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a \
		build/programs/linux/string/static_lib.o
	@# @$(OBJDUMP) -M intel -D string > string.dump

tinyfetch: \
	build/tinyc/linux_runtime.o \
	build/libtinyc.so \
	build/libdynamic.so
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib \
		-no-pie \
		$(STANDARD_OPTIONS) \
		-Wl,-rpath,'$$ORIGIN' -L./build \
 		-ltinyc \
 		-ldynamic \
		-o build/tinyfetch \
		src/programs/linux/tinyfetch/tinyfetch_main.c \
		build/tinyc/linux_runtime.o
	@# @$(OBJDUMP) -M intel -D tinyfetch > tinyfetch.dump

static_pie: \
	build/programs/linux/string/static_lib.o \
	build/tinyc/linux_runtime.o \
	build/libtinyc.a \
	src/programs/linux/string/string_main.c
	@echo "building static_pie..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-DPIE \
		-nostdlib \
		-fPIE -pie \
		$(STANDARD_OPTIONS) \
		-o build/static_pie \
		src/programs/linux/string/string_main.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a \
		build/programs/linux/string/static_lib.o
	@# @$(OBJDUMP) -M intel -D static_pie > static_pie.dump


dynamic: \
	build/tinyc/linux_runtime.o \
	build/libtinyc.so \
	build/libdynamic.so \
	src/programs/linux/dynamic/dynamic_main.c
	@echo "building dynamic..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib \
		-no-pie \
		$(STANDARD_OPTIONS) \
		-Wl,-rpath,'$$ORIGIN' -L./build \
 		-ldynamic \
 		-ltinyc \
		-o build/dynamic \
		src/programs/linux/dynamic/dynamic_main.c \
		build/tinyc/linux_runtime.o
	@# @$(OBJDUMP) -M intel -D dynamic > dynamic.dump

windynamic.exe: \
		msvcrt.dll \
		ntdll.dll \
		windynamiclib.dll \
		src/dlls/macros.h \
		src/programs/windows/win_dynamic/win_dynamic_main.c \
		src/programs/windows/win_dynamic/win_runtime.c
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
		-o build/windynamic.exe \
		build/ntdll.dll \
		build/msvcrt.dll \
		build/windynamiclib.dll \
		src/programs/windows/win_dynamic/win_dynamic_main.c \
		src/programs/windows/win_dynamic/win_runtime.c
	@# @$(OBJDUMP) -M intel -D windynamic.exe > windynamic.exe.dump

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
		-o build/windynamicfull.exe \
		build/windynamiclibfull.dll \
		src/programs/windows/win_dynamic/win_dynamic_full_main.c
	@# @$(OBJDUMP) -M intel -D windynamicfull.exe > windynamicfull.exe.dump

readwin: \
		tools/readwin/readwin_main.c \
		src/list.h \
		src/loader/windows/pe_tools.h \
		src/loader/windows/pe_tools.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a
	@echo "building readwin..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-masm=intel \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o build/readwin \
		tools/readwin/readwin_main.c \
		src/loader/windows/pe_tools.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a

readlin: \
		tools/readlin/readlin_main.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a \
		src/loader/linux/elf_tools.h \
		src/loader/linux/elf_tools.c
	@echo "building readlin..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-masm=intel \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o build/readlin \
		tools/readlin/readlin_main.c \
		src/loader/linux/elf_tools.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a

clean:
	@rm -rf ./build/*
	@rm -f \
		*.dump *.o *.s *.a *.so *.dll *.exe \
		loader env string tinyfetch dynamic unit_test static_pie winloader readwin \
		windynamic_linux readlin
