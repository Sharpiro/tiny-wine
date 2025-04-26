#include "macros.h"
#include <dlls/ntdll.h>
#include <dlls/twlibc.h>
#include <stddef.h>

#define STDOUT 1
#define STDERR 2

EXPORTABLE char *_acmdln = NULL;
static char *command_line_array[100] = {};

ssize_t write(int32_t fd, const char *data, size_t length) {
    HANDLE win_handle;
    if (fd == STDOUT) {
        win_handle = (HANDLE)-11;
    } else if (fd == STDERR) {
        win_handle = (HANDLE)-12;
    } else {
        return -1;
    }

    return NtWriteFile(
        win_handle,
        NULL,
        NULL,
        NULL,
        NULL,
        (PVOID)data,
        (ULONG)length,
        NULL,
        NULL
    );
}

static char *to_char_pointer(wchar_t *data) {
    size_t length = wcslen(data);
    char *buffer = malloc(length + 1);
    size_t i;
    for (i = 0; i < length; i++) {
        buffer[i] = (char)data[i];
    }
    buffer[i] = 0x00;
    return buffer;
}

typedef void (*_PVFV)(void);

/**
 * Initializes app with an array of functions
 */
EXPORTABLE void _initterm(_PVFV *first, _PVFV *last) {
    TEB *teb = NULL;
    __asm__("mov %0, gs:[0x30]\n" : "=r"(teb));
    UNICODE_STRING *command_line =
        &teb->ProcessEnvironmentBlock->ProcessParameters->CommandLine;
    _acmdln = to_char_pointer((wchar_t *)command_line->Buffer);

    size_t len = (size_t)(last - first);
    for (size_t i = 0; i < len; i++) {
        _PVFV func = first[i];
        if (func == NULL) {
            continue;
        }
        func();
    }
}

/**
 * @param do_wild_card If nonzero, enables wildcard expansion for
 command-line arguments (only relevant in certain CRT implementations).
 * @param start_info Reserved for internal use.
 */
EXPORTABLE int __getmainargs(
    int *pargc,
    char ***pargv,
    char ***penvp,
    [[maybe_unused]] int do_wild_card,
    [[maybe_unused]] void *start_info
) {
    char *current_cmd = _acmdln;

    int arg_count = 0;
    while (true) {
        const int MAX_ARG_SIZE =
            sizeof(command_line_array) / sizeof(*command_line_array);
        if (arg_count == MAX_ARG_SIZE) {
            return -1;
        }
        char *start = current_cmd;
        current_cmd = strchrnul(current_cmd, ' ');
        size_t str_len = (size_t)current_cmd - (size_t)start;
        char *cmd_part = malloc(str_len + 1);
        memcpy(cmd_part, start, str_len);
        cmd_part[str_len] = 0x00;
        command_line_array[arg_count] = cmd_part;
        arg_count += 1;

        if (*current_cmd == 0x00) {
            break;
        }
        current_cmd += 1;
    }

    *pargc = arg_count;
    *pargv = command_line_array;
    *penvp = NULL;
    return 0;
}

EXPORTABLE void _lock([[maybe_unused]] int32_t locknum) {
}

EXPORTABLE void _unlock([[maybe_unused]] int32_t locknum) {
}

/* Returns the code page identifier for the current locale */
EXPORTABLE int ___lc_codepage_func() {
    fprintf(stderr, "___lc_codepage_func unimplemented\n");
    exit(42);
}

/* Returns the maximum number of bytes in a multibyte character for the
 * current locale */
EXPORTABLE int ___mb_cur_max_func() {
    fprintf(stderr, "___mb_cur_max_func unimplemented\n");
    exit(42);
}

EXPORTABLE void __C_specific_handler() {
    fprintf(stderr, "__C_specific_handler unimplemented\n");
    exit(42);
}

EXPORTABLE void __initenv() {
    fprintf(stderr, "__initenv unimplemented\n");
    exit(42);
}

/**
 * Initialize locale-specific information.
 */
EXPORTABLE int __lconv_init() {
    return 0;
}

/**
 * Informs runtime if app is console or gui.
 */
EXPORTABLE void __set_app_type([[maybe_unused]] int type) {
}

EXPORTABLE void __setusermatherr() {
    fprintf(stderr, "__setusermatherr unimplemented\n");
    exit(42);
}

EXPORTABLE void _amsg_exit() {
    fprintf(stderr, "_amsg_exit unimplemented\n");
    exit(42);
}

EXPORTABLE void _cexit() {
    fprintf(stderr, "_cexit unimplemented\n");
    exit(42);
}

EXPORTABLE void _commode() {
    fprintf(stderr, "_commode unimplemented\n");
    exit(42);
}

EXPORTABLE void _fmode() {
    fprintf(stderr, "_fmode unimplemented\n");
    exit(42);
}

EXPORTABLE void _onexit([[maybe_unused]] void (*func)()) {
}

EXPORTABLE void signal() {
    fprintf(stderr, "signal unimplemented\n");
    exit(42);
}

EXPORTABLE int32_t *_errno() {
    return &errno;
}
