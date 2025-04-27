ifeq ($(CC),cc)
  CC := clang
endif

# @todo: generate header deps -MMD and -MP
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
	-Iinclude

all: \
	linux \
	windows

linux: \
	build/loader \
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
		-DLINUX \
		-masm=intel \
		$< \
		-o $@

build/libtinyc.a: build/dlls/twlibc.o build/dlls/sys_linux.o
	@echo "building libtinyc.a..."
	@ar rcs build/libtinyc.a build/dlls/twlibc.o build/dlls/sys_linux.o

build/libtinyc.so: build/dlls/twlibc.o build/dlls/sys_linux.o
	@echo "building libtinyc.so..."
	@$(CC) $(CFLAGS) \
		-O0 \
		$(STANDARD_OPTIONS) \
		-g \
		-DAMD64 \
		-nostdlib -static \
		-shared \
		-o build/libtinyc.so \
		build/dlls/twlibc.o \
		build/dlls/sys_linux.o

build/libdynamic.so:
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
		src/programs/linux/dynamic/dynamic_lib.c

build/libntdll.so: \
		src/dlls/ntdll.c \
		src/dlls/sys_linux.c
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
		src/dlls/sys_linux.c \
		src/dlls/ntdll.c

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

loader: build/loader
build/loader: \
	src/loader/memory_map.c \
	src/loader/linux/loader_main.c\
	src/loader/linux/loader_lib.c \
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

winloader:build/winloader
build/winloader: \
		src/loader/memory_map.c \
		src/loader/windows/win_loader_main.c \
		src/loader/windows/win_loader_lib.c \
		src/loader/windows/pe_tools.c \
		src/loader/linux/loader_lib.c \
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

build/unit_test: \
		src/programs/linux/unit_test/unit_test_main.c \
		src/loader/memory_map.c \
		src/loader/linux/loader_lib.c \
		src/loader/windows/win_loader_lib.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a
	@echo "building unit_test..."
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

build/env: \
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

build/string: \
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

build/tinyfetch: \
	build/tinyc/linux_runtime.o \
	build/libtinyc.so \
	build/libdynamic.so
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
		src/programs/linux/tinyfetch/tinyfetch_main.c \
		build/tinyc/linux_runtime.o

build/static_pie: \
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


build/dynamic: \
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
		tools/readwin/readwin_main.c \
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

readlin: build/readlin
build/readlin: \
		tools/readlin/readlin_main.c \
		build/tinyc/linux_runtime.o \
		build/libtinyc.a \
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

dump: all
	@files=$$(find build -type f ! -name "*.dump" -not -path '*/\.*'); \
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
