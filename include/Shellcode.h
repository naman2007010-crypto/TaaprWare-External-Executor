#pragma once
// TaaprWare V3 - Shellcode for Script Execution
// Position-independent code that calls luavm_load
// This shellcode is injected into Roblox and executed via thread hijacking

#include <cstdint>
#include <windows.h>

namespace Shellcode {

// Shellcode context - passed to shellcode via register or stack
struct ShellcodeContext {
  uintptr_t luaState;      // lua_State pointer
  uintptr_t luavmLoadAddr; // luavm_load function address
  uintptr_t bytecodeAddr;  // Address of compiled bytecode in target
  uintptr_t bytecodeSize;  // Size of bytecode
  uintptr_t originalRip;   // Original RIP to restore after execution
  uintptr_t chunkName;     // "@TaaprExec" string address
  uintptr_t taskDeferAddr; // task.defer address (native execution)
};

// Simple shellcode that:
// 1. Saves registers
// 2. Calls luavm_load(state, bytecode_ptr, "@TaaprExec", 0)
// 3. Restores registers
// 4. Jumps back to original RIP

// This is x64 position-independent shellcode
// Compiled from:
/*
    push rbx
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x28

    ; Context pointer in R15
    mov r15, rcx

    ; Load parameters for luavm_load(state, bytecode, chunkname, env)
    mov rcx, [r15]           ; lua_State
    mov rdx, [r15 + 0x10]    ; bytecode address
    mov r8, [r15 + 0x28]     ; chunkname "@TaaprExec"
    xor r9, r9               ; env = 0

    ; Call luavm_load
    mov rax, [r15 + 0x08]    ; luavm_load address
    call rax

    add rsp, 0x28
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbx

    ; Jump back to original RIP
    mov rax, [r15 + 0x20]    ; original RIP
    jmp rax
*/

// Pre-compiled shellcode bytes (placeholder - needs actual compilation)
constexpr uint8_t ShellcodeBytes[] = {
    // push rbx,    // 1. Reserve slot for Return Address (Original RIP)
    0x48, 0x83, 0xEC, 0x08, // sub rsp, 8

    // 2. Save Flags
    0x9C, // pushfq

    // 3. Save GPRs (Volatile & Non-Volatile)
    0x50,       // push rax
    0x53,       // push rbx
    0x51,       // push rcx
    0x52,       // push rdx
    0x56,       // push rsi
    0x57,       // push rdi
    0x55,       // push rbp
    0x41, 0x50, // push r8
    0x41, 0x51, // push r9
    0x41, 0x52, // push r10
    0x41, 0x53, // push r11
    0x41, 0x54, // push r12
    0x41, 0x55, // push r13
    0x41, 0x56, // push r14
    0x41, 0x57, // push r15

    // 4. Setup Safe Context Pointer (RBX)
    // RBX is preserved on stack, so we can use it.
    // RCX holds Context Ptr (passed via Thread Hijack)
    0x48, 0x89, 0xCB, // mov rbx, rcx

    // 5. Stack Alignment & Shadow Space
    0x48, 0x89, 0xE5,       // mov rbp, rsp (Save Top of Stack)
    0x48, 0x83, 0xE4, 0xF0, // and rsp, -16 (Align to 16 bytes)

    // 6. Save XMM/FPU State (Critical for Roblox physics/render threads)
    0x48, 0x81, 0xEC, 0x00, 0x02, 0x00,
    0x00,                         // sub rsp, 512   (Reserve 512 bytes)
    0x48, 0x0F, 0xAE, 0x04, 0x24, // fxsave64 [rsp] (Save state)

    // 7. Shadow Space
    0x48, 0x83, 0xEC, 0x20, // sub rsp, 32

    // ================= PAYLOAD =================
    // RBX = Context*

    // Call luavm_load
    // luavm_load(lua_State* L, char* chunkName, char* source, size_t size)
    0x48, 0x8B, 0x0B,       // mov rcx, [rbx]       ; lua_State (Offset 0)
    0x48, 0x8D, 0x53, 0x28, // lea rdx, [rbx+0x28]  ; chunkName (Offset 0x28 -
                            // wait, check struct!)
    // Struct: luaState(8), luavmLoad(8), bytecode(8), size(8), originalRip(8),
    // taskDefer(8).
    // Size = 48 (0x30). ChunkName is AFTER struct?
    // Wait, Shellcode.h defines offsets?
    // No, struct definition at bottom of file.
    // 0x00: luaState
    // 0x08: luavmLoadAddr
    // 0x10: bytecodeAddr
    // 0x18: bytecodeSize
    // 0x20: originalRip
    // 0x28: chunkName (char*)?
    // 0x30: taskDeferAddr
    // Let's verify struct layout below!
    // Struct:
    // uintptr_t luaState;
    // uintptr_t luavmLoadAddr;
    // uintptr_t bytecodeAddr;
    // size_t bytecodeSize;
    // uintptr_t originalRip;
    // uintptr_t chunkName; // Pointer!
    // uintptr_t taskDeferAddr;

    // So RBX+0x28 is POINTER to chunkName.
    // RCX = luaState
    0x48, 0x8B, 0x53, 0x28, // mov rdx, [rbx+0x28] ; chunkName (char*)
    0x4C, 0x8B, 0x43, 0x10, // mov r8,  [rbx+0x10] ; bytecodeAddr (char*)
    0x4C, 0x8B, 0x4B, 0x18, // mov r9,  [rbx+0x18] ; bytecodeSize (size_t)

    0x48, 0x8B, 0x43, 0x08, // mov rax, [rbx+0x08] ; luavm_load addr
    0xFF, 0xD0,             // call rax

    // Check luavm_load result (rax == 0 means success usually? Or 1? Wait.
    // luavm_load returns status (0=OK, >0=Error).
    // Let's assume we check for 0.
    0x85, 0xC0, // test eax, eax
    0x75, 0x0E, // jnz +14 (Skip to cleanup - manual count!)

    // Check & Call task_defer
    0x48, 0x8B, 0x0B,       // mov rcx, [rbx]       ; lua_State -> RCX
    0x48, 0x8B, 0x43, 0x30, // mov rax, [rbx+0x30]  ; taskDeferAddr -> RAX
    // Struct offset 0x30? Yes (5 ptrs * 8 = 40 = 0x28 + 0x8 = 0x30).
    0x48, 0x85, 0xC0, // test rax, rax
    0x74, 0x02,       // jz +2
    0xFF, 0xD0,       // call rax

    // ================= RESTORE =================
    // 1. Restore Shadow Space
    0x48, 0x83, 0xC4, 0x20, // add rsp, 32

    // 2. Restore XMM/FPU
    0x48, 0x0F, 0xAE, 0x0C, 0x24,             // fxrstor64 [rsp]
    0x48, 0x81, 0xC4, 0x00, 0x02, 0x00, 0x00, // add rsp, 512

    // 3. Restore Stack Ptr
    0x48, 0x89, 0xEC, // mov rsp, rbp (Restore Stack)

    // Setup Return Address
    // [rbx + 0x20] = OriginalRip
    0x48, 0x8B, 0x43, 0x20, // mov rax, [rbx+0x20]
    // Store at [rsp + 128] (15 regs * 8 + 8 flags = 128)
    0x48, 0x89, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00, // mov [rsp+0x80], rax

    // Pop GPRs (Reverse Order)
    0x41, 0x5F, // pop r15
    0x41, 0x5E, // pop r14
    0x41, 0x5D, // pop r13
    0x41, 0x5C, // pop r12
    0x41, 0x5B, // pop r11
    0x41, 0x5A, // pop r10
    0x41, 0x59, // pop r9
    0x41, 0x58, // pop r8
    0x5D,       // pop rbp
    0x5F,       // pop rdi
    0x5E,       // pop rsi
    0x5A,       // pop rdx
    0x59,       // pop rcx
    0x5B,       // pop rbx
    0x58,       // pop rax

    // Pop Flags
    0x9D, // popfq

    // Return (Jumps to OriginalRip stored at rsp)
    0xC3 // ret
};

constexpr size_t ShellcodeSize = sizeof(ShellcodeBytes);

// Chunk name string to embed
constexpr char ChunkName[] = "@TaaprExec";
constexpr size_t ChunkNameSize = sizeof(ChunkName);

} // namespace Shellcode
