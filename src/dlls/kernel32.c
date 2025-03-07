#include "ntdll.h"
#include <stddef.h>

void DllMainCRTStartup(void) {
}

void DeleteCriticalSection() {
}

void EnterCriticalSection() {
}

void GetLastError() {
}

void GetStartupInfoA() {
}

void InitializeCriticalSection() {
}

void LeaveCriticalSection() {
}

void SetUnhandledExceptionFilter() {
}

void Sleep() {
}

void TlsGetValue() {
}

void VirtualProtect() {
    NtTerminateProcess((HANDLE)-1, 2);
}

void VirtualQuery() {
}

void IsDBCSLeadByteEx() {
}

void MultiByteToWideChar() {
}

void WideCharToMultiByte() {
}
