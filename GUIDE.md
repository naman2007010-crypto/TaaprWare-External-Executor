# Step-by-Step Guide: Building & Testing TaaprWare External

## Prerequisites

You need a C++ compiler. Choose one:

### Option 1: Install Visual Studio (Recommended)
1. Download: https://visualstudio.microsoft.com/downloads/
2. Install "Desktop development with C++"
3. Run `build.bat`

### Option 2: Install MinGW (Faster)
```cmd
winget install MSYS2.MSYS2
```
Then in MSYS2 terminal:
```bash
pacman -S mingw-w64-x86_64-gcc
```
Add to PATH: `C:\msys64\mingw64\bin`
Run `build-mingw.bat`

---

## Building

Once you have a compiler:

```cmd
cd c:\Users\naman\Desktop\Bnation\roblox\TaaprWare-External
build.bat
```

OR

```cmd
build-mingw.bat
```

---

## Testing

### Step 1: Start Roblox
1. Open Roblox Player
2. Join any game

### Step 2: Run TaaprWare
```cmd
cd build
TaaprWare-External.exe
```

### Expected Output:
```
╔════════════════════════════════════════╗
║     TaaprWare V3 - External Mode      ║
║   Boosting Nation Hub Auto-Executor   ║
╚════════════════════════════════════════╝

[*] Searching for Roblox...
[+] Attached to Roblox (PID: 12345)
[+] Base Address: 0x7FF6A0000000
[*] Initializing script injector...
[*] Scanning for Luau VM...
```

### Current Limitations:
- ⚠️ Will attach to Roblox ✅
- ⚠️ Will NOT execute scripts yet ❌
  (Needs Luau VM reverse engineering)

---

## What's Next?

To make script execution work, we need:

1. **Find Luau VM signatures** for current Roblox build
2. **Reverse engineer** script execution functions
3. **Implement** bytecode injection

This requires using tools like:
- **Cheat Engine** (memory scanning)
- **x64dbg** (debugging)
- **IDA Pro/Ghidra** (disassembly)

Would you like guidance on reverse engineering the Luau VM?
