#include <stdint.h>

#if defined(_WIN32)
#define IMPORTABLE __declspec(dllimport)
#else
#define IMPORTABLE
#endif

extern IMPORTABLE int32_t lib_var_bss;

extern IMPORTABLE int32_t lib_var_data;

int32_t *get_lib_var_bss();

int32_t *get_lib_var_data();
