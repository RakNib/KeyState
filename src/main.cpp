#include <windows.h>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <ctime>
#include "config.h"
#include "keyboard.h"
#include "display_ui.h"
#include "chart_ui.h"
#include "settings_ui.h"
#include "theme_editor.h"
#include "lang.h"
#include "imgui_setup.h"

// 自定义消息
#define WM_TRAY_ICON      (WM_APP + 1)
#define WM_OVERLAY_RCLICK (WM_APP + 2)

// 全局对象
static AppConfig       g_config;
static KeyStateManager g_keyState;
static KeyboardHook    g_keyHook;
static MouseHook       g_mouseHook;
static DisplayUI       g_display;
static ChartUI          g_chart;
static SettingsUI      g_settings;
static ThemeEditor     g_themeEditor;

static HINSTANCE       g_hInst       = nullptr;
static HWND            g_hMainWnd    = nullptr;
static NOTIFYICONDATAW g_nid         = {};
static HICON           g_hIcon       = nullptr;

static std::atomic<bool> g_running = true;

// ========== 录制功能 ==========
static std::atomic<bool> g_recording     = false;
static std::atomic<bool> g_recordToggle  = false;
static uint64_t          g_recStartTime  = 0;

struct RecordingSnapshot {
    uint64_t timeMs;
    uint64_t totalPresses;
    int      totalKPS;
    std::vector<uint64_t> keyTotals;
    std::vector<int>      keyKPS;
};
static std::vector<RecordingSnapshot> g_recData;
static std::vector<std::wstring>      g_recKeyLabels;

static void SaveRecording() {
    if (g_recData.empty()) return;
    // 使用配置的输出目录（为空则默认 exe 目录下的 record）
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    std::wstring recordDir = g_config.recordingDir.empty()
        ? (std::wstring(exePath) + L"record")
        : g_config.recordingDir;
    CreateDirectoryW(recordDir.c_str(), nullptr);
    // 确保末尾有反斜杠
    if (!recordDir.empty() && recordDir.back() != L'\\') recordDir += L'\\';
    time_t t = time(nullptr);
    struct tm tm;
    localtime_s(&tm, &t);
    wchar_t fname[MAX_PATH];
    swprintf(fname, MAX_PATH, L"%lsKeyStateRecording_%04d%02d%02d_%02d%02d%02d.json",
             recordDir.c_str(), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    std::wstring out;
    out += L"{\n";
    out += L"  \"version\": 1,\n";
    out += L"  \"startTime\": "; out += std::to_wstring(g_recStartTime);
    out += L",\n  \"durationMs\": "; out += std::to_wstring(g_recData.back().timeMs);
    out += L",\n  \"keys\": [\n";
    for (size_t i = 0; i < g_recKeyLabels.size(); ++i) {
        out += L"    \"" + g_recKeyLabels[i] + L"\"";
        if (i + 1 < g_recKeyLabels.size()) out += L",";
        out += L"\n";
    }
    out += L"  ],\n  \"data\": [\n";
    for (size_t i = 0; i < g_recData.size(); ++i) {
        auto& d = g_recData[i];
        out += L"    {\n";
        out += L"      \"time\": " + std::to_wstring(d.timeMs) + L",\n";
        out += L"      \"totalPresses\": " + std::to_wstring(d.totalPresses) + L",\n";
        out += L"      \"totalKPS\": " + std::to_wstring(d.totalKPS) + L",\n";
        out += L"      \"keys\": [\n";
        for (size_t j = 0; j < d.keyTotals.size(); ++j) {
            out += L"        {\"total\": " + std::to_wstring(d.keyTotals[j]);
            out += L", \"kps\": " + std::to_wstring(d.keyKPS[j]) + L"}";
            if (j + 1 < d.keyTotals.size()) out += L",";
            out += L"\n";
        }
        out += L"      ]\n    }";
        if (i + 1 < g_recData.size()) out += L",";
        out += L"\n";
    }
    out += L"  ]\n}\n";
    int ulen = WideCharToMultiByte(CP_UTF8, 0, out.c_str(), (int)out.size(), nullptr, 0, nullptr, nullptr);
    std::string utf8(ulen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, out.c_str(), (int)out.size(), &utf8[0], ulen, nullptr, nullptr);
    HANDLE h = CreateFileW(fname, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(h, utf8.c_str(), (DWORD)utf8.size(), &written, nullptr);
        CloseHandle(h);
    }
    wchar_t msgBuf[512];
    swprintf(msgBuf, 512, L"%ls\n%ls", LANG(104), fname);
    MessageBoxW(g_hMainWnd, msgBuf, LANG(103), MB_ICONINFORMATION);
}

// ========== 配置文件路径 ==========
static std::wstring g_configPath;

static bool FileExists(const wchar_t* path) {
    DWORD attr = GetFileAttributesW(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

static std::wstring GetConfigPath() {
    if (!g_configPath.empty()) return g_configPath;
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *lastSlash = L'\0';
    g_configPath = std::wstring(exePath) + L"\\KeyStateSetting.json";
    return g_configPath;
}

static HICON LoadAppIcon() {
    HICON h = LoadIconW(GetModuleHandleW(nullptr), L"IDI_KEYSTATE");
    if (h) return h;
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    std::wstring iconPath = std::wstring(exePath) + L"favicon.ico";
    return (HICON)LoadImageW(nullptr, iconPath.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
}

static void SyncAndSave() {
    for (auto& kc : g_config.keys)
        kc.totalPresses = g_keyState.GetTotal(kc.keyCode);
    g_config.Save(GetConfigPath().c_str());
}

// ========== 托盘 ==========
static const UINT TRAY_ID = 1;

static void AddTrayIcon(HWND hwnd) {
    g_nid.cbSize           = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd             = hwnd;
    g_nid.uID              = TRAY_ID;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAY_ICON;
    g_nid.hIcon            = g_hIcon ? g_hIcon : LoadIconW(nullptr, (LPCWSTR)IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"KeyState - Keyboard Monitor");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

static void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

static void ShowTrayMenu(HWND hwnd) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 1, L"Settings");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 2, L"Exit");
    POINT pt; GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);
    if (cmd == 1) { g_settings.Show(true); }
    if (cmd == 2) { PostQuitMessage(0); }
}

// ========== 注册快捷键 ==========
static void RegisterAllHotkeys(HWND hwnd) {
    RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_SHIFT, g_config.hotkeySettingsVK);
    RegisterHotKey(hwnd, 2, MOD_CONTROL | MOD_SHIFT, g_config.hotkeyThemeEditorVK);
    RegisterHotKey(hwnd, 3, MOD_CONTROL | MOD_SHIFT, g_config.hotkeyToggleDisplayVK);
    RegisterHotKey(hwnd, 4, MOD_CONTROL | MOD_SHIFT, g_config.hotkeyNextThemeVK);
    RegisterHotKey(hwnd, 5, MOD_CONTROL | MOD_SHIFT, g_config.hotkeyPrevThemeVK);
    RegisterHotKey(hwnd, 6, MOD_CONTROL | MOD_SHIFT, g_config.hotkeyToggleTrackVK);
    RegisterHotKey(hwnd, 7, MOD_CONTROL | MOD_SHIFT, g_config.hotkeyToggleChartVK);
}

// ========== 主窗口过程 ==========
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        AddTrayIcon(hwnd);
        RegisterAllHotkeys(hwnd);
        // V1.6: 设置定时器驱动 ImGui 渲染
        SetTimer(hwnd, 1, 16, nullptr); // ~60 FPS
        break;

    case WM_TIMER:
        // V1.6: 设置面板渲染包含所有子窗口（主题编辑器、快捷键编辑器）
        if (!SettingsUI::s_inModalDialog) {
            if (g_settings.IsVisible()) g_settings.RenderImGui();
        }
        break;

    case WM_HOTKEY:
        if (wp == 1) g_settings.Show(true);
        if (wp == 2) g_themeEditor.Show(true);
        if (wp == 3) {
            bool vis = IsWindowVisible(g_display.GetHwnd());
            g_display.Show(!vis);
        }
        if (wp == 4) g_settings.CycleTheme(1);
        if (wp == 5) g_settings.CycleTheme(-1);
        if (wp == 6) {
            g_config.showHistory = !g_config.showHistory;
            g_config.Save(GetConfigPath().c_str());
        }
        if (wp == 7) {
            g_config.showChart = !g_config.showChart;
            g_chart.Show(g_config.showChart);
            g_config.Save(GetConfigPath().c_str());
        }
        break;

    case WM_TRAY_ICON:
        if (LOWORD(lp) == WM_RBUTTONUP) ShowTrayMenu(hwnd);
        if (LOWORD(lp) == WM_LBUTTONUP) g_settings.Show(true);
        break;

    case WM_OVERLAY_RCLICK:
        ShowTrayMenu(hwnd);
        break;

    case WM_APP + 5:
        UnregisterHotKey(hwnd, 1); UnregisterHotKey(hwnd, 2);
        UnregisterHotKey(hwnd, 3); UnregisterHotKey(hwnd, 4);
        UnregisterHotKey(hwnd, 5); UnregisterHotKey(hwnd, 6);
        UnregisterHotKey(hwnd, 7);
        RegisterAllHotkeys(hwnd);
        break;

    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

// ========== 叠加层更新线程 ==========
static void OverlayUpdateLoop() {
    timeBeginPeriod(1);
    uint64_t lastRecSample = 0;
    uint64_t lastRecUiUpdate = 0;

    while (g_running) {
        uint64_t now = GetTickCount64();

        g_display.UpdateOverlay(g_config, g_keyState);
        if (g_config.showChart) g_chart.UpdateChart(g_config, g_keyState);

        if (now - lastRecUiUpdate >= 500) {
            lastRecUiUpdate = now;
            g_settings.UpdateRecordStatus(g_recording, g_recStartTime);
        }

        if (g_recording) {
            if (lastRecSample == 0 || now - lastRecSample >= 1000) {
                lastRecSample = now;
                RecordingSnapshot snap;
                snap.timeMs = now - g_recStartTime;
                uint64_t sumTotal = 0;
                double   sumKPS   = 0.0;
                for (size_t i = 0; i < g_config.keys.size(); ++i) {
                    auto& kc = g_config.keys[i];
                    uint64_t t = g_keyState.GetTotal(kc.keyCode);
                    double   k = g_keyState.GetKPS(kc.keyCode);
                    snap.keyTotals.push_back(t);
                    snap.keyKPS.push_back((int)k);
                    sumTotal += t;
                    sumKPS   += k;
                }
                snap.totalPresses = sumTotal;
                snap.totalKPS     = (int)sumKPS;
                g_recData.push_back(snap);
            }
        }

        int fps = g_config.fps;
        if (fps <= 0) fps = 45;
        Sleep(1000 / fps);
    }
    timeEndPeriod(1);
}

// ========== 程序入口 ==========
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInst = hInstance;

    // 1. 加载配置
    auto cfgPath = GetConfigPath();
    SetLanguage(0);
    wchar_t exeDir[MAX_PATH];
    wcscpy_s(exeDir, MAX_PATH, cfgPath.c_str());
    wchar_t* lastSlash2 = wcsrchr(exeDir, L'\\');
    if (lastSlash2) *(lastSlash2 + 1) = L'\0';
    std::wstring themePath = std::wstring(exeDir) + L"KeyStateThemes.json";
    if (!FileExists(themePath.c_str())) {
        g_config.InitDefaultThemes();
        g_config.SaveThemes(themePath.c_str());
    }
    g_config.LoadThemes(themePath.c_str());
    g_hIcon = LoadAppIcon();

    if (FileExists(cfgPath.c_str())) {
        g_config.Load(cfgPath.c_str());
    } else {
        g_config = AppConfig{};
        g_config.keys.push_back({32, L"Space", {235,235,245,255}, {48,48,48,200}, {255,95,95,255}});
        g_config.keys.push_back({65, L"A",     {235,235,245,255}, {48,48,48,200}, {95,255,95,255}});
        g_config.keys.push_back({83, L"S",     {235,235,245,255}, {48,48,48,200}, {95,130,255,255}});
        g_config.keys.push_back({68, L"D",     {235,235,245,255}, {48,48,48,200}, {255,210,60,255}});
        g_config.Save(cfgPath.c_str());
        g_config.LoadThemes(themePath.c_str());
    }

    bool cleaned = false;
    g_config.keys.erase(
        std::remove_if(g_config.keys.begin(), g_config.keys.end(),
            [&cleaned](const KeyConfig& kc) {
                if (kc.keyCode == 229) { cleaned = true; return true; }
                return false;
            }),
        g_config.keys.end());
    if (g_config.keys.empty()) {
        g_config.keys.push_back({32, L"Space", {235,235,245,255}, {48,48,48,200}, {255,95,95,255}});
        cleaned = true;
    }
    if (cleaned) g_config.Save(cfgPath.c_str());
    SetLanguage(g_config.lang);
    for (auto& kc : g_config.keys) {
        kc.totalPresses = 0;
        g_recKeyLabels.push_back(kc.label);
    }

    // 2. 创建隐藏主窗口
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"KeyStateMainV3";
    wc.hIcon         = g_hIcon;
    RegisterClassExW(&wc);
    g_hMainWnd = CreateWindowExW(0, L"KeyStateMainV3", L"KeyState Main",
                                  WS_OVERLAPPED, 0, 0, 0, 0,
                                  nullptr, nullptr, hInstance, nullptr);

    // 3. 创建 State 悬浮叠加层
    g_display.Create(hInstance);
    g_display.SetConfig(&g_config);
    g_display.SetConfigPath(cfgPath.c_str());
    g_display.SetClickThrough(g_config.clickThrough);
    g_display.SetTopMost(g_config.alwaysOnTop);
    g_display.SetNotifyHwnd(g_hMainWnd);
    g_display.Show(true);

    // 3.5 创建 KPS Chart 悬浮窗口
    g_chart.Create(hInstance);
    g_chart.SetConfig(&g_config);
    g_chart.SetConfigPath(cfgPath.c_str());
    g_chart.SetTopMost(g_config.alwaysOnTop);
    g_chart.SetNotifyHwnd(g_hMainWnd);
    g_chart.Show(g_config.showChart);

    // 4. 创建 ImGui 设置窗口
    g_settings.Create(hInstance, &g_config, &g_display, &g_chart, &g_themeEditor, &g_keyState);
    g_settings.SetConfigPath(cfgPath.c_str());
    g_settings.Show(true);
    // 应用保存的 UI 主题
    ApplyImGuiTheme(g_config.uiTheme);

    // 4.5 创建主题编辑器
    g_themeEditor.Create(hInstance, &g_config);
    g_themeEditor.SetConfigPath(themePath.c_str());
    g_themeEditor.SetSettings(&g_settings);

    // 5. 安装键盘钩子 & 鼠标钩子
    auto inputCallback = [&](int keyCode, bool pressed) {
        if (pressed && g_config.recordingHotkeyVK != 0 && keyCode == g_config.recordingHotkeyVK) {
            if (!g_recording) {
                g_recording = true;
                g_recData.clear();
                g_recStartTime = GetTickCount64();
                g_recKeyLabels.clear();
                for (auto& kc : g_config.keys)
                    g_recKeyLabels.push_back(kc.label);
            } else {
                g_recording = false;
                SaveRecording();
            }
            return;
        }
        for (auto& kc : g_config.keys) {
            if (kc.keyCode == keyCode) {
                g_keyState.OnKeyEvent(keyCode, pressed);
                return;
            }
        }
    };
    g_keyHook.Install(inputCallback);
    g_mouseHook.Install(inputCallback);

    // 6. 启动叠加层渲染线程
    std::thread renderThread(OverlayUpdateLoop);

    // 7. 消息循环（ImGui 由 WM_TIMER 驱动渲染）
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 8. 清理
    g_running = false;
    if (renderThread.joinable()) renderThread.join();
    g_keyHook.Uninstall();
    g_mouseHook.Uninstall();
    SyncAndSave();

    if (g_recording) {
        g_recording = false;
        SaveRecording();
    }

    return 0;
}
