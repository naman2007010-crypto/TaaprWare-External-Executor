// TaaprWare External Executor V3
// Secure Boot compatible - Stealthy Hyperion Bypass
// Uses direct syscalls, section mapping, and thread hijacking

#include "StealthInjector.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

// ==========================================
// PSAPI Dynamic Loading
// ==========================================
typedef BOOL(WINAPI *EnumProcessModules_t)(HANDLE, HMODULE *, DWORD, LPDWORD);
typedef DWORD(WINAPI *GetModuleBaseNameA_t)(HANDLE, HMODULE, LPSTR, DWORD);
typedef BOOL(WINAPI *EnumProcesses_t)(DWORD *, DWORD, DWORD *);

HMODULE hPsapi = NULL;
EnumProcessModules_t pEnumProcessModules = NULL;
GetModuleBaseNameA_t pGetModuleBaseNameA = NULL;
EnumProcesses_t pEnumProcesses = NULL;

bool LoadPsapi() {
  hPsapi = LoadLibraryA("psapi.dll");
  if (!hPsapi)
    return false;
  pEnumProcessModules =
      (EnumProcessModules_t)GetProcAddress(hPsapi, "EnumProcessModules");
  pGetModuleBaseNameA =
      (GetModuleBaseNameA_t)GetProcAddress(hPsapi, "GetModuleBaseNameA");
  pEnumProcesses = (EnumProcesses_t)GetProcAddress(hPsapi, "EnumProcesses");
  return (pEnumProcessModules && pGetModuleBaseNameA && pEnumProcesses);
}

// ==========================================
// MemoryReader Class
// ==========================================
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
    if (hPsapi)
      FreeLibrary(hPsapi);
  }

  bool AttachToRoblox() {
    if (!LoadPsapi()) {
      std::cerr << "[-] Failed to load PSAPI.dll" << std::endl;
      return false;
    }

    DWORD processes[1024];
    DWORD cbNeeded;
    if (!pEnumProcesses(processes, sizeof(processes), &cbNeeded))
      return false;

    DWORD count = cbNeeded / sizeof(DWORD);
    for (DWORD i = 0; i < count; i++) {
      if (processes[i] != 0) {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ |
                                       PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
                                   FALSE, processes[i]);
        if (hProc) {
          HMODULE hMod;
          DWORD cbNeededMod;
          char szProcessName[MAX_PATH] = "<unknown>";

          if (pEnumProcessModules(hProc, &hMod, sizeof(hMod), &cbNeededMod)) {
            pGetModuleBaseNameA(hProc, hMod, szProcessName,
                                sizeof(szProcessName));
          }

          std::string name(szProcessName);
          if (name.find("RobloxPlayerBeta.exe") != std::string::npos ||
              name.find("Windows10Universal.exe") != std::string::npos) {
            processId = processes[i];
            hProcess = hProc;
            baseAddress = (uintptr_t)hMod;
            std::cout << "[+] Attached to " << name << " (PID: " << processId
                      << ")" << std::endl;
            std::cout << "[+] Base Address: 0x" << std::hex << baseAddress
                      << std::dec << std::endl;
            return true;
          }
          if (hProcess != hProc)
            CloseHandle(hProc);
        }
      }
    }
    return false;
  }

  template <typename T> T Read(uintptr_t address) {
    T value{};
    ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), nullptr);
    return value;
  }

  std::string ReadString(uintptr_t address, size_t maxLen = 256) {
    char buffer[256] = {0};
    SIZE_T bytesRead;
    if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer,
                          min(maxLen, sizeof(buffer)), &bytesRead)) {
      buffer[min(bytesRead, sizeof(buffer) - 1)] = '\0';
      return std::string(buffer);
    }
    return "";
  }

  uintptr_t FindPattern(const char *pattern, const char *mask) {
    if (!baseAddress)
      return 0;

    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t currentAddress = baseAddress;
    uintptr_t endAddress = baseAddress + 0x10000000;

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

  // Search for pattern and return ALL occurrences
  std::vector<uintptr_t> FindPatternAll(const char *pattern, const char *mask) {
    std::vector<uintptr_t> results;
    if (!baseAddress)
      return results;

    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t currentAddress = baseAddress;
    uintptr_t endAddress = baseAddress + 0x8000000; // Search decent range

    while (currentAddress < endAddress && results.size() < 100) {
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
              results.push_back((uintptr_t)mbi.BaseAddress + i);
            }
          }
        }
      }
      currentAddress += mbi.RegionSize;
    }
    return results;
  }

  // Search for a string in memory and return ALL occurrences
  std::vector<uintptr_t> FindStringInMemory(const char *str) {
    std::vector<uintptr_t> results;
    size_t strLen = strlen(str);
    if (!baseAddress || strLen == 0)
      return results;

    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t currentAddress = 0x10000; // Start from low memory
    uintptr_t endAddress = 0x7FFFFFFFFFFF;

    while (currentAddress < endAddress && results.size() < 100) {
      if (VirtualQueryEx(hProcess, (LPCVOID)currentAddress, &mbi,
                         sizeof(mbi)) == 0)
        break;

      if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_GUARD) == 0 &&
          (mbi.Protect & PAGE_NOACCESS) == 0) {
        std::vector<BYTE> buffer(mbi.RegionSize);
        SIZE_T bytesRead;

        if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(),
                              mbi.RegionSize, &bytesRead)) {
          for (size_t i = 0; i < bytesRead - strLen; i++) {
            if (memcmp(buffer.data() + i, str, strLen) == 0) {
              results.push_back((uintptr_t)mbi.BaseAddress + i);
            }
          }
        }
      }
      currentAddress = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    return results;
  }

  HANDLE GetHandle() const { return hProcess; }
  DWORD GetProcessId() const { return processId; }
  uintptr_t GetBaseAddress() const { return baseAddress; }
};

// ==========================================
// Roblox Structure Offsets (from TaaprWare V3)
// ==========================================
namespace RobloxOffsets {
// ScriptContext -> lua_State (with XOR decryption)
// state = encrypted ^ (scriptContext + stateOffset)
constexpr size_t ScriptContextState = 0x1F8;     // From RuntimeScanner.hpp
constexpr size_t ScriptContextState_Alt = 0x200; // From roblox.h

// lua_State structure
constexpr size_t StateTop = 0x18;
constexpr size_t StateBase = 0x20;
constexpr size_t StateGlobal = 0x30;

// Job name offsets to try
constexpr size_t JobNameOffsets[] = {0x10, 0x18, 0x78, 0x80, 0x88, 0x90, 0x98};
constexpr size_t NumNameOffsets =
    sizeof(JobNameOffsets) / sizeof(JobNameOffsets[0]);

// Decrypt lua_State pointer (Roblox XOR encryption)
inline uintptr_t DecryptState(uintptr_t encrypted, uintptr_t contextAddr,
                              size_t offset) {
  return encrypted ^ (contextAddr + offset);
}
} // namespace RobloxOffsets

// ==========================================
// ScriptInjector Class (Full Implementation)
// ==========================================
class ScriptInjector {
private:
  MemoryReader *memory;
  uintptr_t schedulerAddress;
  uintptr_t scriptContextAddress;
  uintptr_t luaStateAddress;
  uintptr_t luavmLoadAddress; // Bytecode loader function
  uintptr_t taskDeferAddress;

public:
  ScriptInjector(MemoryReader *mem)
      : memory(mem), schedulerAddress(0), scriptContextAddress(0),
        luaStateAddress(0), luavmLoadAddress(0), taskDeferAddress(0) {}

  uintptr_t ResolveRelativePtr(uintptr_t address, int offset_offset,
                               int instr_size) {
    if (!address)
      return 0;
    int32_t relative = memory->Read<int32_t>(address + offset_offset);
    return address + instr_size + relative;
  }

  bool FindTaskScheduler() {
    std::cout << "[*] Auto-Scanning for TaskScheduler..." << std::endl;

    struct Pattern {
      const char *name;
      const char *bytes;
      const char *mask;
      int offset_to_ptr;
      int instr_size;
    } patterns[] = {
        {"Standard", "\x48\x8B\x05\x00\x00\x00\x00\x48\x83\xC4\x48\xC3",
         "xxx????xxxxx", 3, 7},
        {"GlobalPtr", "\x48\x8B\x05\x00\x00\x00\x00\x48\x83\xC4\x28\xC3",
         "xxx????xxxxx", 3, 7},
        {"AltStart",
         "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20",
         "xxxxxxxxxxxxxxx", -1, 0}};

    for (const auto &p : patterns) {
      std::cout << "[*] Trying pattern: " << p.name << "..." << std::endl;
      uintptr_t result = memory->FindPattern(p.bytes, p.mask);

      if (result) {
        std::cout << "[+] Found Signature (" << p.name << ") at: 0x" << std::hex
                  << result << std::dec << std::endl;

        uintptr_t candidate = 0;
        if (p.offset_to_ptr != -1) {
          candidate = ResolveRelativePtr(result, p.offset_to_ptr, p.instr_size);
        } else {
          // AltStart logic (simulated by reading near instructions or just
          // using hardcoded offset logic from previous logs? No, previous logs
          // ResolveRelativePtr logic was implicit) Check original code for
          // AltStart specific logic. Original code: if (p.name == "AltStart")
          // schedulerAddress = ResolveRelativePtr(result + 0x20, 3, 7);
          // Replicating that:
          candidate = ResolveRelativePtr(result + 0x20, 3, 7);
        }

        if (candidate) {
          schedulerAddress = candidate;
          std::cout << "[+] Candidate Scheduler: 0x" << std::hex << candidate
                    << std::dec << std::endl;

          // Validate via EnumerateJobs (assuming ScanOffsets refers to this)
          // Note: EnumerateJobs prints its own output
          if (EnumerateJobs(true)) {
            std::cout << "[+] TaskScheduler validated!" << std::endl;
            return true;
          } else {
            std::cout << "[-] Candidate Invalid (No jobs found)" << std::endl;
            schedulerAddress = 0;
          }
        }
      }
    }

    std::cout << "[-] Could not find TaskScheduler via patterns" << std::endl;
    return false;
  }

  bool EnumerateJobs(bool validationOnly = false) {
    if (!schedulerAddress)
      return false;

    std::cout << "[*] Enumerating Jobs (scanning offsets)..." << std::endl;

    // Scan a range of offsets for the jobs vector
    for (size_t offset = 0x100; offset <= 0x300; offset += 8) {
      uintptr_t jobsStart = memory->Read<uintptr_t>(schedulerAddress + offset);
      uintptr_t jobsEnd =
          memory->Read<uintptr_t>(schedulerAddress + offset + 8);

      // Validate pointers
      if (jobsStart < 0x10000 || jobsEnd < 0x10000)
        continue;
      if (jobsEnd <= jobsStart)
        continue;

      size_t jobCount = (jobsEnd - jobsStart) / sizeof(uintptr_t);

      // Sanity check: job count should be reasonable (5-500)
      if (jobCount < 5 || jobCount > 500)
        continue;

      std::cout << "[*] Testing offset 0x" << std::hex << offset << std::dec
                << " (" << jobCount << " potential jobs)" << std::endl;

      int validJobs = 0;
      for (size_t i = 0; i < min(jobCount, (size_t)50); i++) {
        uintptr_t sharedPtrAddr =
            memory->Read<uintptr_t>(jobsStart + i * sizeof(uintptr_t));
        if (!sharedPtrAddr || sharedPtrAddr < 0x10000)
          continue;

        // shared_ptr stores pointer to object at offset 0
        uintptr_t jobPtr = memory->Read<uintptr_t>(sharedPtrAddr);
        if (!jobPtr || jobPtr < 0x10000)
          continue;

        // Try to read job name from various offsets
        std::string jobName;
        bool nameValid = false;

        for (size_t n = 0; n < RobloxOffsets::NumNameOffsets && !nameValid;
             n++) {
          size_t nameOff = RobloxOffsets::JobNameOffsets[n];

          // Try reading as inline string (SSO)
          jobName = memory->ReadString(jobPtr + nameOff, 64);
          nameValid =
              !jobName.empty() && jobName[0] >= 'A' && jobName[0] <= 'z';

          // Try reading as pointer to string
          if (!nameValid) {
            uintptr_t namePtr = memory->Read<uintptr_t>(jobPtr + nameOff);
            if (namePtr > 0x10000 && namePtr < 0x7FFFFFFFFFFF) {
              jobName = memory->ReadString(namePtr, 64);
              nameValid =
                  !jobName.empty() && jobName[0] >= 'A' && jobName[0] <= 'z';
            }
          }
        }

        if (nameValid) {
          validJobs++;
          std::cout << "  [" << i << "] " << jobName << std::endl;

          // Check for script-related jobs
          if (jobName.find("Script") != std::string::npos ||
              jobName.find("Waiting") != std::string::npos ||
              jobName.find("Lua") != std::string::npos ||
              jobName.find("Render") != std::string::npos) {
            // ... (existing logic)
          }
        }

        // ================= BRUTE FORCE FALLBACK =================
        // If name check failed or we want to be thorough, scan the job struct
        // anyway
        for (size_t ctxOff = 0x50; ctxOff <= 0x500; ctxOff += 8) {
          uintptr_t ctx = memory->Read<uintptr_t>(jobPtr + ctxOff);
          if (ctx < 0x10000 || ctx > 0x7FFFFFFFFFFF)
            continue;

          // Scan for lua_State in this candidate context
          for (size_t luaOff = 0x50; luaOff <= 0x500; luaOff += 8) {
            uintptr_t lua = memory->Read<uintptr_t>(ctx + luaOff);
            // Need XOR check? Try direct first, then XOR
            if (lua < 0x10000 || lua > 0x7FFFFFFFFFFF)
              continue;

            // Validate lua_State
            // Check L->top and L->base
            // Struct: ... global_State* (0x10 or 0x18), top (0x10/18?), base
            // (0x20?) 64-bit Lua 5.1/Luau: 0x10: GCObject* environment 0x18:
            // void* top 0x20: void* base
            uintptr_t top =
                memory->Read<uintptr_t>(lua + 0x18); // Potential top
            uintptr_t base =
                memory->Read<uintptr_t>(lua + 0x20); // Potential base

            if (top > 0x10000 && base > 0x10000 && top >= base &&
                (top - base) < 0x100000) {
              // Found a plausible stack!
              // This is extremely likely to be lua_State
              scriptContextAddress = ctx;
              luaStateAddress = lua;
              std::cout << "[+] Found plausible lua_State via Brute Force!"
                        << std::endl;
              std::cout << "    Job Offset: 0x" << std::hex << offset
                        << std::endl;
              std::cout << "    Job Index: " << i << std::endl;
              std::cout << "    Context Offset: 0x" << ctxOff << std::endl;
              std::cout << "    State Offset: 0x" << luaOff << std::endl;
              std::cout << "    lua_State: 0x" << lua << std::dec << std::endl;
              return true;
            }

            // Try XORed state? (Usually stored in ScriptContext, not lua_State
            // itself) ScriptContext usually stores encrypted L.
            uintptr_t encryptedL =
                memory->Read<uintptr_t>(ctx + luaOff); // Reuse
            uintptr_t decryptedL = encryptedL ^ (ctx + luaOff);
            if (decryptedL > 0x10000 && decryptedL < 0x7FFFFFFFFFFF) {
              uintptr_t dTop = memory->Read<uintptr_t>(decryptedL + 0x18);
              uintptr_t dBase = memory->Read<uintptr_t>(decryptedL + 0x20);
              if (dTop > dBase && (dTop - dBase) < 0x100000) {
                scriptContextAddress = ctx;
                luaStateAddress = decryptedL;
                std::cout
                    << "[+] Found plausible lua_State (XOR) via Brute Force!"
                    << std::endl;
                std::cout << "    lua_State: 0x" << std::hex << decryptedL
                          << std::dec << std::endl;
                return true;
              }
            }
          }
        }
      }

      if (validJobs > 3) {
        std::cout << "[+] Found " << validJobs << " valid jobs at offset 0x"
                  << std::hex << offset << std::dec << std::endl;
        if (validationOnly)
          return true;
      }
    }

    std::cout << "[-] Could not find lua_State via Jobs" << std::endl;
    return false;
  }

  bool FindLuaStateViaStringSearch() {
    std::cout << "[*] Trying ScriptContext scan with XOR decryption..."
              << std::endl;

    // Scan scheduler for ScriptContext pointer
    if (schedulerAddress) {
      std::cout << "[*] Scanning scheduler for ScriptContext pointer..."
                << std::endl;

      for (size_t off = 0; off <= 0x2000; off += 8) {
        uintptr_t maybeCtx = memory->Read<uintptr_t>(schedulerAddress + off);

        if (maybeCtx > 0x10000 && maybeCtx < 0x7FFFFFFFFFFF) {
          // Try to decrypt lua_State using Brute Force offsets
          // Instead of hardcoded offsets, scan the structure
          for (size_t stateOff = 0; stateOff < 0x500; stateOff += 8) {
            uintptr_t encrypted = memory->Read<uintptr_t>(maybeCtx + stateOff);

            if (encrypted > 0x1000) {
              // Apply XOR decryption
              uintptr_t decrypted =
                  RobloxOffsets::DecryptState(encrypted, maybeCtx, stateOff);

              if (decrypted > 0x10000 && decrypted < 0x7FFFFFFFFFFF) {
                uintptr_t top = memory->Read<uintptr_t>(
                    decrypted + RobloxOffsets::StateTop);
                uintptr_t base = memory->Read<uintptr_t>(
                    decrypted + RobloxOffsets::StateBase);

                if (top > 0x10000 && base > 0x10000 && top < 0x7FFFFFFFFFFF &&
                    base < 0x7FFFFFFFFFFF) {
                  scriptContextAddress = maybeCtx;
                  luaStateAddress = decrypted;
                  std::cout << "[+] Found ScriptContext at scheduler offset 0x"
                            << std::hex << off << std::dec << std::endl;
                  std::cout << "[+] ScriptContext: 0x" << std::hex << maybeCtx
                            << std::dec << std::endl;
                  std::cout << "[+] lua_State (XOR decrypted): 0x" << std::hex
                            << decrypted << std::dec << std::endl;
                  return true;
                }
              }
            }
          }
        }
      }
    }

    std::cout << "[-] Could not find lua_State via scheduler scan" << std::endl;
    return false;
  }

  bool FindLuavmLoad() {
    std::cout << "[*] Scanning for luavm_load (bytecode loader)..."
              << std::endl;

    // Pattern from TaaprWare V3 roblox.h
    // "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC"
    const char *pattern = "\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74"
                          "\x24\x00\x57\x48\x83\xEC";
    const char *mask = "xxxx?xxxx?xxxx?xxxx";

    uintptr_t result = memory->FindPattern(pattern, mask);

    if (result) {
      luavmLoadAddress = result;
      std::cout << "[+] Found luavm_load at: 0x" << std::hex << result
                << std::dec << std::endl;
      return true;
    }

    // Try alternative pattern (function prologue)
    const char *altPattern = "\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x55";
    const char *altMask = "xxxx?xxxxxxx";

    result = memory->FindPattern(altPattern, altMask);
    if (result) {
      luavmLoadAddress = result;
      std::cout << "[+] Found luavm_load (alt) at: 0x" << std::hex << result
                << std::dec << std::endl;
      return true;
    }

    std::cout << "[-] Could not find luavm_load" << std::endl;
    return false;
  }

  bool FindLuaState() {
    if (!FindTaskScheduler())
      return false;

    // Try job enumeration first
    if (EnumerateJobs()) {
      FindLuavmLoad(); // Also try to find luavm_load
      return true;
    }

    // Fallback: XOR decryption search
    if (FindLuaStateViaStringSearch()) {
      FindLuavmLoad();
      return true;
    }

    return false;
  }

  bool FindTaskDefer() {
    std::cout << "[*] Scanning for task_defer..." << std::endl;
    // Pattern: E8 ? ? ? ? 48 8B D8
    const char *pattern = "\xE8\x00\x00\x00\x00\x48\x8B\xD8";
    const char *mask = "x????xxx";

    std::vector<uintptr_t> results = memory->FindPatternAll(pattern, mask);
    for (uintptr_t addr : results) {
      // Resolve relative call
      int32_t rel = memory->Read<int32_t>(addr + 1);
      uintptr_t target = addr + 5 + rel;

      // Sanity checks
      if (target > 0x100000 && target < 0x7FFFFFFFFFFF) {
        taskDeferAddress = target;
        std::cout << "[+] Found task_defer at: 0x" << std::hex
                  << taskDeferAddress << std::dec << std::endl;
        return true;
      }
    }

    std::cout << "[-] Could not find task_defer" << std::endl;
    return false;
  }

  bool ExecuteScript(const std::string &script) {
    if (!luaStateAddress) {
      std::cout << "[-] lua_State not found" << std::endl;
      return false;
    }

    if (!luavmLoadAddress) {
      std::cout << "[-] luavm_load not found, cannot execute" << std::endl;
      return false;
    }

    if (!taskDeferAddress) {
      FindTaskDefer();
      if (!taskDeferAddress) {
        std::cout << "[-] task_defer not found, relying on luavm_load only "
                     "(might not execute)"
                  << std::endl;
        // We can return false, but let's try injecting anyway just in case the
        // user wants to test Just pass 0 as taskDeferAddress
      }
    }

    std::cout << "[*] Attempting stealth injection..." << std::endl;

    if (StealthInjector::InjectAndExecute(
            memory->GetHandle(), memory->GetProcessId(), luaStateAddress,
            luavmLoadAddress, taskDeferAddress, script)) {
      std::cout << "[+] Execution sequence initiated!" << std::endl;
      return true;
    }

    std::cerr << "[-] Injection failed" << std::endl;
    return false;
  }

  uintptr_t GetLuaState() const { return luaStateAddress; }
  uintptr_t GetScriptContext() const { return scriptContextAddress; }
  uintptr_t GetLuavmLoad() const { return luavmLoadAddress; }
};

// ==========================================
// Main
// ==========================================
void PrintBanner() {
  std::cout << R"(
╔════════════════════════════════════════╗
║     TaaprWare V3 - External Mode      ║
║   Boosting Nation Hub Auto-Executor   ║
╚════════════════════════════════════════╝
)" << std::endl;
}

std::string LoadScript(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open())
    return "";
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

int main() {
  PrintBanner();

  std::cout << "[*] Searching for Roblox..." << std::endl;
  MemoryReader memory;

  if (!memory.AttachToRoblox()) {
    std::cerr << "[-] Failed to find Roblox process!" << std::endl;
    std::cerr << "[!] Make sure Roblox is running" << std::endl;
    system("pause");
    return 1;
  }

  std::cout << "[*] Initializing script injector..." << std::endl;
  ScriptInjector injector(&memory);

  if (injector.FindLuaState()) {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  SUCCESS! Found Luau VM Components   " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  lua_State:     0x" << std::hex << injector.GetLuaState()
              << std::dec << std::endl;
    std::cout << "  ScriptContext: 0x" << std::hex
              << injector.GetScriptContext() << std::dec << std::endl;
    if (injector.GetLuavmLoad()) {
      std::cout << "  luavm_load:    0x" << std::hex << injector.GetLuavmLoad()
                << std::dec << std::endl;
    }
    std::cout << "========================================" << std::endl;
  } else {
    std::cerr << "[!] Could not fully resolve lua_State" << std::endl;
    std::cerr << "[!] Offsets may need adjustment for this Roblox build"
              << std::endl;
  }

  std::cout << "[*] Loading script..." << std::endl;
  std::string script = LoadScript("BoostingNationHub.lua");
  if (script.empty()) {
    std::cout << "[*] Using embedded script..." << std::endl;
    script = "print('TaaprWare Test')";
  }

  std::cout << "[*] Attempting execution..." << std::endl;
  injector.ExecuteScript(script);

  std::cout << "\n[*] Press any key to exit..." << std::endl;
  system("pause");
  return 0;
}
