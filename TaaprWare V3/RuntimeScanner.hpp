#pragma once
#include <Windows.h>
#include <optional>
#include <psapi.h>
#include <string>
#include <vector>


// Runtime Offset Scanner
// Finds Roblox internal addresses at runtime by searching for string references
// This makes the DLL self-updating - no hardcoded offsets needed

namespace RuntimeScanner {

struct RobloxAddresses {
  uintptr_t taskScheduler = 0; // GetTaskScheduler function
  uintptr_t scriptContext = 0; // ScriptContext instance
  uintptr_t luaState = 0;      // lua_State pointer
  uintptr_t luavm_load = 0;    // Bytecode loader
  uintptr_t task_defer = 0;    // task.defer function
  bool valid = false;
};

inline MODULEINFO GetModuleInfo() {
  MODULEINFO info = {};
  HMODULE hModule = GetModuleHandleA(NULL);
  GetModuleInformation(GetCurrentProcess(), hModule, &info, sizeof(info));
  return info;
}

// Find a string in memory and return its address
inline uintptr_t FindString(const char *str, uintptr_t start, size_t size) {
  size_t len = strlen(str);
  uint8_t *mem = (uint8_t *)start;

  for (size_t i = 0; i < size - len; ++i) {
    if (memcmp(mem + i, str, len) == 0) {
      return start + i;
    }
  }
  return 0;
}

// Find all cross-references to a string address
inline std::vector<uintptr_t> FindXrefs(uintptr_t stringAddr, uintptr_t start,
                                        size_t size) {
  std::vector<uintptr_t> xrefs;
  uint8_t *mem = (uint8_t *)start;

  // Search for direct references (LEA patterns in x64)
  for (size_t i = 0; i < size - 8; ++i) {
    // Check for LEA with RIP-relative addressing
    // Pattern: 48 8D 0D/05/15/1D/25/2D/35/3D [4-byte offset]
    if (mem[i] == 0x48 && mem[i + 1] == 0x8D) {
      int32_t offset = *(int32_t *)(mem + i + 3);
      uintptr_t targetAddr = start + i + 7 + offset;
      if (targetAddr == stringAddr) {
        xrefs.push_back(start + i);
      }
    }
  }

  return xrefs;
}

// Find a pattern in memory
inline uintptr_t ScanPattern(const char *pattern, uintptr_t start,
                             size_t size) {
  std::vector<std::pair<uint8_t, bool>> bytes; // byte, isWildcard

  for (size_t i = 0; pattern[i]; ++i) {
    if (pattern[i] == ' ')
      continue;
    if (pattern[i] == '?') {
      bytes.push_back({0, true});
      if (pattern[i + 1] == '?')
        i++;
    } else {
      char byteStr[3] = {pattern[i], pattern[i + 1], 0};
      bytes.push_back({(uint8_t)strtol(byteStr, nullptr, 16), false});
      i++;
    }
  }

  uint8_t *mem = (uint8_t *)start;
  for (size_t i = 0; i < size - bytes.size(); ++i) {
    bool found = true;
    for (size_t j = 0; j < bytes.size(); ++j) {
      if (!bytes[j].second && mem[i + j] != bytes[j].first) {
        found = false;
        break;
      }
    }
    if (found)
      return start + i;
  }
  return 0;
}

// Main scanner function - finds all required addresses
inline RobloxAddresses ScanForAddresses() {
  RobloxAddresses addrs;

  MODULEINFO modInfo = GetModuleInfo();
  uintptr_t base = (uintptr_t)modInfo.lpBaseOfDll;
  size_t size = modInfo.SizeOfImage;

  OutputDebugStringA("[Scanner] Starting runtime offset scan...");
  char buf[256];
  sprintf_s(buf, "[Scanner] Module base: %p, size: 0x%llX", (void *)base,
            (unsigned long long)size);
  OutputDebugStringA(buf);

  // 1. Find "TaskScheduler" string
  uintptr_t taskSchedulerStr = FindString("TaskScheduler", base, size);
  if (taskSchedulerStr) {
    sprintf_s(buf, "[Scanner] Found 'TaskScheduler' string at: %p",
              (void *)taskSchedulerStr);
    OutputDebugStringA(buf);

    // Find xrefs to this string - one of them leads to GetTaskScheduler
    auto xrefs = FindXrefs(taskSchedulerStr, base, size);
    sprintf_s(buf, "[Scanner] Found %llu xrefs to TaskScheduler",
              (unsigned long long)xrefs.size());
    OutputDebugStringA(buf);

    if (!xrefs.empty()) {
      // The GetTaskScheduler function usually starts near the first xref
      // We need to trace back to find the function start
      addrs.taskScheduler = xrefs[0];
    }
  }

  // 2. Find "ScriptContext" string
  uintptr_t scriptContextStr = FindString("ScriptContext", base, size);
  if (scriptContextStr) {
    sprintf_s(buf, "[Scanner] Found 'ScriptContext' string at: %p",
              (void *)scriptContextStr);
    OutputDebugStringA(buf);
  }

  // 3. Find "WaitingHybridScriptsJob" - used to locate DataModel
  uintptr_t waitingJobStr = FindString("WaitingHybridScriptsJob", base, size);
  if (waitingJobStr) {
    sprintf_s(buf, "[Scanner] Found 'WaitingHybridScriptsJob' at: %p",
              (void *)waitingJobStr);
    OutputDebugStringA(buf);
  }

  // 4. Find patterns for key functions
  // luavm_load pattern (common prologue)
  uintptr_t luavm = ScanPattern(
      "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC", base, size);
  if (luavm) {
    addrs.luavm_load = luavm;
    sprintf_s(buf, "[Scanner] Potential luavm_load at: %p", (void *)luavm);
    OutputDebugStringA(buf);
  }

  // 5. Find "task.defer" or related scheduler functions
  uintptr_t deferStr = FindString("defer", base, size);
  if (deferStr) {
    sprintf_s(buf, "[Scanner] Found 'defer' string at: %p", (void *)deferStr);
    OutputDebugStringA(buf);
  }

  // Mark as valid if we found key addresses
  addrs.valid = (taskSchedulerStr != 0 && scriptContextStr != 0);

  if (addrs.valid) {
    OutputDebugStringA("[Scanner] Scan complete - key strings found!");
  } else {
    OutputDebugStringA("[Scanner] Scan incomplete - some strings not found");
  }

  return addrs;
}

// Resolve the actual lua_State from ScriptContext
// This is Roblox-specific and may need adjustment
inline uintptr_t GetLuaStateFromContext(uintptr_t scriptContext,
                                        int stateOffset = 0x1F8) {
  if (!scriptContext)
    return 0;

  __try {
    // Read the encrypted state pointer
    uintptr_t encryptedState = *(uintptr_t *)(scriptContext + stateOffset);
    // Roblox uses XOR encryption: state = encrypted ^ (scriptContext + offset)
    uintptr_t state = encryptedState ^ (scriptContext + stateOffset);
    return state;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return 0;
  }
}
} // namespace RuntimeScanner
