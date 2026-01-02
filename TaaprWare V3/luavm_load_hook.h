#pragma once
#include "roblox.h"

// External ASM symbols
extern "C" void bytecode_hook_asm();
extern "C" uintptr_t luavm_load_return_addr;

// C++ Handler called by ASM
extern "C" void bytecode_hook_handler();

bool init_hooks();

// New Export
extern "C" void ExecuteScript(const char *source);