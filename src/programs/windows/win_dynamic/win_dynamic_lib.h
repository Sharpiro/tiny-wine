#include <stdint.h>

/**
 * Variables must be marked as exportable with -nostdlib
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

extern EXPORTABLE int32_t lib_var;

int lib_add(int x, int y);
