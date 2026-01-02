#pragma once

// AMD SVM (Secure Virtual Machine) Definitions
// For AMD Ryzen processors

#include <intrin.h>
#include <ntddk.h>

// CPUID Leaf for SVM support
#define CPUID_SVM_FEATURES 0x8000000A
#define CPUID_VENDOR_AMD 0x80000000

// MSR Definitions for AMD-V
#define MSR_EFER 0xC0000080
#define MSR_VM_HSAVE_PA 0xC0010117
#define MSR_VM_CR 0xC0010114

// EFER bits
#define EFER_SVME (1ULL << 12) // SVM Enable

// VM_CR bits
#define VM_CR_SVMDIS (1ULL << 4) // SVM Disabled

// VMCB (Virtual Machine Control Block) size
#define VMCB_SIZE 4096

// VMCB Control Area Offsets
#define VMCB_INTERCEPT_CR_READ 0x000
#define VMCB_INTERCEPT_CR_WRITE 0x002
#define VMCB_INTERCEPT_DR_READ 0x004
#define VMCB_INTERCEPT_DR_WRITE 0x006
#define VMCB_INTERCEPT_EXCEPTION 0x008
#define VMCB_INTERCEPT_MISC1 0x00C
#define VMCB_INTERCEPT_MISC2 0x010
#define VMCB_PAUSE_FILTER_THRESHOLD 0x03C
#define VMCB_PAUSE_FILTER_COUNT 0x03E
#define VMCB_IOPM_BASE_PA 0x040
#define VMCB_MSRPM_BASE_PA 0x048
#define VMCB_TSC_OFFSET 0x050
#define VMCB_GUEST_ASID 0x058
#define VMCB_TLB_CONTROL 0x05C
#define VMCB_VINTR 0x060
#define VMCB_INTERRUPT_SHADOW 0x068
#define VMCB_EXITCODE 0x070
#define VMCB_EXITINFO1 0x078
#define VMCB_EXITINFO2 0x080
#define VMCB_EXITINTINFO 0x088
#define VMCB_NP_ENABLE 0x090
#define VMCB_AVIC_APIC_BAR 0x098
#define VMCB_GHCB_PA 0x0A0
#define VMCB_EVENTINJ 0x0A8
#define VMCB_N_CR3 0x0B0
#define VMCB_LBR_VIRTUALIZATION 0x0B8
#define VMCB_VMCB_CLEAN 0x0C0
#define VMCB_NRIP 0x0C8

// VMCB State Save Area Offsets (starts at 0x400)
#define VMCB_SS_ES_SELECTOR 0x400
#define VMCB_SS_CS_SELECTOR 0x410
#define VMCB_SS_SS_SELECTOR 0x420
#define VMCB_SS_DS_SELECTOR 0x430
#define VMCB_SS_FS_SELECTOR 0x440
#define VMCB_SS_GS_SELECTOR 0x450
#define VMCB_SS_GDTR_SELECTOR 0x460
#define VMCB_SS_LDTR_SELECTOR 0x470
#define VMCB_SS_IDTR_SELECTOR 0x480
#define VMCB_SS_TR_SELECTOR 0x490
#define VMCB_SS_CPL 0x4CB
#define VMCB_SS_EFER 0x4D0
#define VMCB_SS_CR4 0x548
#define VMCB_SS_CR3 0x550
#define VMCB_SS_CR0 0x558
#define VMCB_SS_DR7 0x560
#define VMCB_SS_DR6 0x568
#define VMCB_SS_RFLAGS 0x570
#define VMCB_SS_RIP 0x578
#define VMCB_SS_RSP 0x5D8
#define VMCB_SS_RAX 0x5F8

// VMEXIT Codes
#define VMEXIT_CR0_READ 0x000
#define VMEXIT_CR0_WRITE 0x010
#define VMEXIT_CPUID 0x072
#define VMEXIT_MSR 0x07C
#define VMEXIT_VMRUN 0x080
#define VMEXIT_VMMCALL 0x081
#define VMEXIT_NPF 0x400 // Nested Page Fault

// Intercept bits
#define INTERCEPT_MSR (1ULL << 28)
#define INTERCEPT_CPUID (1ULL << 18)
#define INTERCEPT_VMRUN (1ULL << 0)
#define INTERCEPT_VMMCALL (1ULL << 1)

// TLB Control values
#define TLB_CONTROL_DO_NOTHING 0
#define TLB_CONTROL_FLUSH_ALL 1

// Nested Paging enable bit
#define NP_ENABLE (1ULL << 0)

// Segment descriptor structure
typedef struct _SEGMENT_DESCRIPTOR {
  UINT16 Selector;
  UINT16 Attributes;
  UINT32 Limit;
  UINT64 Base;
} SEGMENT_DESCRIPTOR, *PSEGMENT_DESCRIPTOR;

// VMCB structure as C struct
#pragma pack(push, 1)
typedef struct _VMCB_CONTROL_AREA {
  UINT16 InterceptCrRead;    // 0x000
  UINT16 InterceptCrWrite;   // 0x002
  UINT16 InterceptDrRead;    // 0x004
  UINT16 InterceptDrWrite;   // 0x006
  UINT32 InterceptException; // 0x008
  UINT32 InterceptMisc1;     // 0x00C
  UINT32 InterceptMisc2;     // 0x010
  UINT8 Reserved1[0x03C - 0x014];
  UINT16 PauseFilterThreshold; // 0x03C
  UINT16 PauseFilterCount;     // 0x03E
  UINT64 IopmBasePa;           // 0x040
  UINT64 MsrpmBasePa;          // 0x048
  UINT64 TscOffset;            // 0x050
  UINT32 GuestAsid;            // 0x058
  UINT32 TlbControl;           // 0x05C
  UINT64 VIntr;                // 0x060
  UINT64 InterruptShadow;      // 0x068
  UINT64 ExitCode;             // 0x070
  UINT64 ExitInfo1;            // 0x078
  UINT64 ExitInfo2;            // 0x080
  UINT64 ExitIntInfo;          // 0x088
  UINT64 NpEnable;             // 0x090
  UINT64 AvicApicBar;          // 0x098
  UINT64 GhcbPa;               // 0x0A0
  UINT64 EventInj;             // 0x0A8
  UINT64 NCr3;                 // 0x0B0 - Nested Page Table CR3
  UINT64 LbrVirtualization;    // 0x0B8
  UINT64 VmcbClean;            // 0x0C0
  UINT64 NRip;                 // 0x0C8 - Next RIP
  UINT8 Reserved2[0x400 - 0x0D0];
} VMCB_CONTROL_AREA, *PVMCB_CONTROL_AREA;

typedef struct _VMCB_STATE_SAVE_AREA {
  SEGMENT_DESCRIPTOR Es;   // 0x400
  SEGMENT_DESCRIPTOR Cs;   // 0x410
  SEGMENT_DESCRIPTOR Ss;   // 0x420
  SEGMENT_DESCRIPTOR Ds;   // 0x430
  SEGMENT_DESCRIPTOR Fs;   // 0x440
  SEGMENT_DESCRIPTOR Gs;   // 0x450
  SEGMENT_DESCRIPTOR Gdtr; // 0x460
  SEGMENT_DESCRIPTOR Ldtr; // 0x470
  SEGMENT_DESCRIPTOR Idtr; // 0x480
  SEGMENT_DESCRIPTOR Tr;   // 0x490
  UINT8 Reserved1[0x4CB - 0x4A0];
  UINT8 Cpl; // 0x4CB
  UINT32 Reserved2;
  UINT64 Efer; // 0x4D0
  UINT8 Reserved3[0x548 - 0x4D8];
  UINT64 Cr4;    // 0x548
  UINT64 Cr3;    // 0x550
  UINT64 Cr0;    // 0x558
  UINT64 Dr7;    // 0x560
  UINT64 Dr6;    // 0x568
  UINT64 Rflags; // 0x570
  UINT64 Rip;    // 0x578
  UINT8 Reserved4[0x5D8 - 0x580];
  UINT64 Rsp; // 0x5D8
  UINT8 Reserved5[0x5F8 - 0x5E0];
  UINT64 Rax;          // 0x5F8
  UINT64 Star;         // 0x600
  UINT64 LStar;        // 0x608
  UINT64 CStar;        // 0x610
  UINT64 SfMask;       // 0x618
  UINT64 KernelGsBase; // 0x620
  UINT64 SysenterCs;   // 0x628
  UINT64 SysenterEsp;  // 0x630
  UINT64 SysenterEip;  // 0x638
  UINT64 Cr2;          // 0x640
  UINT8 Reserved6[0x668 - 0x648];
  UINT64 GPat;         // 0x668
  UINT64 DbgCtl;       // 0x670
  UINT64 BrFrom;       // 0x678
  UINT64 BrTo;         // 0x680
  UINT64 LastExcpFrom; // 0x688
  UINT64 LastExcpTo;   // 0x690
} VMCB_STATE_SAVE_AREA, *PVMCB_STATE_SAVE_AREA;

typedef struct _VMCB {
  VMCB_CONTROL_AREA ControlArea;
  VMCB_STATE_SAVE_AREA StateSaveArea;
  UINT8 Reserved[0x1000 - sizeof(VMCB_CONTROL_AREA) -
                 sizeof(VMCB_STATE_SAVE_AREA)];
} VMCB, *PVMCB;
#pragma pack(pop)

// Per-processor hypervisor context
typedef struct _HV_CPU_CONTEXT {
  PVMCB Vmcb;              // Virtual Machine Control Block
  PHYSICAL_ADDRESS VmcbPa; // Physical address of VMCB
  PVOID HostStackTop;      // Host stack for VM exits
  PVOID MsrPermissionsMap; // MSR permission bitmap
  BOOLEAN IsVirtualized;   // Is this CPU virtualized?
  UINT64 HostRsp;          // Saved host RSP
  UINT64 HostRip;          // Return address after VMRUN

  // === LUAVM INTERCEPT FIELDS ===
  UINT64 LuavmLoadAddr; // Address of luavm_load to intercept
  UINT64 TaskDeferAddr; // Address of task_defer function
  UINT64 InterceptRip;  // RIP to intercept for patching
  UINT64 ScriptAddr;    // Address of script data in guest memory
  UINT64 LuaStateAddr;  // lua_State* for execution
} HV_CPU_CONTEXT, *PHV_CPU_CONTEXT;

// Global hypervisor state
typedef struct _HV_GLOBAL_STATE {
  PHV_CPU_CONTEXT *CpuContexts; // Array of per-CPU contexts
  UINT32 CpuCount;              // Number of processors
  BOOLEAN IsActive;             // Is hypervisor running?
} HV_GLOBAL_STATE, *PHV_GLOBAL_STATE;

// Function declarations
BOOLEAN SvmIsSupported(VOID);
NTSTATUS SvmInitialize(VOID);
VOID SvmTerminate(VOID);
VOID SvmVirtualizeCpu(PVOID Context);
VOID SvmHandleExit(PHV_CPU_CONTEXT Context);
