#pragma once
// TaaprWare V3 - Stealthy Injector
// Uses NtMapViewOfSection for memory injection
// Uses thread hijacking for code execution
// Designed to evade Hyperion anti-cheat

#include "Shellcode.h"
#include "Syscalls.h"
#include <iostream>
#include <tlhelp32.h>
#include <vector>
#include <windows.h>

namespace StealthInjector {

// Find a suitable thread in the target process to hijack
inline DWORD FindTargetThread(DWORD processId) {
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snapshot == INVALID_HANDLE_VALUE)
    return 0;

  THREADENTRY32 te;
  te.dwSize = sizeof(te);

  DWORD targetThread = 0;

  if (Thread32First(snapshot, &te)) {
    // Loop through threads
    int threadCount = 0;
    do {
      if (te.th32OwnerProcessID == processId) {
        threadCount++;
        // Skip the first thread (Main Thread) to avoid UI/Render deadlocks
        // Hijacking the main thread often causes "Application Not Responding"
        // or immediate crashes
        if (threadCount > 1) {
          CloseHandle(snapshot);
          return te.th32ThreadID;
        }
      }
    } while (Thread32Next(snapshot, &te));
  }

  CloseHandle(snapshot);
  return targetThread;
}

// Inject code using section mapping (stealthier than VirtualAllocEx)
inline bool InjectViaSection(HANDLE processHandle, const void *code,
                             size_t codeSize, void **remoteAddress) {
  if (!Syscalls::pNtCreateSection || !Syscalls::pNtMapViewOfSection) {
    std::cerr << "[-] Syscalls not initialized" << std::endl;
    return false;
  }

  // Create a section object
  HANDLE sectionHandle = NULL;
  LARGE_INTEGER sectionSize;
  sectionSize.QuadPart = codeSize;

  NTSTATUS status = Syscalls::pNtCreateSection(
      &sectionHandle, SECTION_ALL_ACCESS, NULL, &sectionSize,
      PAGE_EXECUTE_READWRITE, SEC_COMMIT, NULL);

  if (status != STATUS_SUCCESS) {
    std::cerr << "[-] NtCreateSection failed: 0x" << std::hex << status
              << std::dec << std::endl;
    return false;
  }

  // Map section into our process
  PVOID localAddress = NULL;
  SIZE_T viewSize = 0;

  status =
      Syscalls::pNtMapViewOfSection(sectionHandle, GetCurrentProcess(),
                                    &localAddress, 0, codeSize, NULL, &viewSize,
                                    2, // ViewUnmap
                                    0, PAGE_READWRITE);

  if (status != STATUS_SUCCESS) {
    std::cerr << "[-] NtMapViewOfSection (local) failed: 0x" << std::hex
              << status << std::dec << std::endl;
    CloseHandle(sectionHandle);
    return false;
  }

  // Write code to local view
  memcpy(localAddress, code, codeSize);

  // Map same section into target process
  *remoteAddress = NULL;
  viewSize = 0;

  status = Syscalls::pNtMapViewOfSection(
      sectionHandle, processHandle, remoteAddress, 0, codeSize, NULL, &viewSize,
      2, // ViewUnmap
      0, PAGE_EXECUTE_READ);

  if (status != STATUS_SUCCESS) {
    std::cerr << "[-] NtMapViewOfSection (remote) failed: 0x" << std::hex
              << status << std::dec << std::endl;
    Syscalls::pNtUnmapViewOfSection(GetCurrentProcess(), localAddress);
    CloseHandle(sectionHandle);
    return false;
  }

  // Unmap local view (remote still mapped)
  Syscalls::pNtUnmapViewOfSection(GetCurrentProcess(), localAddress);
  CloseHandle(sectionHandle);

  std::cout << "[+] Code injected via section at: 0x" << std::hex
            << *remoteAddress << std::dec << std::endl;
  return true;
}

// Execute injected code via thread hijacking
// Execute injected code via thread hijacking
inline bool ExecuteViaThreadHijack(HANDLE processHandle, DWORD threadId,
                                   void *codeAddress, void *contextAddress,
                                   uintptr_t *outOldRip = nullptr) {
  if (!Syscalls::pNtSuspendThread || !Syscalls::pNtResumeThread ||
      !Syscalls::pNtGetContextThread || !Syscalls::pNtSetContextThread) {
    std::cerr << "[-] Thread syscalls not initialized" << std::endl;
    return false;
  }

  // Open the target thread
  HANDLE threadHandle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                                       THREAD_SET_CONTEXT,
                                   FALSE, threadId);

  if (!threadHandle) {
    std::cerr << "[-] OpenThread failed: " << GetLastError() << std::endl;
    return false;
  }

  // Suspend the thread
  ULONG suspendCount = 0;
  NTSTATUS status = Syscalls::pNtSuspendThread(threadHandle, &suspendCount);
  if (status != STATUS_SUCCESS) {
    std::cerr << "[-] NtSuspendThread failed: 0x" << std::hex << status
              << std::dec << std::endl;
    CloseHandle(threadHandle);
    return false;
  }

  // Get thread context
  CONTEXT ctx;
  ctx.ContextFlags = CONTEXT_FULL;

  status = Syscalls::pNtGetContextThread(threadHandle, &ctx);
  if (status != STATUS_SUCCESS) {
    std::cerr << "[-] NtGetContextThread failed: 0x" << std::hex << status
              << std::dec << std::endl;
    Syscalls::pNtResumeThread(threadHandle, &suspendCount);
    CloseHandle(threadHandle);
    return false;
  }

  // Save original RIP
  uintptr_t originalRip = ctx.Rip;
  if (outOldRip)
    *outOldRip = originalRip;
  std::cout << "[*] Original RIP: 0x" << std::hex << originalRip << std::dec
            << std::endl;

  // Set RIP to our shellcode
  ctx.Rip = (DWORD64)codeAddress;

  // Set RCX to context pointer (first parameter in x64 calling convention)
  ctx.Rcx = (DWORD64)contextAddress;

  // Set thread context
  status = Syscalls::pNtSetContextThread(threadHandle, &ctx);
  if (status != STATUS_SUCCESS) {
    std::cerr << "[-] NtSetContextThread failed: 0x" << std::hex << status
              << std::dec << std::endl;
    // Restore and resume
    ctx.Rip = originalRip;
    Syscalls::pNtSetContextThread(threadHandle, &ctx);
    Syscalls::pNtResumeThread(threadHandle, &suspendCount);
    CloseHandle(threadHandle);
    return false;
  }

  std::cout << "[+] Thread RIP modified to: 0x" << std::hex << codeAddress
            << std::dec << std::endl;

  // Resume thread (shellcode will execute)
  status = Syscalls::pNtResumeThread(threadHandle, &suspendCount);
  if (status != STATUS_SUCCESS) {
    std::cerr << "[-] NtResumeThread failed: 0x" << std::hex << status
              << std::dec << std::endl;
    CloseHandle(threadHandle);
    return false;
  }

  std::cout << "[+] Thread resumed, shellcode executing..." << std::endl;

  // Wait a moment for execution
  Sleep(100);

  // TODO: Restore original RIP after shellcode runs
  // This requires the shellcode to signal completion

  CloseHandle(threadHandle);
  return true;
}

// Main injection function
inline bool InjectAndExecute(HANDLE processHandle, DWORD processId,
                             uintptr_t luaState, uintptr_t luavmLoad,
                             uintptr_t taskDefer, const std::string &script) {
  std::cout << "[*] Initializing stealthy injection..." << std::endl;

  // Initialize syscalls
  if (!Syscalls::InitializeSyscalls()) {
    std::cerr << "[-] Failed to initialize syscalls" << std::endl;
    return false;
  }
  std::cout << "[+] Syscalls initialized" << std::endl;

  // Find a thread to hijack
  DWORD threadId = FindTargetThread(processId);
  if (!threadId) {
    std::cerr << "[-] No suitable thread found" << std::endl;
    return false;
  }
  std::cout << "[+] Target thread: " << threadId << std::endl;

  // Calculate total injection size
  size_t totalSize = Shellcode::ShellcodeSize +
                     sizeof(Shellcode::ShellcodeContext) +
                     Shellcode::ChunkNameSize + script.size() + 1;

  // 1. Create Section
  HANDLE sectionHandle = NULL;
  LARGE_INTEGER sectionSize;
  sectionSize.QuadPart = totalSize;

  NTSTATUS status = Syscalls::pNtCreateSection(
      &sectionHandle, SECTION_ALL_ACCESS, NULL, &sectionSize,
      PAGE_EXECUTE_READWRITE, SEC_COMMIT, NULL);

  if (status != STATUS_SUCCESS) {
    std::cerr << "[-] NtCreateSection failed: 0x" << std::hex << status
              << std::dec << std::endl;
    return false;
  }

  // 2. Map Local View (Read/Write)
  PVOID localBase = NULL;
  SIZE_T viewSize = 0;
  status = Syscalls::pNtMapViewOfSection(
      sectionHandle, GetCurrentProcess(), &localBase, 0, totalSize, NULL,
      &viewSize, 2 /* ViewUnmap */, 0, PAGE_READWRITE);

  if (status != STATUS_SUCCESS) {
    std::cerr << "[-] NtMapViewOfSection (local) failed: 0x" << std::hex
              << status << std::dec << std::endl;
    CloseHandle(sectionHandle);
    return false;
  }

  // 3. Map Remote View (Execute/Read)
  PVOID remoteBase = NULL;
  viewSize = 0;
  status = Syscalls::pNtMapViewOfSection(
      sectionHandle, processHandle, &remoteBase, 0, totalSize, NULL, &viewSize,
      2 /* ViewUnmap */, 0, PAGE_EXECUTE_READ);

  if (status != STATUS_SUCCESS) {
    std::cerr << "[-] NtMapViewOfSection (remote) failed: 0x" << std::hex
              << status << std::dec << std::endl;
    Syscalls::pNtUnmapViewOfSection(GetCurrentProcess(), localBase);
    CloseHandle(sectionHandle);
    return false;
  }

  // Calculate offsets aligned to 8 bytes
  size_t offset = 0;
  size_t shellcodeOffset = offset;
  offset += Shellcode::ShellcodeSize;

  offset = (offset + 7) & ~7;
  size_t contextOffset = offset;
  offset += sizeof(Shellcode::ShellcodeContext);

  offset = (offset + 7) & ~7;
  size_t chunkNameOffset = offset;
  offset += Shellcode::ChunkNameSize;

  offset = (offset + 7) & ~7;
  size_t scriptOffset = offset;

  // Calculate Remote Addresses based on remoteBase
  uintptr_t remoteShellcode = (uintptr_t)remoteBase + shellcodeOffset;
  uintptr_t remoteContext = (uintptr_t)remoteBase + contextOffset;
  uintptr_t remoteChunkName = (uintptr_t)remoteBase + chunkNameOffset;
  uintptr_t remoteScript = (uintptr_t)remoteBase + scriptOffset;

  // 4. Write Data to Local View (which is shared with Remote)
  uint8_t *bytePtr = (uint8_t *)localBase;

  // Copy Shellcode
  memcpy(bytePtr + shellcodeOffset, Shellcode::ShellcodeBytes,
         Shellcode::ShellcodeSize);

  // Generate random chunk name for evasion
  std::string randomChunkName = "@" + []() {
    const char charset[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string s;
    for (int i = 0; i < 10; ++i)
      s += charset[rand() % (sizeof(charset) - 1)];
    return s;
  }();

  // Copy Random ChunkName (including null terminator)
  memcpy(bytePtr + chunkNameOffset, randomChunkName.c_str(),
         randomChunkName.size() + 1);

  // Copy Script
  memcpy(bytePtr + scriptOffset, script.c_str(), script.size() + 1);

  // Create Context
  Shellcode::ShellcodeContext ctx;
  ctx.luaState = luaState;
  ctx.luavmLoadAddr = luavmLoad;
  ctx.bytecodeAddr = remoteScript;
  ctx.bytecodeSize = script.size();
  ctx.originalRip = 0; // Filled by hijack
  ctx.chunkName = remoteChunkName;
  ctx.taskDeferAddr = taskDefer;

  // Copy Context
  memcpy(bytePtr + contextOffset, &ctx, sizeof(ctx));

  // Execute via thread hijack
  std::cout << "[*] Executing via thread hijack..." << std::endl;

  // Pointer to originalRip storage in LOCAL view (shared with remote)
  uintptr_t *localOldRipPtr =
      (uintptr_t *)((uint8_t *)localBase + contextOffset +
                    0x20); // 0x20 is offsetof(originalRip)

  if (!ExecuteViaThreadHijack(processHandle, threadId, (void *)remoteShellcode,
                              (void *)remoteContext, localOldRipPtr)) {
    std::cerr << "[-] Thread hijack failed" << std::endl;
    // Cleanup
    Syscalls::pNtUnmapViewOfSection(GetCurrentProcess(), localBase);
    CloseHandle(sectionHandle);
    return false;
  }

  // 5. Unmap Local View (Unmap AFTER modifying context via thread hijack
  // return)
  Syscalls::pNtUnmapViewOfSection(GetCurrentProcess(), localBase);
  CloseHandle(sectionHandle);

  std::cout << "[+] Script execution initiated!" << std::endl;
  return true;
}

} // namespace StealthInjector
