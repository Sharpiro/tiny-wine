#pragma once

#include "../../../dlls/export.h"
#include <stdint.h>

// @todo: doesn't need 'EXPORTABLE' when running directly in Windows

EXPORTABLE extern int32_t lib_var_bss;
EXPORTABLE extern int32_t lib_var_data;

EXPORTABLE int32_t *get_lib_var_bss();

EXPORTABLE int32_t *get_lib_var_data();
