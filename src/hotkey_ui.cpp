#include "hotkey_ui.h"
#include <cstdio>

// 自定义消息：快捷键已变更，主窗口需重新注册热键
#define WM_HOTKEY_CHANGED (WM_APP + 5)

// ========== 将 VK 码转为可读的按键名 ==========
static void GetVKName(int vk, wchar_t* buf, int bufLen) {
    if (vk <= 0) { wcscpy_s(buf, bufLen, L"(未设置)"); return; }
    UINT sc = MapVirtualKeyW((UINT)vk, MAPVK_VK_TO_VSC);
    LONG lp = (sc << 16);
    // 扩展键标记
    switch (vk) {
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
    case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
    case VK_INSERT: case VK_DELETE: case VK_DIVIDE:
    case VK_NUMLOCK:
        lp |= 0x01000000; break;
    }
    if (GetKeyNameTextW(lp, buf, bufLen) > 0) return;

    // 回退：常见键名
    struct { int vk; const wchar_t* name; } table[] = {
        {VK_BACK,L"Backspace"},{VK_TAB,L"Tab"},{VK_RETURN,L"Enter"},
        {VK_SHIFT,L"Shift"},{VK_CONTROL,L"Ctrl"},{VK_MENU,L"Alt"},
        {VK_CAPITAL,L"CapsLock"},{VK_ESCAPE,L"Esc"},{VK_SPACE,L"Space"},
        {VK_OEM_COMMA,L",<"},{VK_OEM_PERIOD,L".>"},{VK_OEM_MINUS,L"-_"},{VK_OEM_PLUS,L"=+"},
    };
    for (auto& e : table) {
        if (e.vk == vk) { wcscpy_s(buf, bufLen, e.name); return; }
    }
    if (vk >= 'A' && vk <= 'Z') { buf[0] = (wchar_t)vk; buf[1] = L'\0'; return; }
    if (vk >= '0' && vk <= '9') { buf[0] = (wchar_t)vk; buf[1] = L'\0'; return; }
    if (vk >= VK_F1 && vk <= VK_F24) { swprintf(buf, bufLen, L"F%d", vk - VK_F1 + 1); return; }
    swprintf(buf, bufLen, L"VK_%d", vk);
}

HotkeyEditor::~HotkeyEditor() {
    if (m_hFont) DeleteObject(m_hFont);
    if (m_hwnd)  DestroyWindow(m_hwnd);
}

bool HotkeyEditor::Create(HINSTANCE hInst, AppConfig* cfg, HWND notifyHwnd) {
    m_hInst      = hInst;
    m_cfg        = cfg;
    m_notifyHwnd = notifyHwnd;

    // 初始化快捷键条目
    m_entries[0] = {L"打开设置面板",       &m_cfg->hotkeySettingsVK};
    m_entries[1] = {L"打开主题编辑器",     &m_cfg->hotkeyThemeEditorVK};
    m_entries[2] = {L"切换按键映射显示",   &m_cfg->hotkeyToggleDisplayVK};
    m_entries[3] = {L"下一个预设方案",     &m_cfg->hotkeyNextThemeVK};
    m_entries[4] = {L"上一个预设方案",     &m_cfg->hotkeyPrevThemeVK};
    m_entries[5] = {L"切换轨道显示",       &m_cfg->hotkeyToggleTrackVK};
    m_entries[6] = {L"切换图表显示",       &m_cfg->hotkeyToggleChartVK};

    static const wchar_t* CLASS_NAME = L"KeyStateHotkeyEditor";
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    HICON hIcon = LoadIconW(GetModuleHandleW(nullptr), L"IDI_KEYSTATE");
    if (!hIcon) hIcon = LoadIconW(nullptr, (LPCWSTR)IDI_APPLICATION);
    wc.hIcon = hIcon;
    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(0, CLASS_NAME, L"快捷键设置",
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                              CW_USEDEFAULT, CW_USEDEFAULT, 480, 400,
                              nullptr, nullptr, hInst, this);
    return m_hwnd != nullptr;
}

void HotkeyEditor::Show(bool visible) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, visible ? SW_SHOWNORMAL : SW_HIDE);
        if (visible) {
            RefreshKeyButtons();
            SetForegroundWindow(m_hwnd);
        }
    }
}

void HotkeyEditor::OnCreate(HWND hwnd) {
    m_hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    auto ctl = [&](const wchar_t* cls, const wchar_t* text, DWORD style,
                   int x, int y, int w, int h, int id) {
        HWND c = CreateWindowExW(0, cls, text, style | WS_VISIBLE | WS_CHILD,
                                  x, y, w, h, hwnd, (HMENU)(INT_PTR)id, m_hInst, nullptr);
        if (m_hFont) SendMessageW(c, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        return c;
    };
    auto lbl = [&](const wchar_t* text, int x, int y, int w, int h) {
        return ctl(L"STATIC", text, SS_LEFT, x, y, w, h, 0);
    };

    int y = 14;
    int colAction = 20;
    int colKey = 220;

    // 标题行
    lbl(L"动作", colAction, y, 180, 20);
    lbl(L"快捷键 (Ctrl+Shift+)", colKey, y, 220, 20);
    y += 28;

    // 分隔线
    ctl(L"STATIC", L"", SS_ETCHEDHORZ | SS_SUNKEN, 15, y, 440, 2, 0);
    y += 12;

    // 7 行快捷键
    for (int i = 0; i < 7; ++i) {
        lbl(m_entries[i].label, colAction, y, 180, 26);
        wchar_t keyName[64];
        GetVKName(*m_entries[i].vkPtr, keyName, 64);
        wchar_t btnText[128];
        swprintf(btnText, 128, L"Ctrl+Shift+%ls", keyName);
        m_btnKeys[i] = ctl(L"BUTTON", btnText, BS_PUSHBUTTON | WS_TABSTOP,
                           colKey, y, 220, 26, 100 + i);
        y += 34;
    }

    // 关闭按钮
    ctl(L"BUTTON", L"关闭", BS_PUSHBUTTON | WS_TABSTOP, 370, y + 10, 80, 30, 200);
}

void HotkeyEditor::RefreshKeyButtons() {
    for (int i = 0; i < 7; ++i) {
        if (!m_btnKeys[i]) continue;
        wchar_t keyName[64];
        GetVKName(*m_entries[i].vkPtr, keyName, 64);
        wchar_t btnText[128];
        swprintf(btnText, 128, L"Ctrl+Shift+%ls", keyName);
        SetWindowTextW(m_btnKeys[i], btnText);
    }
}

void HotkeyEditor::CaptureHotkey(int idx) {
    if (!m_cfg || idx < 0 || idx >= 7) return;

    // 进入按键捕获模式
    SetWindowTextW(m_btnKeys[idx], L"按任意键… (ESC 取消)");

    int capturedVK = 0;
    MSG msg;
    while (capturedVK == 0 && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            int vk = (int)msg.wParam;
            if (vk == 229) { TranslateMessage(&msg); DispatchMessageW(&msg); continue; }
            capturedVK = vk;
            if (capturedVK == VK_ESCAPE) capturedVK = -1;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (capturedVK <= 0) {
        RefreshKeyButtons();
        return;
    }

    // 更新配置
    *m_entries[idx].vkPtr = capturedVK;

    // 保存配置
    const wchar_t* savePath = L"KeyStateSetting.json";
    // 尝试找到实际配置路径（通过 exe 路径）
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) {
        *(lastSlash + 1) = L'\0';
        wcscat_s(exePath, MAX_PATH, L"KeyStateSetting.json");
        savePath = exePath;
    }
    m_cfg->Save(savePath);

    // 通知主窗口重新注册热键
    if (m_notifyHwnd) {
        PostMessageW(m_notifyHwnd, WM_HOTKEY_CHANGED, 0, 0);
    }

    RefreshKeyButtons();
}

void HotkeyEditor::OnCommand(WPARAM wp) {
    int cid = LOWORD(wp);

    // 关闭按钮
    if (cid == 200) {
        Show(false);
        return;
    }

    // 快捷键按钮 (100-106)
    if (cid >= 100 && cid <= 106) {
        CaptureHotkey(cid - 100);
        return;
    }
}

LRESULT CALLBACK HotkeyEditor::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_CREATE) {
        auto* self = static_cast<HotkeyEditor*>(reinterpret_cast<CREATESTRUCT*>(lp)->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        self->m_hwnd = hwnd;
        self->OnCreate(hwnd);
    }
    auto* self = reinterpret_cast<HotkeyEditor*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);
    switch (msg) {
    case WM_CLOSE: self->Show(false); return 0;
    case WM_COMMAND: self->OnCommand(wp); return 0;
    case WM_DESTROY: self->m_hwnd = nullptr; return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
