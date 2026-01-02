#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <shared_mutex>
#include "Scanner.hpp"

// Mock 2026 offsets (Placeholder values for x64)
// In a real scenario, these would be found via Scans or updated weekly
namespace offsets {
    namespace scriptcontext {
        // x64 alignment usually pushes this further out
        constexpr int state_offset = 0x200; 

        inline uintptr_t get_scriptstate(uintptr_t scriptcontext) {
            // Decryption logic varies. 2026 Hyperion likely uses pointer obfuscation.
            // Simplified: return *(uintptr_t*)(scriptcontext + state_offset) ^ (scriptcontext + state_offset);
            if (!scriptcontext) return 0;
            return *reinterpret_cast<uintptr_t*>(scriptcontext + state_offset);
        }
    }
    namespace state {
        constexpr int top = 0x18;      // L->top (x64)
        constexpr int base = 0x20;     // L->base
        constexpr int global = 0x30;   // L->l_G
    }
}

namespace addresses {
    inline uintptr_t task_defer = 0;
    inline uintptr_t luavm_load = 0;
    inline uintptr_t get_scheduler = 0;

    // Pattern strings (Mock)
    // "E8 ? ? ? ? 48 8B D8" -> call task_defer
    const std::string pat_task_defer = "E8 ? ? ? ? 48 8B D8"; 
    const std::string pat_luavm_load = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30"; // sub rsp, 30 ...

    inline bool Initialize() {
        auto res_defer = Scanner::Scan(pat_task_defer);
        if (res_defer) {
            // Resolve relative call
            task_defer = Scanner::ResolveRip(res_defer.value(), 1, 5); 
        } else {
            // Fallback for demo
            task_defer = 0xCAFEBABE; 
        }

        auto res_load = Scanner::Scan(pat_luavm_load);
        if (res_load) luavm_load = res_load.value();
        
        return (task_defer != 0 && luavm_load != 0);
    }
}

namespace functions {
    // Typedefs for x64 calling convention (__fastcall is default in x64)
    typedef int(__fastcall* t_luavm_load)(uintptr_t state, std::string* compressed_bytecode, const char* chunkname, int env);
    typedef int(__fastcall* t_task_defer)(uintptr_t state);
    
    inline t_luavm_load luavm_load = nullptr;
    inline t_task_defer task_defer = nullptr;

    inline void Update() {
        if (addresses::Initialize()) {
            luavm_load = reinterpret_cast<t_luavm_load>(addresses::luavm_load);
            task_defer = reinterpret_cast<t_task_defer>(addresses::task_defer);
        }
    }
}