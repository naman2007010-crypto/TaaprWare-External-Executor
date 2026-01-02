# TaaprWare Hypervisor

AMD-V (SVM) based hypervisor for bypassing Hyperion anti-cheat.

## Architecture

```
┌──────────────────────────────────┐
│     Roblox + Hyperion            │ ← Ring 3
├──────────────────────────────────┤
│     Windows Kernel               │ ← Ring 0  
├──────────────────────────────────┤
│  ★ TaaprWare Hypervisor ★        │ ← Ring -1 (VMX Root)
├──────────────────────────────────┤
│     AMD Ryzen Hardware           │
└──────────────────────────────────┘
```

## Files

| File | Description |
|:---|:---|
| `include/svm.h` | AMD SVM structures and constants |
| `src/svm.c` | Core hypervisor implementation |
| `src/svm_asm.asm` | VMRUN/VMEXIT assembly routines |

## Prerequisites

1. **Windows Driver Kit (WDK)**
   - Download: https://go.microsoft.com/fwlink/?linkid=2196230
   - Required: Visual Studio 2022 + WDK 10

2. **Enable Test Signing**
   ```cmd
   bcdedit /set testsigning on
   ```
   Reboot required.

3. **Disable Secure Boot in BIOS**
   For loading unsigned drivers.

## Building

1. Open Visual Studio 2022
2. Create new "Empty WDM Driver" project
3. Add source files from this folder
4. Set target architecture to x64
5. Build in Debug mode (for test signing)

## Loading

```powershell
# Create service
sc create TaaprHV type=kernel start=demand binPath=C:\path\to\driver.sys

# Start hypervisor
sc start TaaprHV
```

## Safety Warning

⚠️ **This is experimental kernel code that can cause:**
- System crashes (BSOD)
- Data corruption
- Security vulnerabilities

**Test in a VM first!**
