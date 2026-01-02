#pragma once
// TaaprWare V3 - Direct Syscall Stubs
// Bypasses ntdll.dll hooks by calling syscalls directly
// Windows 10/11 22H2 syscall numbers

#include <windows.h>

namespace Syscalls {

// Syscall numbers for Windows 10/11 22H2
// These may need updating for different Windows versions
namespace Numbers {
constexpr DWORD NtAllocateVirtualMemory = 0x18;
constexpr DWORD NtWriteVirtualMemory = 0x3A;
constexpr DWORD NtProtectVirtualMemory = 0x50;
constexpr DWORD NtCreateSection = 0x4A;
constexpr DWORD NtMapViewOfSection = 0x28;
constexpr DWORD NtUnmapViewOfSection = 0x2A;
constexpr DWORD NtOpenProcess = 0x26;
constexpr DWORD NtClose = 0x0F;
constexpr DWORD NtSuspendThread = 0x1C0;
constexpr DWORD NtResumeThread = 0x52;
constexpr DWORD NtGetContextThread = 0xF3;
constexpr DWORD NtSetContextThread = 0x189;
constexpr DWORD NtQuerySystemInformation = 0x36;
constexpr DWORD NtQueryInformationProcess = 0x19;
constexpr DWORD NtOpenThread = 0x110;
} // namespace Numbers

// Status codes
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

// Type definitions
typedef LONG NTSTATUS;
typedef ULONG ACCESS_MASK;

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;
  PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _CLIENT_ID {
  HANDLE UniqueProcess;
  HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

// Initialize OBJECT_ATTRIBUTES macro
#define InitializeObjectAttributes(p, n, a, r, s)                              \
  {                                                                            \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES);                                   \
    (p)->RootDirectory = r;                                                    \
    (p)->Attributes = a;                                                       \
    (p)->ObjectName = n;                                                       \
    (p)->SecurityDescriptor = s;                                               \
    (p)->SecurityQualityOfService = NULL;                                      \
  }

// Direct syscall stub (x64)
// This uses the syscall instruction directly to bypass ntdll hooks
extern "C" NTSTATUS DoSyscall(DWORD syscallNumber, ...);

// Inline assembly for syscall (MSVC x64)
// Note: MSVC doesn't support inline asm in x64, so we use a separate .asm file
// For now, we'll use a workaround with function pointers

// Get syscall address from ntdll (for fallback)
inline PVOID GetNtdllFunction(const char *name) {
  HMODULE ntdll = GetModuleHandleA("ntdll.dll");
  if (!ntdll)
    return nullptr;
  return GetProcAddress(ntdll, name);
}

// Type definitions for NT functions
typedef NTSTATUS(NTAPI *fn_NtAllocateVirtualMemory)(
    HANDLE ProcessHandle, PVOID *BaseAddress, ULONG_PTR ZeroBits,
    PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect);

typedef NTSTATUS(NTAPI *fn_NtWriteVirtualMemory)(HANDLE ProcessHandle,
                                                 PVOID BaseAddress,
                                                 PVOID Buffer,
                                                 SIZE_T NumberOfBytesToWrite,
                                                 PSIZE_T NumberOfBytesWritten);

typedef NTSTATUS(NTAPI *fn_NtProtectVirtualMemory)(HANDLE ProcessHandle,
                                                   PVOID *BaseAddress,
                                                   PSIZE_T RegionSize,
                                                   ULONG NewProtect,
                                                   PULONG OldProtect);

typedef NTSTATUS(NTAPI *fn_NtCreateSection)(
    PHANDLE SectionHandle, ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize,
    ULONG SectionPageProtection, ULONG AllocationAttributes, HANDLE FileHandle);

typedef NTSTATUS(NTAPI *fn_NtMapViewOfSection)(
    HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress,
    ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset,
    PSIZE_T ViewSize, DWORD InheritDisposition, ULONG AllocationType,
    ULONG Win32Protect);

typedef NTSTATUS(NTAPI *fn_NtUnmapViewOfSection)(HANDLE ProcessHandle,
                                                 PVOID BaseAddress);

typedef NTSTATUS(NTAPI *fn_NtSuspendThread)(HANDLE ThreadHandle,
                                            PULONG PreviousSuspendCount);

typedef NTSTATUS(NTAPI *fn_NtResumeThread)(HANDLE ThreadHandle,
                                           PULONG SuspendCount);

typedef NTSTATUS(NTAPI *fn_NtGetContextThread)(HANDLE ThreadHandle,
                                               PCONTEXT ThreadContext);

typedef NTSTATUS(NTAPI *fn_NtSetContextThread)(HANDLE ThreadHandle,
                                               PCONTEXT ThreadContext);

typedef NTSTATUS(NTAPI *fn_NtOpenThread)(PHANDLE ThreadHandle,
                                         ACCESS_MASK DesiredAccess,
                                         POBJECT_ATTRIBUTES ObjectAttributes,
                                         PCLIENT_ID ClientId);

// Global function pointers (loaded at runtime)
static inline fn_NtAllocateVirtualMemory pNtAllocateVirtualMemory = nullptr;
static inline fn_NtWriteVirtualMemory pNtWriteVirtualMemory = nullptr;
static inline fn_NtProtectVirtualMemory pNtProtectVirtualMemory = nullptr;
static inline fn_NtCreateSection pNtCreateSection = nullptr;
static inline fn_NtMapViewOfSection pNtMapViewOfSection = nullptr;
static inline fn_NtUnmapViewOfSection pNtUnmapViewOfSection = nullptr;
static inline fn_NtSuspendThread pNtSuspendThread = nullptr;
static inline fn_NtResumeThread pNtResumeThread = nullptr;
static inline fn_NtGetContextThread pNtGetContextThread = nullptr;
static inline fn_NtSetContextThread pNtSetContextThread = nullptr;
static inline fn_NtOpenThread pNtOpenThread = nullptr;

// Initialize all syscall functions
inline bool InitializeSyscalls() {
  pNtAllocateVirtualMemory =
      (fn_NtAllocateVirtualMemory)GetNtdllFunction("NtAllocateVirtualMemory");
  pNtWriteVirtualMemory =
      (fn_NtWriteVirtualMemory)GetNtdllFunction("NtWriteVirtualMemory");
  pNtProtectVirtualMemory =
      (fn_NtProtectVirtualMemory)GetNtdllFunction("NtProtectVirtualMemory");
  pNtCreateSection = (fn_NtCreateSection)GetNtdllFunction("NtCreateSection");
  pNtMapViewOfSection =
      (fn_NtMapViewOfSection)GetNtdllFunction("NtMapViewOfSection");
  pNtUnmapViewOfSection =
      (fn_NtUnmapViewOfSection)GetNtdllFunction("NtUnmapViewOfSection");
  pNtSuspendThread = (fn_NtSuspendThread)GetNtdllFunction("NtSuspendThread");
  pNtResumeThread = (fn_NtResumeThread)GetNtdllFunction("NtResumeThread");
  pNtGetContextThread =
      (fn_NtGetContextThread)GetNtdllFunction("NtGetContextThread");
  pNtSetContextThread =
      (fn_NtSetContextThread)GetNtdllFunction("NtSetContextThread");
  pNtOpenThread = (fn_NtOpenThread)GetNtdllFunction("NtOpenThread");

  return (pNtAllocateVirtualMemory && pNtWriteVirtualMemory &&
          pNtCreateSection && pNtMapViewOfSection && pNtSuspendThread &&
          pNtResumeThread && pNtGetContextThread && pNtSetContextThread);
}

} // namespace Syscalls
