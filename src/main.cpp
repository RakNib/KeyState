#include <windows.h>
#include <thread>
#include <atomic>
#include <string>
#include <algorithm>
#include "config.h"
#include "keyboard.h"
#include "display_ui.h"
#include "chart_ui.h"
#include "settings_ui.h"
#include "theme_editor.h"
#include "lang.h"

// 自定义消息
#define WM_TRAY_ICON      (WM_APP + 1)
#define WM_OVERLAY_RCLICK (WM_APP + 2)

// 全局对象
static AppConfig       g_config;
static KeyStateManager g_keyState;
static KeyboardHook    g_keyHook;
static DisplayUI       g_display;
static ChartUI          g_chart;
static SettingsUI      g_settings;
static ThemeEditor     g_themeEditor;

static HINSTANCE       g_hInst       = nullptr;
static HWND            g_hMainWnd    = nullptr;
static NOTIFYICONDATAW g_nid         = {};
static HICON           g_hIcon       = nullptr;

static std::atomic<bool> g_running = true;

// ========== 配置文件路径（exe 同目录 KeyStateSetting.json） ==========
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
    // 优先使用嵌入资源
    HICON h = LoadIconW(GetModuleHandleW(nullptr), L"IDI_KEYSTATE");
    if (h) return h;
    // 回退：从文件加载
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    std::wstring iconPath = std::wstring(exePath) + L"favicon.ico";
    return (HICON)LoadImageW(nullptr, iconPath.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
}

// 同步 Total 并保存
static void SyncAndSave() {
    for (auto& kc : g_config.keys)
        kc.totalPresses = g_keyState.GetTotal(kc.keyCode);
    g_config.Save(GetConfigPath().c_str());
}

// ========== 托盘相关 ==========
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
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                              pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);

    if (cmd == 1) { g_settings.Show(true); }
    if (cmd == 2) { PostQuitMessage(0); }
}

// ========== 主窗口过程（隐藏窗口，仅用于消息泵和托盘） ==========
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        AddTrayIcon(hwnd);
        // 注册热键 Ctrl+Shift+K 打开设置, Ctrl+Shift+T 主题编辑器
        RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_SHIFT, 'K');
        RegisterHotKey(hwnd, 2, MOD_CONTROL | MOD_SHIFT, 'T');
        break;

    case WM_HOTKEY:
        if (wp == 1) g_settings.Show(true);
        if (wp == 2) g_themeEditor.Show(true);
        break;

    case WM_TRAY_ICON:
        if (LOWORD(lp) == WM_RBUTTONUP) ShowTrayMenu(hwnd);
        if (LOWORD(lp) == WM_LBUTTONUP) g_settings.Show(true);
        break;

    case WM_OVERLAY_RCLICK:
        ShowTrayMenu(hwnd);
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
    // 使用 timeBeginPeriod 提高定时器精度
    timeBeginPeriod(1);

    while (g_running) {
        g_display.UpdateOverlay(g_config, g_keyState);
        if (g_config.showChart) g_chart.UpdateChart(g_config, g_keyState);
        int fps = g_config.fps;
        if (fps <= 0) fps = 45;
        Sleep(1000 / fps);
    }

    timeEndPeriod(1);
}

// ========== 程序入口 ==========
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInst = hInstance;

    // 1. 加载 KeyStateSetting.json（exe 同目录）
    auto cfgPath = GetConfigPath();
    SetLanguage(0);

    // 提取 exe 目录用于主题文件
    wchar_t exeDir[MAX_PATH];
    wcscpy_s(exeDir, MAX_PATH, cfgPath.c_str());
    wchar_t* lastSlash2 = wcsrchr(exeDir, L'\\');
    if (lastSlash2) *(lastSlash2 + 1) = L'\0';
    std::wstring themePath = std::wstring(exeDir) + L"KeyStateThemes.json";
    // 生成默认主题文件（不存在时）
    if (!FileExists(themePath.c_str())) {
        g_config.InitDefaultThemes();
        g_config.SaveThemes(themePath.c_str());
    }
    g_config.LoadThemes(themePath.c_str());

    // 加载自定义图标
    g_hIcon = LoadAppIcon();

    if (FileExists(cfgPath.c_str())) {
        g_config.Load(cfgPath.c_str());
    } else {
        // 文件不存在 → 创建默认配置并写入
        g_config = AppConfig{};
        g_config.keys.push_back({32, L"Space", {235,235,245,255}, {48,48,48,200}, {255,95,95,255}});
        g_config.keys.push_back({65, L"A",     {235,235,245,255}, {48,48,48,200}, {95,255,95,255}});
        g_config.keys.push_back({83, L"S",     {235,235,245,255}, {48,48,48,200}, {95,130,255,255}});
        g_config.keys.push_back({68, L"D",     {235,235,245,255}, {48,48,48,200}, {255,210,60,255}});
        g_config.Save(cfgPath.c_str());
        g_config.LoadThemes(themePath.c_str()); // AppConfig{} 清空了 themePresets, 需重新加载
    }

    // 清理 IME 虚拟键脏数据 + 边界保护
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

    // 应用语言
    SetLanguage(g_config.lang);

    // Total 每次启动从 0 开始
    for (auto& kc : g_config.keys) {
        kc.totalPresses = 0;
    }

    // 2. 创建隐藏主窗口（消息泵 + 托盘 + 热键）
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
    g_display.SetConfigPath(cfgPath.c_str());       // 拖拽保存用
    g_display.SetClickThrough(g_config.clickThrough);
    g_display.SetNotifyHwnd(g_hMainWnd);
    g_display.Show(true);

    // 3.5 创建 KPS Chart 悬浮窗口
    g_chart.Create(hInstance);
    g_chart.SetConfig(&g_config);
    g_chart.SetConfigPath(cfgPath.c_str());
    g_chart.SetNotifyHwnd(g_hMainWnd);
    g_chart.Show(g_config.showChart);

    // 4. 创建设置窗口
    g_settings.Create(hInstance, &g_config, &g_display, &g_chart, &g_themeEditor, &g_keyState);
    g_settings.SetConfigPath(cfgPath.c_str());
    g_settings.Show(true);

    // 4.5 创建主题编辑器（Ctrl+Shift+T 打开）
    g_themeEditor.Create(hInstance, &g_config);
    g_themeEditor.SetConfigPath(themePath.c_str());
    g_themeEditor.SetSettings(&g_settings);

    // 5. 安装键盘钩子
    g_keyHook.Install([](int keyCode, bool pressed) {
        // 只跟踪配置中的按键
        for (auto& kc : g_config.keys) {
            if (kc.keyCode == keyCode) {
                g_keyState.OnKeyEvent(keyCode, pressed);
                return;
            }
        }
    });

    // 6. 启动叠加层渲染线程
    std::thread renderThread(OverlayUpdateLoop);

    // 7. 消息循环
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 8. 清理
    g_running = false;
    if (renderThread.joinable()) renderThread.join();

    g_keyHook.Uninstall();

    // 最终保存
    SyncAndSave();

    return 0;
}
