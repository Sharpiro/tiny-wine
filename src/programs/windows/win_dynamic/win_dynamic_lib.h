#include <stdint.h>

/**
 * Variables must be marked as exportable with -nostdlib.
 * 'Some' functions require it as well.
 * If you use it on one function in a file, you must use it on all.
 */
#if defined(_WIN32)
#ifdef DLL
#define EXPORTABLE __declspec(dllexport)
#else
#define EXPORTABLE __declspec(dllimport)
#endif
#else
#define EXPORTABLE
#endif

extern EXPORTABLE int32_t lib_var_bss;

extern EXPORTABLE int32_t lib_var_data;

int32_t *get_lib_var_bss();

int32_t *get_lib_var_data();
