# 🛡️ Bypassing Hyperion (Byfron) Anti-Cheat

If Cheat Engine or x64dbg is getting detected (Roblox closing/crashing), follow these steps.

## Method 1: The "Suspend" Technique (Safest)
Hyperion cannot detect your debugger if the process is **paused**.

1.  **Download System Informer** (formerly Process Hacker).
2.  Open Roblox and join a game.
3.  Open System Informer as **Administrator**.
4.  Right-click `RobloxPlayerBeta.exe` -> **Suspend**.
5.  Now open Cheat Engine / x64dbg.
6.  **Attach** to Roblox.
7.  **Scan/Search** for what you need.
    *   *Note: You cannot see values change in real-time, but you can find static addresses.*
8.  Detach before Resuming (or acknowledge it will crash).

## Method 2: ScyllaHide (for x64dbg)
1.  Download **ScyllaHide** from GitHub.
2.  Copy the plugins to your `x64dbg/x64/plugins/` folder.
3.  Open x64dbg -> Plugins -> ScyllaHide -> Options.
4.  Select Profile: **VMProtect** or **Themida**.
5.  Enable **"NtQueryInformationProcess"** hooking.
6.  Try attaching.

## Method 3: Renaming Tools
Hyperion scans for window titles and process names.
*   Rename `Cheat Engine.exe` -> `Notepad.exe`
*   Rename `x96dbg.exe` -> `Calc.exe`
*   Change the window title if possible (Cheat Engine has settings for this).

## ⚠️ Important Note
I have updated the `TaaprWare-External` code with **Auto-Signature Scanning**.
Before you struggle with Cheat Engine, try running the new build! It attempts to find the standard "TaskScheduler" pattern automatically.
