ifeq ($(CC),cc)
  CC := clang
endif

STANDARD_COMPILER_OPTIONS = \
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
	-MMD -MP \
	-fPIC \
	-g \
	-O0 \
	-masm=intel

all: \
	linux \
	windows

DEPS = $(shell find build -name "*.d")

-include $(DEPS)

linux: \
	build/linloader \
	build/unit_test \
	build/env \
	build/string \
	build/tinyfetch \
	build/string_pie \
	build/dynamic \
	build/dynamic_pie \
	build/readlin

windows: \
	build/winloader \
	build/readwin \
 	build/readwin.exe \
	build/windynamic.exe \
	build/windynamicfull.exe

build/linux/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "building $@..."
	@$(CC) \
		$(STANDARD_COMPILER_OPTIONS) \
 		$(CFLAGS) \
		-c \
		-DLINUX \
		$< \
		-o $@

build/windows/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "building $@..."
	@$(CC) \
		$(STANDARD_COMPILER_OPTIONS) \
 		$(CFLAGS) \
		-c \
		--target=x86_64-w64-windows-gnu \
		$< \
		-o $@

build/libtinyc.a: \
		build/linux/src/dlls/sys_linux.o \
		build/linux/src/dlls/twlibc.o \
		build/linux/src/dlls/twlibc_linux.o
	@echo "building libtinyc.a..."
	@ar rcs build/libtinyc.a \
		build/linux/src/dlls/sys_linux.o \
		build/linux/src/dlls/twlibc.o \
		build/linux/src/dlls/twlibc_linux.o

build/libtinyc.so: \
		build/linux/src/dlls/sys_linux.o \
		build/linux/src/dlls/twlibc.o \
		build/linux/src/dlls/twlibc_linux.o
	@echo "building libtinyc.so..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib -static \
		-shared \
		-o build/libtinyc.so \
		build/linux/src/dlls/sys_linux.o \
		build/linux/src/dlls/twlibc.o \
		build/linux/src/dlls/twlibc_linux.o

build/libdynamic.so: \
		build/linux/src/programs/linux/dynamic/dynamic_lib.o
	@echo "building libdynamic.so..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib -static \
		-shared \
		-o build/libdynamic.so \
		build/linux/src/programs/linux/dynamic/dynamic_lib.o

build/libntdll.so: \
		build/linux/src/dlls/ntdll.o \
		build/linux/src/dlls/sys_linux.o
	@echo "building libntdll.so..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-DDLL \
		-nostdlib \
		-shared \
		-o build/libntdll.so \
		build/linux/src/dlls/ntdll.o \
		build/linux/src/dlls/sys_linux.o

build/windows/src/dlls/ntdll.o: CFLAGS += "-DDLL"
build/windows/src/dlls/sys_linux.o: CFLAGS += "-DDLL"
build/ntdll.dll: \
		build/libntdll.so \
		build/windows/src/dlls/ntdll.o \
		build/windows/src/dlls/sys_linux.o
	@echo "building ntdll.dll..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-DDLL \
		-nostdlib \
		-shared \
		-o build/ntdll.dll \
		build/windows/src/dlls/ntdll.o \
		build/windows/src/dlls/sys_linux.o

build/windows/src/dlls/twlibc.o: CFLAGS += "-DDLL"
build/windows/src/dlls/twlibc_win.o: CFLAGS += "-DDLL"
build/windows/src/dlls/twlibc_linux.o: CFLAGS += "-DDLL"
build/msvcrt.dll: \
		build/ntdll.dll \
		build/windows/src/dlls/twlibc.o \
		build/windows/src/dlls/twlibc_win.o \
		build/windows/src/dlls/twlibc_linux.o
	@echo "building msvcrt.dll..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-DDLL \
		-nostdlib \
		-shared \
		-o build/msvcrt.dll \
		build/windows/src/dlls/twlibc.o \
		build/windows/src/dlls/twlibc_win.o \
		build/windows/src/dlls/twlibc_linux.o \
		build/ntdll.dll

build/KERNEL32.dll: \
		build/ntdll.dll \
		build/windows/src/dlls/kernel32.o
	@echo "building KERNEL32.dll..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-DDLL \
		-nostdlib \
		-shared \
		-o build/KERNEL32.dll \
		build/ntdll.dll \
		build/windows/src/dlls/kernel32.o

build/windynamiclib.dll: \
		build/ntdll.dll \
		build/windows/src/programs/windows/win_dynamic/win_dynamic_lib.o
	@echo "building windynamiclib.dll..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-DDLL \
		-nostdlib \
		-shared \
		-Wl,-e,DllMain \
		-o build/windynamiclib.dll \
		build/ntdll.dll \
		build/windows/src/programs/windows/win_dynamic/win_dynamic_lib.o

build/windynamiclibfull.dll: \
		build/windows/src/programs/windows/win_dynamic/win_dynamic_lib_full.o
	@echo "building windynamiclibfull.dll..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-DDLL \
		-shared \
		-L/usr/lib/gcc/x86_64-w64-mingw32/10-win32 \
		-o build/windynamiclibfull.dll \
		build/windows/src/programs/windows/win_dynamic/win_dynamic_lib_full.o

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
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib -static \
		-Wl,--section-start=.text=7d7d0000 \
		-o build/linloader \
		build/linux/src/loader/memory_map.o \
		build/linux/src/loader/linux/loader_main.o \
		build/linux/src/loader/linux/loader_lib.o \
		build/linux/src/loader/linux/elf_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a

# @note: '-fno-pic' required b/c winloader asm uses a global variable
build/linux/src/loader/windows/win_loader_main.o: CFLAGS += -fno-pic
winloader:build/winloader
build/winloader: \
		build/linux/src/loader/windows/win_loader_main.o \
		build/linux/src/loader/memory_map.o \
		build/linux/src/loader/windows/win_loader_lib.o \
		build/linux/src/loader/windows/pe_tools.o \
		build/linux/src/loader/linux/loader_lib.o \
		build/linux/src/loader/linux/elf_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a
	@echo "building winloader..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib -static \
		-Wl,--section-start=.text=7d7d0000 \
		-o build/winloader \
		build/linux/src/loader/windows/win_loader_main.o \
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
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib -static \
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
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib -static \
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
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib -static \
		-o build/string \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/string/static_lib.o \
		build/linux/src/programs/linux/string/string_main.o \
		build/libtinyc.a

build/string_pie: \
		build/linux/src/programs/linux/string/static_lib.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/string/string_main.o \
		build/libtinyc.a
	@echo "building string_pie..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-pie \
		-nostdlib \
		-o build/string_pie \
		build/linux/src/programs/linux/string/static_lib.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/string/string_main.o \
		build/libtinyc.a

build/tinyfetch: \
		build/libtinyc.so \
		build/libdynamic.so \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/tinyfetch/tinyfetch_main.o
	@echo "building tinyfetch..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib \
		-no-pie \
		-Wl,-rpath,'$$ORIGIN' -L./build \
 		-ltinyc \
 		-ldynamic \
		-o build/tinyfetch \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/tinyfetch/tinyfetch_main.o

build/dynamic: \
		build/libtinyc.so \
		build/libdynamic.so \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/dynamic/dynamic_main.o
	@echo "building dynamic..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib \
		-no-pie \
		-Wl,-rpath,'$$ORIGIN' -L./build \
 		-ldynamic \
 		-ltinyc \
		-o build/dynamic \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/dynamic/dynamic_main.o

build/dynamic_pie: \
		build/libtinyc.so \
		build/libdynamic.so \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/dynamic/dynamic_main.o
	@echo "building dynamic..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib \
		-pie \
		-Wl,-rpath,'$$ORIGIN' -L./build \
 		-ldynamic \
 		-ltinyc \
		-o build/dynamic_pie \
		build/linux/src/programs/linux/linux_runtime.o \
		build/linux/src/programs/linux/dynamic/dynamic_main.o

build/windynamic.exe: \
		build/msvcrt.dll \
		build/ntdll.dll \
		build/windynamiclib.dll \
		build/windows/src/programs/windows/win_dynamic/win_dynamic_main.o \
		build/windows/src/programs/windows/win_dynamic/win_runtime.o
	@echo "building windynamic.exe..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-nostdlib \
		-o build/windynamic.exe \
		build/ntdll.dll \
		build/msvcrt.dll \
		build/windynamiclib.dll \
		build/windows/src/programs/windows/win_dynamic/win_dynamic_main.o \
		build/windows/src/programs/windows/win_dynamic/win_runtime.o

build/windynamicfull.exe: \
		build/msvcrt.dll \
		build/KERNEL32.dll \
		build/windynamiclibfull.dll \
		build/windows/src/programs/windows/win_dynamic/win_dynamic_full_main.o
	@echo "building windynamicfull.exe..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-L/usr/lib/gcc/x86_64-w64-mingw32/10-win32 \
		-o build/windynamicfull.exe \
		build/windynamiclibfull.dll \
		build/windows/src/programs/windows/win_dynamic/win_dynamic_full_main.o

readwin: build/readwin
build/readwin: \
		build/linux/tools/readwin/readwin_main.o \
		build/linux/src/loader/windows/pe_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a
	@echo "building readwin..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib -static \
		-o build/readwin \
		build/linux/tools/readwin/readwin_main.o \
		build/linux/src/loader/windows/pe_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a

build/readwin.exe: \
		build/windows/tools/readwin/readwin_main.o \
		build/windows/src/loader/windows/pe_tools.o \
		build/msvcrt.dll
	@echo "building readwin.exe..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		--target=x86_64-w64-windows-gnu \
		-L/usr/lib/gcc/x86_64-w64-mingw32/10-win32 \
		-o build/readwin.exe \
		build/msvcrt.dll \
		build/windows/tools/readwin/readwin_main.o \
		build/windows/src/loader/windows/pe_tools.o

readlin: build/readlin
build/readlin: \
		build/linux/tools/readlin/readlin_main.o \
		build/linux/src/loader/linux/elf_tools.o \
		build/linux/src/programs/linux/linux_runtime.o \
		build/libtinyc.a
	@echo "building readlin..."
	@$(CC) $(CFLAGS) \
		$(STANDARD_COMPILER_OPTIONS) \
		-nostdlib -static \
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
		loader env string tinyfetch dynamic unit_test string_pie winloader readwin \
		windynamic_linux readlin
