// TaaprWare V3 - MessageBox Test
// Simplest possible test to verify code execution

#include <Windows.h>

extern "C" __declspec(dllexport) void TaaprMain() {
  MessageBoxA(NULL, "TaaprMain called!", "TaaprWare V3", MB_OK | MB_TOPMOST);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    MessageBoxA(NULL, "DllMain called!", "TaaprWare V3", MB_OK | MB_TOPMOST);
  }
  return TRUE;
}
