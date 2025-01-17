#include "loader_lib.h"
#include "pe_tools.h"
#include <stddef.h>

bool get_runtime_import_address_table(
    const struct ImportAddressEntry *import_address_table,
    size_t import_address_table_len,
    const WinRuntimeObjectList *shared_libraries,
    RuntimeImportAddressEntryList *runtime_import_table,
    size_t image_base,
    size_t runtime_iat_base
);

bool map_import_address_table(
    int32_t fd,
    size_t runtime_iat_base,
    size_t idata_base,
    const RuntimeImportAddressEntryList *import_address_table,
    size_t dynamic_callback_windows,
    size_t *iat_runtime_base
);
