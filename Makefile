ifeq ($(CC),cc)
  CC := clang
endif

# @todo: object file and deps build doesn't work w/ linux and windows
# @todo: still things called 'tinyc'
# @todo: default PIE/PIC?
# @todo: reduce number of linux test program binaries

STANDARD_OPTIONS = \
	-std=gnu2x \
	-Wall -Wextra -Wpedantic -Wno-varargs \
	-Wno-gnu-zero-variadic-macro-arguments \
	-Wconversion \
	-Werror=return-type \
	-Werror=incompatible-pointer-types \
	-Wno-gnu-empty-initializer \
	-Wvla \
	-Wno-format-pedantic \
	-Iinclude \
	-MMD -MP
	
DEP = $(shell find build -name "*.d")

-include $(DEP)

all: \
	linux \
	windows

linux: \
	build/linloader \
	build/unit_test \
	build/env \
	build/string \
	build/tinyfetch \
	build/static_pie \
	build/dynamic \
	build/readlin

windows: \
	build/winloader \
	build/readwin \
	build/windynamic.exe \
	build/windynamicfull.exe

build/linux/%.o: %.c
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
		-DLINUX \
		-masm=intel \
		$< \
		-o $@

build/libtinyc.a: \
		build/linux/src/dlls/twlibc.o \
		build/linux/src/dlls/sys_linux.o
	@echo "building libtinyc.a..."
	@ar rcs build/libtinyc.a \
		build/linux/src/dlls/twlibc.o \
		build/linux/src/dlls/sys_linux.o

build/libtinyc.so: \
		build/linux/src//dlls/twlibc.o \
		build/linux/src//dlls/sys_linux.o
	@echo "building libtinyc.so..."
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		-g \
		-DAMD64 \
		-nostdlib -static \
		-shared \
		-o build/libtinyc.so \
		build/linux/src//dlls/twlibc.o \
		build/linux/src//dlls/sys_linux.o

build/libdynamic.so: \
		build/linux/src/programs/linux/dynamic/dynamic_lib.o
	@echo "building libdynamic.so..."
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		-g \
		-DAMD64 \
		-nostdlib -static \
		-shared \
		-fPIC \
		-o build/libdynamic.so \
		build/linux/src/programs/linux/dynamic/dynamic_lib.o

build/libntdll.so: \
		build/linux/src/dlls/ntdll.o \
		build/linux/src/dlls/sys_linux.o
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
		build/linux/src/dlls/ntdll.o \
		build/linux/src/dlls/sys_linux.o

build/ntdll.dll: \
		build/libntdll.so \
		src/dlls/ntdll.c \
		src/dlls/sys_linux.c
	@echo "building ntdll.dll..."
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
		-o build/ntdll.dll \
		src/dlls/sys_linux.c \
		src/dlls/ntdll.c

build/msvcrt.dll: \
		build/ntdll.dll \
		src/dlls/twlibc.c \
		src/dlls/twlibc_win.c
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
		src/dlls/twlibc.c \
		src/dlls/twlibc_win.c \
		build/ntdll.dll

build/KERNEL32.dll: \
		build/ntdll.dll \
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

build/windynamiclib.dll: \
		build/ntdll.dll \
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

build/windynamiclibfull.dll: \
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

linloader: build/linloader
build/linloader: \
	build/linux/src/loader/memory_map.o \
	build/linux/src/loader/linux/loader_main.o\
	build/linux/src/loader/linux/loader_lib.o \
	build/linux/src/loader/linux/elf_tools.o \
	build/linux/src/programs/linux/linux_runtime.o \
	build/libtinyc.a
	@echo "building linloader..."
	@$(CC) $(CFLAGS) \
		-O0 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-Wl,--section-start=.text=7d7d0000 \
		-g \
		-DAMD64 \
		-masm=intel \
		-o build/linloader \
		build/linux/src/loader/memory_map.o \
		build/linux/src/loader/linux/loader_main.o \
		build/linux/src/loader/linux/loader_lib.o \
		build/linux/src/loader/linux/elf_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a

winloader:build/winloader
build/winloader: \
		src/loader/windows/win_loader_main.c \
		build/linux/src/loader/memory_map.o \
		build/linux/src/loader/windows/win_loader_lib.o \
		build/linux/src/loader/windows/pe_tools.o \
		build/linux/src/loader/linux/loader_lib.o \
		build/linux/src/loader/linux/elf_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
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
		src/loader/windows/win_loader_main.c \
		build/linux/src/loader/memory_map.o \
		build/linux/src/loader/windows/win_loader_lib.o \
		build/linux/src/loader/windows/pe_tools.o \
		build/linux/src/loader/linux/loader_lib.o \
		build/linux/src/loader/linux/elf_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a

build/unit_test: \
		build/linux/src//programs/linux/unit_test/unit_test_main.o \
		build/linux/src//loader/memory_map.o \
		build/linux/src//loader/linux/loader_lib.o \
		build/linux/src//loader/windows/win_loader_lib.o \
		build/linux/src//programs/linux/linux_runtime.o \
		build/libtinyc.a
	@echo "building unit_test..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o build/unit_test \
		build/linux/src//programs/linux/unit_test/unit_test_main.o \
		build/linux/src//loader/memory_map.o \
		build/linux/src//loader/linux/loader_lib.o \
		build/linux/src//loader/windows/win_loader_lib.o \
		build/linux/src//programs/linux/linux_runtime.o \
		build/libtinyc.a

build/env: \
		build/linux/src/programs/linux/env/env_main.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a
	@echo "building env..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o build/env \
		build/linux/src/programs/linux/env/env_main.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a

build/string: \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/string/static_lib.o \
		build/linux/src/programs/linux/string/string_main.o \
		build/libtinyc.a
	@echo "building string..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o build/string \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/string/static_lib.o \
		build/linux/src/programs/linux/string/string_main.o \
		build/libtinyc.a

build/tinyfetch: \
		build/libtinyc.so \
		build/libdynamic.so \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/tinyfetch/tinyfetch_main.o
	@echo "building tinyfetch..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-nostdlib \
		-no-pie \
		$(STANDARD_OPTIONS) \
		-Wl,-rpath,'$$ORIGIN' -L./build \
 		-ltinyc \
 		-ldynamic \
		-o build/tinyfetch \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/tinyfetch/tinyfetch_main.o

build/static_pie: \
		build/linux/src/programs/linux/string/static_lib.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/string/string_main.o \
		build/libtinyc.a
	@echo "building static_pie..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-DPIE \
		-nostdlib \
		-fPIE -pie \
		$(STANDARD_OPTIONS) \
		-o build/static_pie \
		build/linux/src/programs/linux/string/static_lib.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/string/string_main.o \
		build/libtinyc.a


build/dynamic: \
		build/libtinyc.so \
		build/libdynamic.so \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/dynamic/dynamic_main.o
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
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/dynamic/dynamic_main.o

build/windynamic.exe: \
		build/msvcrt.dll \
		build/ntdll.dll \
		build/windynamiclib.dll \
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

build/windynamicfull.exe: \
		build/winloader \
		build/msvcrt.dll \
		build/KERNEL32.dll \
		build/windynamiclibfull.dll \
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

readwin: build/readwin
build/readwin: \
		build/linux/tools/readwin/readwin_main.o \
		build/linux/src/loader/windows/pe_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a
	@echo "building readwin..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-masm=intel \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o build/readwin \
		build/linux/tools/readwin/readwin_main.o \
		build/linux/src/loader/windows/pe_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a

readlin: build/readlin
build/readlin: \
		build/linux/tools/readlin/readlin_main.o \
		build/linux/src/loader/linux/elf_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a
	@echo "building readlin..."
	@$(CC) $(CFLAGS) -g \
		-DAMD64 \
		-masm=intel \
		-nostdlib -static \
		$(STANDARD_OPTIONS) \
		-o build/readlin \
		build/linux/tools/readlin/readlin_main.o \
		build/linux/src/loader/linux/elf_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a

dump:
	@files=$$(find build -type f ! -name "*.dump" -not -path '*/\.*' -not -path '*.d'); \
	for file in $$files; do \
	    echo "dumping $$file"; \
	    objdump -M intel -D "$$file" > "$$file.dump"; \
	done

clean:
	@rm -rf ./build/*
	@rm -f \
		*.dump *.o *.s *.a *.so *.dll *.exe \
		loader env string tinyfetch dynamic unit_test static_pie winloader readwin \
		windynamic_linux readlin
