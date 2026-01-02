// TaaprWare V3 - LoadLibrary-style Injection with ReflectiveLoader
// Uses a small shellcode that calls LoadLibraryA on the injected DLL path

#include <Windows.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "ntdll.lib")

typedef LONG NTSTATUS;
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef NTSTATUS(NTAPI *fn_NtAllocateVirtualMemory)(HANDLE, PVOID *, ULONG_PTR,
                                                    PSIZE_T, ULONG, ULONG);
typedef NTSTATUS(NTAPI *fn_NtWriteVirtualMemory)(HANDLE, PVOID, PVOID, SIZE_T,
                                                 PSIZE_T);
typedef NTSTATUS(NTAPI *fn_NtCreateThreadEx)(PHANDLE, ACCESS_MASK, PVOID,
                                             HANDLE, PVOID, PVOID, ULONG,
                                             SIZE_T, SIZE_T, SIZE_T, PVOID);

template <typename T> T GetNt(const char *name) {
  return (T)GetProcAddress(GetModuleHandleA("ntdll.dll"), name);
}

int main() {
  printf("\n  [ TaaprWare V3 - Path Injection ]\n\n");

  // Get full DLL path
  char dllPath[MAX_PATH];
  GetFullPathNameA("TaaprWareV3.dll", MAX_PATH, dllPath, nullptr);
  printf("[+] DLL: %s\n", dllPath);

  // Check if DLL exists
  if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES) {
    printf("[!] DLL not found!\n");
    system("pause");
    return 1;
  }

  printf("PID: ");
  DWORD pid;
  std::cin >> pid;

  HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
  if (!hProc) {
    printf("[!] OpenProcess failed: %lu\n", GetLastError());
    system("pause");
    return 1;
  }
  printf("[+] Process opened\n");

  auto NtAlloc = GetNt<fn_NtAllocateVirtualMemory>("NtAllocateVirtualMemory");
  auto NtWrite = GetNt<fn_NtWriteVirtualMemory>("NtWriteVirtualMemory");
  auto NtCreateThreadEx = GetNt<fn_NtCreateThreadEx>("NtCreateThreadEx");

  // Allocate memory for DLL path in target
  SIZE_T pathLen = strlen(dllPath) + 1;
  PVOID remotePath = nullptr;
  if (!NT_SUCCESS(NtAlloc(hProc, &remotePath, 0, &pathLen,
                          MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
    printf("[!] Path alloc failed\n");
    return 1;
  }
  printf("[+] Path buffer: %p\n", remotePath);

  // Write path
  SIZE_T written;
  NtWrite(hProc, remotePath, dllPath, strlen(dllPath) + 1, &written);

  // Get LoadLibraryA address (same in all processes due to ASLR for system
  // DLLs)
  PVOID pLoadLibrary =
      GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
  printf("[+] LoadLibraryA: %p\n", pLoadLibrary);

  // Create remote thread to call LoadLibraryA(path)
  HANDLE hThread = nullptr;
  NTSTATUS status =
      NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS, nullptr, hProc,
                       pLoadLibrary, remotePath, FALSE, 0, 0, 0, nullptr);

  if (NT_SUCCESS(status) && hThread) {
    printf("[+] Thread created, waiting...\n");
    WaitForSingleObject(hThread, 10000);

    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    printf("[+] LoadLibrary returned: 0x%X\n", exitCode);

    if (exitCode == 0) {
      printf("[!] LoadLibrary FAILED - Hyperion may have blocked it\n");
    } else {
      printf("[+] DLL loaded at: 0x%X\n", exitCode);
    }

    CloseHandle(hThread);
  } else {
    printf("[!] Thread creation failed: 0x%lX\n", status);
  }

  CloseHandle(hProc);
  printf("[*] Done\n");
  system("pause");
  return 0;
}
