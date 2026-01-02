#pragma once

// Make sure windows.h is included first and without LEAN_AND_MEAN to ensure all
// types are defined
#include <tlhelp32.h>
#include <windows.h>


#include <iostream>
#include <string>
#include <vector>


// External Memory Reader for Roblox
class MemoryReader {
private:
  HANDLE hProcess;
  DWORD processId;
  uintptr_t baseAddress;

public:
  MemoryReader() : hProcess(NULL), processId(0), baseAddress(0) {}

  ~MemoryReader() {
    if (hProcess)
      CloseHandle(hProcess);
  }

  // Find Roblox process
  bool AttachToRoblox() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
      return false;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
      do {
        if (wcscmp(pe32.szExeFile, L"RobloxPlayerBeta.exe") == 0 ||
            wcscmp(pe32.szExeFile, L"Windows10Universal.exe") == 0) {
          processId = pe32.th32ProcessID;
          CloseHandle(hSnapshot);

          hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

          if (hProcess) {
            std::cout << "[+] Attached to Roblox (PID: " << processId << ")"
                      << std::endl;
            baseAddress = GetModuleBaseAddress();
            return true;
          }
        }
      } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return false;
  }

  // Get base address
  uintptr_t GetModuleBaseAddress() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (hSnapshot == INVALID_HANDLE_VALUE)
      return 0;

    MODULEENTRY32W me32;
    me32.dwSize = sizeof(MODULEENTRY32W);

    if (Module32FirstW(hSnapshot, &me32)) {
      CloseHandle(hSnapshot);
      std::cout << "[+] Base Address: 0x" << std::hex
                << (uintptr_t)me32.modBaseAddr << std::dec << std::endl;
      return (uintptr_t)me32.modBaseAddr;
    }

    CloseHandle(hSnapshot);
    return 0;
  }

  // Read/Write templates
  template <typename T> T Read(uintptr_t address) {
    T value{};
    ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), nullptr);
    return value;
  }

  template <typename T> bool Write(uintptr_t address, T value) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T),
                              &bytesWritten) &&
           bytesWritten == sizeof(T);
  }

  // Pattern scanning
  uintptr_t FindPattern(const char *pattern, const char *mask) {
    if (!baseAddress)
      return 0;

    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t currentAddress = baseAddress;
    uintptr_t endAddress = baseAddress + 0x10000000; // Scan 256MB

    while (currentAddress < endAddress) {
      if (VirtualQueryEx(hProcess, (LPCVOID)currentAddress, &mbi,
                         sizeof(mbi)) == 0)
        break;

      if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_GUARD) == 0 &&
          (mbi.Protect & PAGE_NOACCESS) == 0) {
        std::vector<BYTE> buffer(mbi.RegionSize);
        SIZE_T bytesRead;

        if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(),
                              mbi.RegionSize, &bytesRead)) {
          size_t maskLen = strlen(mask);
          for (size_t i = 0; i < bytesRead - maskLen; i++) {
            bool found = true;
            for (size_t j = 0; j < maskLen; j++) {
              if (mask[j] != '?' && buffer[i + j] != (BYTE)pattern[j]) {
                found = false;
                break;
              }
            }
            if (found) {
              return (uintptr_t)mbi.BaseAddress + i;
            }
          }
        }
      }
      currentAddress += mbi.RegionSize;
    }
    return 0;
  }

  HANDLE GetHandle() const { return hProcess; }
  DWORD GetProcessId() const { return processId; }
};
