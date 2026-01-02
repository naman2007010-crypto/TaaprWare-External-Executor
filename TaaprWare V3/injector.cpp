// TaaprWare V3 Injector
// Simple DLL injector using NtQuerySystemInformation

#include <Windows.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ntdll.lib")

// Native API types
typedef LONG NTSTATUS;
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR Buffer;
} UNICODE_STRING;

typedef struct _SYSTEM_PROCESS_INFORMATION {
  ULONG NextEntryOffset;
  ULONG NumberOfThreads;
  BYTE Reserved1[48];
  UNICODE_STRING ImageName;
  LONG BasePriority;
  HANDLE UniqueProcessId;
} SYSTEM_PROCESS_INFORMATION;

extern "C" NTSTATUS NTAPI
NtQuerySystemInformation(ULONG SystemInformationClass, PVOID SystemInformation,
                         ULONG SystemInformationLength, PULONG ReturnLength);

// Find process by name
DWORD FindProcess(const wchar_t *processName) {
  ULONG bufferSize = 1024 * 1024;
  BYTE *buffer = new BYTE[bufferSize];

  NTSTATUS status = NtQuerySystemInformation(5, buffer, bufferSize, NULL);
  if (!NT_SUCCESS(status)) {
    delete[] buffer;
    return 0;
  }

  SYSTEM_PROCESS_INFORMATION *proc = (SYSTEM_PROCESS_INFORMATION *)buffer;
  DWORD pid = 0;

  while (true) {
    if (proc->ImageName.Buffer && wcsstr(proc->ImageName.Buffer, processName)) {
      pid = (DWORD)(ULONG_PTR)proc->UniqueProcessId;
      break;
    }
    if (proc->NextEntryOffset == 0)
      break;
    proc = (SYSTEM_PROCESS_INFORMATION *)((BYTE *)proc + proc->NextEntryOffset);
  }

  delete[] buffer;
  return pid;
}

// Inject DLL
bool InjectDLL(DWORD pid, const char *dllPath) {
  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
  if (!hProcess) {
    std::cerr << "[-] Failed to open process (Error: " << GetLastError() << ")"
              << std::endl;
    return false;
  }

  size_t pathLen = strlen(dllPath) + 1;
  LPVOID remotePath = VirtualAllocEx(hProcess, NULL, pathLen,
                                     MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!remotePath) {
    CloseHandle(hProcess);
    return false;
  }

  WriteProcessMemory(hProcess, remotePath, dllPath, pathLen, NULL);

  HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
  FARPROC loadLibAddr = GetProcAddress(hKernel32, "LoadLibraryA");

  HANDLE hThread =
      CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibAddr,
                         remotePath, 0, NULL);

  if (!hThread) {
    VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return false;
  }

  WaitForSingleObject(hThread, 10000);

  DWORD exitCode = 0;
  GetExitCodeThread(hThread, &exitCode);

  CloseHandle(hThread);
  VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
  CloseHandle(hProcess);

  return exitCode != 0;
}

int main() {
  std::cout << "========================================" << std::endl;
  std::cout << "   TaaprWare V3 Injector" << std::endl;
  std::cout << "========================================" << std::endl
            << std::endl;

  char exePath[MAX_PATH];
  GetModuleFileNameA(NULL, exePath, MAX_PATH);
  std::string dllPath = exePath;
  dllPath =
      dllPath.substr(0, dllPath.find_last_of("\\/")) + "\\TaaprWareV3.dll";

  std::cout << "[*] DLL: " << dllPath << std::endl;

  if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
    std::cerr << "[-] TaaprWareV3.dll not found!" << std::endl;
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    return 1;
  }

  std::cout << "[+] DLL found!" << std::endl;
  std::cout << "[*] Waiting for Roblox..." << std::endl;

  DWORD pid = 0;
  while (pid == 0) {
    pid = FindProcess(L"RobloxPlayerBeta");
    if (pid == 0)
      pid = FindProcess(L"Windows10Universal");
    Sleep(500);
  }

  std::cout << "[+] Found Roblox (PID: " << pid << ")" << std::endl;
  std::cout << "[*] Injecting..." << std::endl;

  if (InjectDLL(pid, dllPath.c_str())) {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "   INJECTION SUCCESSFUL!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "[+] Press DELETE in-game to execute" << std::endl;
  } else {
    std::cerr << "[-] INJECTION FAILED! Try as Administrator." << std::endl;
  }

  std::cout << "\nPress Enter to exit..." << std::endl;
  std::cin.get();
  return 0;
}
