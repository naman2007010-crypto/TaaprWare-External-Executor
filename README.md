# TaaprWare External Executor

External memory manipulation tool for Roblox script execution.

## Features

- ✅ **No DLL Injection** - External process only
- ✅ **Secure Boot Compatible** - No kernel drivers
- ✅ **Auto-Execute** - Loads Boosting Nation Hub V3
- ✅ **Pattern Scanning** - Finds Luau VM dynamically

## Building

### Requirements
- Visual Studio 2019/2022 with C++ Desktop Development
- Windows 10/11

### Build Steps
```cmd
build.bat
```

This will create `build\TaaprWare-External.exe`

## Usage

1. **Start Roblox** (join any game)
2. **Run TaaprWare-External.exe**
3. **Script auto-executes**

## Current Status

⚠️ **Work in Progress**

- [x] Memory reader (OpenProcess, Read/Write)
- [x] Process attachment
- [x] Pattern scanning framework
- [ ] Luau VM detection (needs current build signatures)
- [ ] Script execution (needs reverse engineering)

## Next Steps

To complete this, we need to:
1. Reverse engineer current Roblox build  
2. Find Luau VM signatures
3. Implement script bytecode injection

## Note

This is an **educational project** for security research.
