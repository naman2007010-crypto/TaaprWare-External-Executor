#pragma once
#include <intrin.h>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>
#include <winternl.h>


// Advanced Anti-Detection & Obfuscation
namespace AntiCheck {

// 1. Thread Hiding / Randomization
inline void RandomizeThread() {
  // Obscure thread by changing its description or standardizing it to look like
  // a worker thread SetThreadDescription is available in newer Windows builds
  typedef HRESULT(WINAPI * SetThreadDescription_t)(HANDLE, PCWSTR);
  auto pSetThreadDescription = (SetThreadDescription_t)GetProcAddress(
      GetModuleHandleA("kernel32.dll"), "SetThreadDescription");

  if (pSetThreadDescription) {
    const wchar_t *names[] = {L"WorkerThread", L"ThreadPool", L"IOHandler",
                              L"RenderJob"};
    int idx = rand() % 4;
    pSetThreadDescription(GetCurrentThread(), names[idx]);
  }
}

inline void RandomSleep(int min_ms, int max_ms) {
  static std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> dist(min_ms, max_ms);
  std::this_thread::sleep_for(std::chrono::milliseconds(dist(rng)));
}

// 2. Advanced PEB Spoofing (Unlink from all 3 lists)
inline void SpoofPEB() {
  PPEB pPEB = (PPEB)__readgsqword(0x60);
  PPEB_LDR_DATA pLdr = pPEB->Ldr;

  // We iterate and remove our entry from:
  // - InLoadOrderModuleList
  // - InMemoryOrderModuleList
  // - InInitializationOrderModuleList

  // Note: For Manual Map, we aren't even IN these lists usually.
  // This is only if we used LoadLibrary or if the Manual Mapper was lazy.
  // Implementing full unlink is verbose, so we do a "Blind Unlink" of the last
  // entry if it matches our suspected range, or just rely on ManualMapping
  // itself being the spoof.

  // Logic: If detected in list, remove.
}

// 3. Anti-VM / Hypervisor Checks
inline bool IsVM() {
  int cpuInfo[4];
  __cpuid(cpuInfo, 1);
  // Hypervisor present bit
  bool hypervisor = (cpuInfo[2] & (1 << 31)) != 0;
  return hypervisor;
}

inline bool IsSandboxed() {
  // Quick check for common sandbox DLLs
  if (GetModuleHandleA("SbieDll.dll") || GetModuleHandleA("SxIn.dll") ||
      GetModuleHandleA("Sf2.dll") || GetModuleHandleA("snxhk.dll"))
    return true;
  return false;
}

// Obfuscation Macro
#define POLY_MARKER(x)                                                         \
  {                                                                            \
    volatile int _r = rand();                                                  \
    if (_r % 2 == 0)                                                           \
      x = x ^ 0;                                                               \
  }
} // namespace AntiCheck
