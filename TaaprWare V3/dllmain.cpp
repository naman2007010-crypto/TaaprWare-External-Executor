// TaaprWare V3 - Internal DLL Main
// This runs INSIDE Roblox process - guaranteed execution
// Uses real std::string objects and direct function calls

#include "AntiCheck.hpp"
#include "RuntimeScanner.hpp"
#include "roblox.h"
#include <Windows.h>
#include <iostream>
#include <string>
#include <thread>

// Global state
uintptr_t g_LuaState = 0;
uintptr_t g_ScriptContext = 0;
bool g_Initialized = false;

// Console for debug output
void CreateConsole() {
  AllocConsole();
  FILE *f;
  freopen_s(&f, "CONOUT$", "w", stdout);
  freopen_s(&f, "CONOUT$", "w", stderr);
  SetConsoleTitleA("TaaprWare V3 - Internal");
  std::cout << "========================================" << std::endl;
  std::cout << "   TaaprWare V3 Internal - Loaded!     " << std::endl;
  std::cout << "========================================" << std::endl;
}

// Simple pattern scan (inline implementation)
uintptr_t ScanPattern(uintptr_t base, size_t size, const char *pattern,
                      const char *mask) {
  uint8_t *mem = (uint8_t *)base;
  size_t patLen = strlen(mask);

  for (size_t i = 0; i < size - patLen; i++) {
    bool found = true;
    for (size_t j = 0; j < patLen; j++) {
      if (mask[j] == 'x' && mem[i + j] != (uint8_t)pattern[j]) {
        found = false;
        break;
      }
    }
    if (found)
      return base + i;
  }
  return 0;
}

// Find lua_State from TaskScheduler -> ScriptContext
bool FindLuaState() {
  std::cout << "[*] Scanning for TaskScheduler..." << std::endl;

  uintptr_t base = (uintptr_t)GetModuleHandleA(nullptr);
  MODULEINFO info = RuntimeScanner::GetModuleInfo();
  size_t size = info.SizeOfImage;

  // Try pattern scan for GetTaskScheduler
  uintptr_t scheduler =
      ScanPattern(base, size, "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74",
                  "xxx????xxxx");

  if (!scheduler) {
    // Alternative pattern
    scheduler =
        ScanPattern(base, size, "\x48\x83\xEC\x28\x48\x8B\x05", "xxxxxxx");
  }

  if (!scheduler) {
    std::cout << "[-] TaskScheduler not found via pattern" << std::endl;

    // Use string-based scan as fallback
    uintptr_t tsStr = RuntimeScanner::FindString("TaskScheduler", base, size);
    if (tsStr) {
      auto xrefs = RuntimeScanner::FindXrefs(tsStr, base, size);
      if (!xrefs.empty()) {
        scheduler = xrefs[0];
        std::cout << "[+] Found via string xref: 0x" << std::hex << scheduler
                  << std::dec << std::endl;
      }
    }
  }

  if (!scheduler) {
    std::cout << "[-] TaskScheduler not found" << std::endl;
    return false;
  }

  std::cout << "[+] Scheduler pattern at: 0x" << std::hex << scheduler
            << std::dec << std::endl;

  // Read ScriptContext from scheduler (try multiple offsets)
  int sc_offsets[] = {0x238, 0x250, 0x1e8, 0x220, 0x200};
  for (int offset : sc_offsets) {
    // Validate pointer before dereferencing
    if (IsBadReadPtr((void *)(scheduler + offset), sizeof(uintptr_t)))
      continue;

    uintptr_t sc = *(uintptr_t *)(scheduler + offset);
    if (sc > 0x10000 && sc < 0x7FFFFFFFFFFF) {
      if (IsBadReadPtr((void *)sc, 0x300))
        continue;

      uintptr_t state = offsets::scriptcontext::get_scriptstate(sc);
      if (state > 0x10000 && state < 0x7FFFFFFFFFFF) {
        g_ScriptContext = sc;
        g_LuaState = state;
        std::cout << "[+] ScriptContext at offset 0x" << std::hex << offset
                  << ": 0x" << sc << std::dec << std::endl;
        std::cout << "[+] lua_State: 0x" << std::hex << state << std::dec
                  << std::endl;
        return true;
      }
    }
  }

  std::cout << "[-] Could not find valid lua_State" << std::endl;
  return false;
}

// Execute a Lua script using internal functions
void ExecuteScript(const std::string &source) {
  if (!g_LuaState || !functions::luavm_load) {
    std::cout << "[-] Not initialized!" << std::endl;
    return;
  }

  // Apply anti-detection
  AntiCheck::RandomizeThread();

  // Wrap in task.spawn for safe execution
  std::string wrapped = "task.spawn(function() " + source + " end)";

  std::cout << "[*] Executing script (" << wrapped.length() << " bytes)..."
            << std::endl;

  // Call the real luavm_load - this uses REAL std::string!
  int result = functions::luavm_load(g_LuaState, &wrapped, "@TaaprExec", 0);

  if (result == 0) {
    std::cout << "[+] luavm_load SUCCESS!" << std::endl;

    // Schedule execution with task_defer
    if (functions::task_defer) {
      functions::task_defer(g_LuaState);
      std::cout << "[+] task_defer called - script scheduled!" << std::endl;
    }
  } else {
    std::cout << "[-] luavm_load returned error: " << result << std::endl;
  }
}

// Main initialization thread
void MainThread() {
  CreateConsole();

  // Wait for Roblox to fully initialize
  std::cout << "[*] Waiting for Roblox initialization..." << std::endl;
  Sleep(3000);

  // Initialize address scanning
  std::cout << "[*] Scanning for addresses..." << std::endl;
  functions::Update();

  if (!addresses::luavm_load || !addresses::task_defer) {
    std::cout << "[-] Failed to find required functions!" << std::endl;
    std::cout << "    luavm_load: 0x" << std::hex << addresses::luavm_load
              << std::endl;
    std::cout << "    task_defer: 0x" << std::hex << addresses::task_defer
              << std::dec << std::endl;
    return;
  }

  std::cout << "[+] luavm_load: 0x" << std::hex << addresses::luavm_load
            << std::dec << std::endl;
  std::cout << "[+] task_defer: 0x" << std::hex << addresses::task_defer
            << std::dec << std::endl;

  // Find lua_State
  if (!FindLuaState()) {
    std::cout << "[-] Could not find lua_State!" << std::endl;
    return;
  }

  g_Initialized = true;
  std::cout << "\n[+] TaaprWare V3 INITIALIZED!" << std::endl;
  std::cout << "[+] Press DELETE to execute test script" << std::endl;
  std::cout << "========================================\n" << std::endl;

  // Test script - properly escaped
  const char *test_script =
      "print('=================================') "
      "print('   TAAPRWARE V3 INTERNAL WORKS!') "
      "print('   luavm_load SUCCESS (err=0)') "
      "print('=================================') "
      "warn('INTERNAL DLL EXECUTION CONFIRMED!') "
      "local part = Instance.new('Part') "
      "part.Name = 'TaaprWare_Internal' "
      "part.Size = Vector3.new(30, 30, 30) "
      "part.Position = Vector3.new(0, 150, 0) "
      "part.BrickColor = BrickColor.new('Bright blue') "
      "part.Anchored = true "
      "part.Material = Enum.Material.Neon "
      "part.Parent = game.Workspace "
      "game:GetService('StarterGui'):SetCore('SendNotification', {Title = "
      "'TaaprWare V3', Text = 'INTERNAL DLL WORKING!', Duration = 15})";

  // Hotkey loop
  while (true) {
    if (GetAsyncKeyState(VK_DELETE) & 1) {
      ExecuteScript(test_script);
    }
    Sleep(100);
  }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);
    std::thread(MainThread).detach();
  }
  return TRUE;
}
