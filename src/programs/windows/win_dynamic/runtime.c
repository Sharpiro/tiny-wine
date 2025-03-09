#include "../../../dlls/msvcrt.h"
// #include "../../../dlls/ntdll.h"

// @todo: relocations probably

// void init_runtime_relocations();

/**
 * Called on stdlib load, unload, etc.
 */
bool DllMain(
    [[maybe_unused]] void *hinstDLL,
    [[maybe_unused]] unsigned long fdwReason,
    [[maybe_unused]] void *lpReserved
) {
    return true;
}

void _init_relocations_win() {
    fprintf(stderr, "_init_relocations_win\n");
    _init_relocations_loader();
}

// void init_relocations(void) {
//     fprintf(stderr, "init_relocations\n");
// }

// /**
//  * Required by MinGW when relocations are necessary.
//  * MinGW expects the startup runtime to call this.
//  */
// void _pei386_runtime_relocator(void) {
//     fprintf(stderr, "_pei386_runtime_relocator\n");
//     // init_runtime_relocations();
//     exit(1);
// }

// extern int main() __attribute__((weak));

// /*
//  * Inserted into and called in the `main` function.
//  * Replaces setting global 'initialized' symbol during stdlib TLS setup.
//  */
// void __main(void) {
// }

// void mainCRTStartup() {
//     _pei386_runtime_relocator();
//     int result = main();
//     exit(result);
// }
