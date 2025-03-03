#include "../../../dlls/export.h"
#include <stdint.h>

// extern __declspec(dllexport) int32_t lib_var_bss;
extern EXPORTABLE int32_t lib_var_bss;
extern __declspec(dllexport) int32_t lib_var_data;
// extern EXPORTABLE int32_t lib_var_data;

int32_t EXPORTABLE *get_lib_var_bss();

int32_t EXPORTABLE *get_lib_var_data();
