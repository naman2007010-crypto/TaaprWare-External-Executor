# TaaprWare - Roblox Script Executor

A comprehensive Roblox script execution framework with multiple approaches for maximum compatibility.

## 🚀 Quick Start

### Option 1: Internal DLL (Recommended - Highest Success Rate)

```powershell
# 1. Start Roblox and join a game
# 2. Run the injector as Administrator
cd "TaaprWare V3\build"
.\Injector.exe

# 3. Press DELETE in-game to execute scripts
```

### Option 2: External Executor (No DLL Injection)

```powershell
# 1. Start Roblox and join a game
# 2. Run the external executor
cd "TaaprWare-External\build"
.\TaaprWare-External.exe

# 3. Wait for injection to complete
```

---

## 📦 Components

| Component | Type | Path | Description |
|-----------|------|------|-------------|
| **External Executor** | EXE | `TaaprWare-External/build/` | Thread hijacking, no DLL |
| **Internal DLL** | DLL | `TaaprWare V3/build/` | Runs inside Roblox process |
| **Injector** | EXE | `TaaprWare V3/build/` | Auto-injects the DLL |
| **Hypervisor** | Driver | `Hypervisor/` | Ring -1 evasion (advanced) |

---

## 🔧 Building From Source

### Prerequisites
- Visual Studio 2022+ with C++ workload
- Windows SDK 10.0+

### Build External Executor
```powershell
cd TaaprWare-External
.\build.bat
```

### Build Internal DLL + Injector
```powershell
cd "TaaprWare V3"
.\build.bat
```

---

## 🛡️ Anti-Cheat Bypass Features

### External Executor
- ✅ Direct syscalls (NtWriteVirtualMemory, NtCreateSection)
- ✅ Section mapping instead of VirtualAllocEx
- ✅ Thread hijacking instead of CreateRemoteThread
- ✅ Multi-pattern signature scanning
- ✅ Bytecode protection (XOR + ProtectedString header)
- ✅ 32-byte MSVC std::string SSO emulation

### Internal DLL
- ✅ Real C++ std::string objects
- ✅ Direct function calls
- ✅ Anti-detection thread randomization
- ✅ Runtime pattern scanning

### Hypervisor (Advanced)
- ✅ Ring -1 execution (below kernel)
- ✅ CPUID interception (hides hypervisor)
- ✅ Memory region hiding via NPT
- ✅ luavm_load intercept hypercalls

---

## 📝 Custom Scripts

Edit the script file before running:

**External:** `TaaprWare-External/build/BoostingNationHub.lua`

**Internal:** Modify `dllmain.cpp` → `test_script` variable

Example script:
```lua
print("Hello from TaaprWare!")

local part = Instance.new("Part")
part.Size = Vector3.new(10, 10, 10)
part.Position = Vector3.new(0, 100, 0)
part.BrickColor = BrickColor.new("Bright red")
part.Anchored = true
part.Material = Enum.Material.Neon
part.Parent = game.Workspace

game:GetService("StarterGui"):SetCore("SendNotification", {
    Title = "TaaprWare",
    Text = "Script executed!",
    Duration = 10
})
```

---

## 🔍 Troubleshooting

### "Injection Failed"
- Run as Administrator
- Disable antivirus temporarily
- Make sure Roblox is fully loaded (in a game)

### "lua_State not found"
- Roblox may have updated offsets
- Try the external executor instead
- Check Discord for updated patterns

### "No output in F9"
- The luavm_load signature may have changed
- Use Cheat Engine to dump current offsets
- Update patterns in `roblox.h` or `main.cpp`

---

## 📊 Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    TaaprWare Framework                       │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────┐    ┌──────────────────┐              │
│  │ External Executor │    │  Internal DLL    │              │
│  │  (TaaprWare-Ext) │    │  (TaaprWare V3)  │              │
│  ├──────────────────┤    ├──────────────────┤              │
│  │ • Thread Hijack  │    │ • LoadLibrary    │              │
│  │ • Shellcode      │    │ • Direct Calls   │              │
│  │ • Section Map    │    │ • Real Objects   │              │
│  └────────┬─────────┘    └────────┬─────────┘              │
│           │                       │                         │
│           └───────────┬───────────┘                         │
│                       ▼                                     │
│           ┌──────────────────────┐                         │
│           │   Roblox Process     │                         │
│           │  ┌────────────────┐  │                         │
│           │  │  luavm_load()  │  │                         │
│           │  │  task_defer()  │  │                         │
│           │  │   lua_State    │  │                         │
│           │  └────────────────┘  │                         │
│           └──────────────────────┘                         │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## ⚠️ Disclaimer

This project is for **educational purposes only**. 

- Do not use on accounts you care about
- May result in bans from Roblox
- Use at your own risk

---

## 📜 License

MIT License - See LICENSE file

---

## 🔗 Links

- **Discord:** For offset updates and support
- **GitHub:** https://github.com/naman2007010-crypto/TaaprWare-External-Executor
