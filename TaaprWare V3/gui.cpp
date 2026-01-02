// TaaprWare V3 - Simple GUI Launcher
// No fancy controls, just basic Win32

#include <Windows.h>
#include <fstream>
#include <string>


#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ntdll.lib")

// Window handles
HWND g_hWnd = NULL;
HWND g_hEdit = NULL;
HWND g_hStatus = NULL;
HWND g_hInjectBtn = NULL;
HWND g_hExecuteBtn = NULL;

HBRUSH g_hBgBrush = NULL;
HFONT g_hFont = NULL;

// Native API
typedef LONG NTSTATUS;
extern "C" NTSTATUS NTAPI NtQuerySystemInformation(ULONG, PVOID, ULONG, PULONG);

DWORD FindRoblox() {
  BYTE *buffer = new BYTE[1024 * 1024];
  NtQuerySystemInformation(5, buffer, 1024 * 1024, NULL);

  struct SPI {
    ULONG Next;
    ULONG Threads;
    BYTE Res[48];
    struct {
      USHORT Len;
      USHORT Max;
      PWSTR Buf;
    } Name;
    LONG Pri;
    HANDLE Pid;
  };

  SPI *proc = (SPI *)buffer;
  DWORD pid = 0;

  while (true) {
    if (proc->Name.Buf && wcsstr(proc->Name.Buf, L"RobloxPlayerBeta")) {
      pid = (DWORD)(ULONG_PTR)proc->Pid;
      break;
    }
    if (!proc->Next)
      break;
    proc = (SPI *)((BYTE *)proc + proc->Next);
  }

  delete[] buffer;
  return pid;
}

bool InjectDLL(DWORD pid, const char *dllPath) {
  HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
  if (!hProc)
    return false;

  LPVOID remote = VirtualAllocEx(hProc, NULL, strlen(dllPath) + 1,
                                 MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!remote) {
    CloseHandle(hProc);
    return false;
  }

  WriteProcessMemory(hProc, remote, dllPath, strlen(dllPath) + 1, NULL);

  HANDLE hThread =
      CreateRemoteThread(hProc, NULL, 0,
                         (LPTHREAD_START_ROUTINE)GetProcAddress(
                             GetModuleHandleA("kernel32"), "LoadLibraryA"),
                         remote, 0, NULL);

  if (!hThread) {
    VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
    CloseHandle(hProc);
    return false;
  }

  WaitForSingleObject(hThread, 10000);
  DWORD exitCode = 0;
  GetExitCodeThread(hThread, &exitCode);

  CloseHandle(hThread);
  VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
  CloseHandle(hProc);

  return exitCode != 0;
}

void SetStatus(const wchar_t *text) { SetWindowTextW(g_hStatus, text); }

void OnInject() {
  SetStatus(L"Finding Roblox...");

  DWORD pid = FindRoblox();
  if (!pid) {
    SetStatus(L"Roblox not found! Start a game first.");
    return;
  }

  SetStatus(L"Injecting...");

  char exePath[MAX_PATH];
  GetModuleFileNameA(NULL, exePath, MAX_PATH);
  std::string dllPath = exePath;
  dllPath =
      dllPath.substr(0, dllPath.find_last_of("\\/")) + "\\TaaprWareV3.dll";

  if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
    SetStatus(L"TaaprWareV3.dll not found!");
    return;
  }

  if (InjectDLL(pid, dllPath.c_str())) {
    SetStatus(L"INJECTED! Press DELETE in-game.");
  } else {
    SetStatus(L"Failed. Try as Administrator.");
  }
}

void OnExecute() {
  int len = GetWindowTextLengthA(g_hEdit);
  if (len == 0) {
    SetStatus(L"No script!");
    return;
  }

  char *script = new char[len + 1];
  GetWindowTextA(g_hEdit, script, len + 1);

  char exePath[MAX_PATH];
  GetModuleFileNameA(NULL, exePath, MAX_PATH);
  std::string scriptPath = exePath;
  scriptPath =
      scriptPath.substr(0, scriptPath.find_last_of("\\/")) + "\\script.lua";

  std::ofstream ofs(scriptPath);
  ofs << script;
  ofs.close();

  delete[] script;
  SetStatus(L"Script saved! Press DELETE in-game.");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_COMMAND:
    if ((HWND)lParam == g_hInjectBtn)
      OnInject();
    else if ((HWND)lParam == g_hExecuteBtn)
      OnExecute();
    break;

  case WM_CTLCOLOREDIT: {
    HDC hdc = (HDC)wParam;
    SetBkColor(hdc, RGB(40, 40, 55));
    SetTextColor(hdc, RGB(220, 220, 255));
    return (LRESULT)CreateSolidBrush(RGB(40, 40, 55));
  }

  case WM_DESTROY:
    if (g_hBgBrush)
      DeleteObject(g_hBgBrush);
    if (g_hFont)
      DeleteObject(g_hFont);
    PostQuitMessage(0);
    break;

  default:
    return DefWindowProcW(hWnd, msg, wParam, lParam);
  }
  return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nShow) {
  g_hBgBrush = CreateSolidBrush(RGB(30, 30, 40));
  g_hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, DEFAULT_PITCH, L"Consolas");

  WNDCLASSW wc = {};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInst;
  wc.lpszClassName = L"TaaprWareGUI";
  wc.hbrBackground = g_hBgBrush;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClassW(&wc);

  g_hWnd = CreateWindowExW(
      0, L"TaaprWareGUI", L"TaaprWare V3",
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT,
      CW_USEDEFAULT, 550, 400, NULL, NULL, hInst, NULL);

  // Script editor
  g_hEdit = CreateWindowExW(
      WS_EX_CLIENTEDGE, L"EDIT",
      L"-- Enter Lua script here\r\nprint('Hello from TaaprWare!')",
      WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, 10,
      10, 515, 270, g_hWnd, NULL, hInst, NULL);
  SendMessageW(g_hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);

  // Buttons
  g_hInjectBtn =
      CreateWindowW(L"BUTTON", L"INJECT", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    10, 290, 250, 35, g_hWnd, NULL, hInst, NULL);

  g_hExecuteBtn = CreateWindowW(L"BUTTON", L"SAVE SCRIPT",
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 275, 290,
                                250, 35, g_hWnd, NULL, hInst, NULL);

  // Status
  g_hStatus = CreateWindowW(L"STATIC", L"Ready - Click INJECT",
                            WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 335, 515, 25,
                            g_hWnd, NULL, hInst, NULL);

  ShowWindow(g_hWnd, nShow);

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  return 0;
}
