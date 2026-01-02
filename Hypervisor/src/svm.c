// TaaprWare Hypervisor - AMD SVM Implementation
// Educational hypervisor for AMD Ryzen processors

#include "../include/svm.h"

// Global hypervisor state
static HV_GLOBAL_STATE g_HvState = {0};

// Check if AMD SVM is supported
BOOLEAN SvmIsSupported(VOID) {
  int cpuInfo[4];

  // Check for AMD CPU
  __cpuid(cpuInfo, 0);
  if (cpuInfo[1] != 'htuA' || cpuInfo[3] != 'itne' || cpuInfo[2] != 'DMAc') {
    return FALSE; // Not AMD
  }

  // Check SVM support (CPUID Fn8000_0001_ECX[SVM])
  __cpuid(cpuInfo, 0x80000001);
  if (!(cpuInfo[2] & (1 << 2))) {
    return FALSE; // SVM not supported
  }

  // Check if SVM is disabled in BIOS (VM_CR.SVMDIS)
  UINT64 vmCr = __readmsr(MSR_VM_CR);
  if (vmCr & VM_CR_SVMDIS) {
    return FALSE; // SVM disabled in BIOS
  }

  return TRUE;
}

// Get number of ASID (Address Space ID) supported
UINT32 SvmGetMaxAsid(VOID) {
  int cpuInfo[4];
  __cpuid(cpuInfo, CPUID_SVM_FEATURES);
  return cpuInfo[1]; // EBX = Number of ASIDs
}

// Allocate and initialize the MSR permission map
PVOID SvmAllocateMsrPermissionMap(VOID) {
  // MSR permission map is 8KB (2 bits per MSR, covering 0x0000-0x1FFF and
  // 0xC000_0000-0xC000_1FFF)
  PHYSICAL_ADDRESS maxAddr;
  maxAddr.QuadPart = MAXUINT64;

  PVOID msrpm = MmAllocateContiguousMemory(8192, maxAddr);
  if (msrpm) {
    RtlZeroMemory(msrpm, 8192);
    // By default, allow all MSR access (don't intercept)
    // We'll selectively enable interception for specific MSRs
  }
  return msrpm;
}

// Initialize VMCB with current CPU state
VOID SvmSetupVmcb(PVMCB Vmcb, PHV_CPU_CONTEXT Context) {
  RtlZeroMemory(Vmcb, sizeof(VMCB));

  // === Control Area Setup ===

  // Intercept CPUID to hide hypervisor presence
  Vmcb->ControlArea.InterceptMisc1 = INTERCEPT_CPUID;

  // Intercept VMRUN (required)
  Vmcb->ControlArea.InterceptMisc2 = INTERCEPT_VMRUN | INTERCEPT_VMMCALL;

  // Set up MSR permission map
  Vmcb->ControlArea.MsrpmBasePa =
      MmGetPhysicalAddress(Context->MsrPermissionsMap).QuadPart;

  // Guest ASID (must be non-zero)
  Vmcb->ControlArea.GuestAsid = 1;

  // Enable Nested Paging (NPT) for memory virtualization
  Vmcb->ControlArea.NpEnable = NP_ENABLE;
  // Note: NCr3 would be set to our NPT root page table

  // === State Save Area Setup ===
  // Copy current CPU state to become the guest state

  // Segment registers
  Vmcb->StateSaveArea.Cs.Selector = __readcs();
  Vmcb->StateSaveArea.Ds.Selector = __readds();
  Vmcb->StateSaveArea.Es.Selector = __reades();
  Vmcb->StateSaveArea.Ss.Selector = __readss();
  Vmcb->StateSaveArea.Fs.Selector = __readfs();
  Vmcb->StateSaveArea.Gs.Selector = __readgs();

  // Control registers
  Vmcb->StateSaveArea.Cr0 = __readcr0();
  Vmcb->StateSaveArea.Cr3 = __readcr3();
  Vmcb->StateSaveArea.Cr4 = __readcr4();

  // EFER (must have SVME set for nested operation)
  Vmcb->StateSaveArea.Efer = __readmsr(MSR_EFER);

  // Debug registers
  Vmcb->StateSaveArea.Dr6 = __readdr(6);
  Vmcb->StateSaveArea.Dr7 = __readdr(7);

  // RIP and RSP will be set by the caller
  Vmcb->StateSaveArea.Rflags = __readeflags();
}

// Handle CPUID interception - hide hypervisor
VOID SvmHandleCpuid(PHV_CPU_CONTEXT Context) {
  PVMCB vmcb = Context->Vmcb;
  int cpuInfo[4];

  // Get the CPUID leaf being requested
  UINT32 leaf = (UINT32)vmcb->StateSaveArea.Rax;

  // Execute real CPUID
  __cpuid(cpuInfo, leaf);

  // Modify results to hide hypervisor presence
  switch (leaf) {
  case 0x40000000: // Hypervisor vendor leaf
    // Return zeros - pretend no hypervisor
    cpuInfo[0] = 0;
    cpuInfo[1] = 0;
    cpuInfo[2] = 0;
    cpuInfo[3] = 0;
    break;

  case 1: // Feature flags
    // Clear hypervisor present bit (ECX.31)
    cpuInfo[2] &= ~(1 << 31);
    break;
  }

  // Store results back (RAX is in VMCB, others in guest GPRs)
  vmcb->StateSaveArea.Rax = cpuInfo[0];
  // RBX, RCX, RDX would need to be in a separate guest GPR area

  // Advance RIP past CPUID instruction (2 bytes)
  vmcb->StateSaveArea.Rip += 2;
}

// Handle VMMCALL - our custom hypercall interface
VOID SvmHandleVmmcall(PHV_CPU_CONTEXT Context) {
  PVMCB vmcb = Context->Vmcb;

  // RAX contains the hypercall number
  UINT64 callNumber = vmcb->StateSaveArea.Rax;

  switch (callNumber) {
  case 0:                                 // Ping - return magic value
    vmcb->StateSaveArea.Rax = 0x54415052; // "TAPR"
    break;

  case 1: // Hide memory region (for DLL injection)
    // TODO: Implement NPT manipulation
    break;

  case 2: // Unhide memory region
    // TODO: Implement NPT manipulation
    break;

  default:
    vmcb->StateSaveArea.Rax = 0xFFFFFFFF; // Unknown call
    break;
  }

  // Advance RIP past VMMCALL instruction (3 bytes)
  vmcb->StateSaveArea.Rip += 3;
}

// Main VM exit handler
VOID SvmHandleExit(PHV_CPU_CONTEXT Context) {
  PVMCB vmcb = Context->Vmcb;
  UINT64 exitCode = vmcb->ControlArea.ExitCode;

  switch (exitCode) {
  case VMEXIT_CPUID:
    SvmHandleCpuid(Context);
    break;

  case VMEXIT_VMMCALL:
    SvmHandleVmmcall(Context);
    break;

  case VMEXIT_NPF: // Nested Page Fault
    // This is where we intercept memory access for hiding DLLs
    // TODO: Implement EPT/NPT page swapping
    break;

  default:
    // Unhandled exit - should not happen in production
    KdPrint(("[HV] Unhandled VMEXIT: 0x%llX\n", exitCode));
    break;
  }
}

// Driver entry point
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                     PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  KdPrint(("[HV] TaaprWare Hypervisor Loading...\n"));

  // Check SVM support
  if (!SvmIsSupported()) {
    KdPrint(("[HV] AMD SVM not supported or disabled!\n"));
    return STATUS_NOT_SUPPORTED;
  }

  KdPrint(("[HV] AMD SVM is supported, Max ASID: %u\n", SvmGetMaxAsid()));

  // TODO: Initialize per-processor contexts
  // TODO: Virtualize all processors via DPC

  DriverObject->DriverUnload = NULL; // Hypervisor can't unload

  return STATUS_SUCCESS;
}
