#pragma once

#include <stddef.h>

/**
 * By default variable exports will require relocations.
 * You can explicitly set export/import to avoid this.
 * If you use exportable on one function in a file, you must use it on all.
 * 'export' marks the symbol as exported in the dll.
 * 'import' tells the executable to reference it in its IAT.
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
