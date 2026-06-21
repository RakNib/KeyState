#include "theme_editor.h"
#include "settings_ui.h"
#include <commctrl.h>
#include <cstdio>

static const wchar_t* SW_CLASS = L"KeyStateSwatch";
// 色块控件窗口类复用（已由 SettingsUI 注册）

ThemeEditor::~ThemeEditor() {
    if (m_hFont) DeleteObject(m_hFont);
    if (m_hwnd)  DestroyWindow(m_hwnd);
}

bool ThemeEditor::Create(HINSTANCE hInst, AppConfig* cfg) {
    m_hInst = hInst;
    m_cfg   = cfg;

    static const wchar_t* CLASS_NAME = L"KeyStateThemeEditor";
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance   = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor     = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    HICON hIcon = LoadIconW(GetModuleHandleW(nullptr), L"IDI_KEYSTATE");
    if (!hIcon) hIcon = LoadIconW(nullptr, (LPCWSTR)IDI_APPLICATION);
    wc.hIcon = hIcon;
    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(0, CLASS_NAME, L"Theme Editor",
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                              CW_USEDEFAULT, CW_USEDEFAULT, 360, 380,
                              nullptr, nullptr, hInst, this);
    return m_hwnd != nullptr;
}

void ThemeEditor::Show(bool visible) {
    if (m_hwnd) ShowWindow(m_hwnd, visible ? SW_SHOWNORMAL : SW_HIDE);
    if (visible) SetForegroundWindow(m_hwnd);
}

void ThemeEditor::OnCreate(HWND hwnd) {
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
    auto addSw = [&](int x, int y, int id) {
        return ctl(SW_CLASS, L"", SS_NOTIFY, x, y, kSwW, kSwH, id);
    };

    lbl(L"Theme:", 12, 14, 60, 24);
    m_combo = CreateWindowExW(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_VISIBLE | WS_CHILD,
                               70, 12, 200, 200, hwnd, (HMENU)(INT_PTR)201, m_hInst, nullptr);
    if (m_hFont) SendMessageW(m_combo, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    for (int i = 1; i <= 15; ++i) {
        auto& tp = m_cfg->themePresets[i];
        SendMessageW(m_combo, CB_ADDSTRING, 0, (LPARAM)(tp.name.empty() ? L"?" : tp.name.c_str()));
    }
    SendMessageW(m_combo, CB_SETCURSEL, 0, 0);

    int ys = 50;
    lbl(L"Name:", 12, ys, 70, kSwH);
    m_editName = ctl(L"EDIT", L"", ES_LEFT | WS_BORDER | WS_TABSTOP, 90, ys, 200, kSwH, 300);
    ys += 36;
    lbl(L"Font:", 12, ys, 70, kSwH);   m_swFont   = addSw(90, ys, 301); m_lblFont   = lbl(L"", 132, ys, 200, kSwH);
    ys += 34;
    lbl(L"Normal:", 12, ys, 70, kSwH); m_swNormal = addSw(90, ys, 302); m_lblNormal = lbl(L"", 132, ys, 200, kSwH);
    ys += 34;
    lbl(L"Press:", 12, ys, 70, kSwH);  m_swPress  = addSw(90, ys, 303); m_lblPress  = lbl(L"", 132, ys, 200, kSwH);
    ys += 34;
    lbl(L"Chart BG:", 12, ys, 70, kSwH); m_swBg  = addSw(90, ys, 304); m_lblBg  = lbl(L"", 132, ys, 200, kSwH);
    ys += 34;
    lbl(L"Chart Line:", 12, ys, 70, kSwH); m_swLine = addSw(90, ys, 305); m_lblLine = lbl(L"", 132, ys, 200, kSwH);

    m_btnSave  = ctl(L"BUTTON", L"Save Themes", BS_PUSHBUTTON, 70, ys + 40, 110, 30, 401);
    m_btnClose = ctl(L"BUTTON", L"Close", BS_PUSHBUTTON, 190, ys + 40, 80, 30, 402);

    RefreshDisplay();
}

void ThemeEditor::RefreshDisplay() {
    if (!m_cfg || m_selTheme < 0 || m_selTheme >= (int)m_cfg->themePresets.size()) return;
    auto& tp = m_cfg->themePresets[m_selTheme];
    wchar_t b[64];

    SetWindowTextW(m_editName, tp.name.c_str());

    auto setSw = [&](HWND sw, HWND lbl, RgbaColor& c) {
        delete reinterpret_cast<SwatchData*>(GetWindowLongPtrW(sw, GWLP_USERDATA));
        auto sd = new SwatchData{&c};
        SetWindowLongPtrW(sw, GWLP_USERDATA, (LONG_PTR)sd);
        InvalidateRect(sw, nullptr, TRUE);
        swprintf(b, 64, L"RGB(%d,%d,%d)", c.r, c.g, c.b);
        SetWindowTextW(lbl, b);
    };
    setSw(m_swFont,   m_lblFont,   tp.font);
    setSw(m_swNormal, m_lblNormal, tp.normal);
    setSw(m_swPress,  m_lblPress,  tp.press);
    setSw(m_swBg,     m_lblBg,     tp.chartBg);
    setSw(m_swLine,   m_lblLine,   tp.chartLine);
}

void ThemeEditor::PickAndSet(RgbaColor* col) {
    if (!col) return;
    static COLORREF cust[16] = {};
    CHOOSECOLORW cc = {};
    cc.lStructSize  = sizeof(cc);
    cc.hwndOwner    = m_hwnd;
    cc.rgbResult    = RGB(col->r, col->g, col->b);
    cc.lpCustColors = cust;
    cc.Flags        = CC_RGBINIT | CC_FULLOPEN;
    if (ChooseColorW(&cc)) {
        col->r = GetRValue(cc.rgbResult);
        col->g = GetGValue(cc.rgbResult);
        col->b = GetBValue(cc.rgbResult);
        RefreshDisplay();
    }
}

void ThemeEditor::OnCommand(WPARAM wp) {
    int cid = LOWORD(wp);

    // 切换主题前保存当前主题名称
    if (cid == 201 && HIWORD(wp) == CBN_SELCHANGE) {
        if (m_selTheme >= 1 && m_selTheme < (int)m_cfg->themePresets.size()) {
            wchar_t buf[64];
            GetWindowTextW(m_editName, buf, 64);
            m_cfg->themePresets[m_selTheme].name = buf;
        }
        m_selTheme = (int)SendMessageW(m_combo, CB_GETCURSEL, 0, 0) + 1;
        if (m_selTheme < 1) m_selTheme = 1;
        RefreshDisplay();
        return;
    }
    if (!m_cfg || m_selTheme < 1 || m_selTheme >= (int)m_cfg->themePresets.size()) return;
    auto& tp = m_cfg->themePresets[m_selTheme];

    // 名称修改后同步更新下拉列表
    auto refreshCombo = [&]() {
        wchar_t buf[64];
        GetWindowTextW(m_editName, buf, 64);
        tp.name = buf;
        SendMessageW(m_combo, CB_RESETCONTENT, 0, 0);
        for (int i = 1; i <= 15; ++i)
            SendMessageW(m_combo, CB_ADDSTRING, 0, (LPARAM)m_cfg->themePresets[i].name.c_str());
        SendMessageW(m_combo, CB_SETCURSEL, m_selTheme - 1, 0);
    };

    switch (cid) {
    case 300: refreshCombo(); break;  // Name edit changed (EN_CHANGE)
    case 301: PickAndSet(&tp.font);      break;
    case 302: PickAndSet(&tp.normal);    break;
    case 303: PickAndSet(&tp.press);     break;
    case 304: PickAndSet(&tp.chartBg);   break;
    case 305: PickAndSet(&tp.chartLine); break;
    case 401:
        refreshCombo();
        if (!m_themePath.empty()) m_cfg->SaveThemes(m_themePath.c_str());
        if (m_settings) m_settings->RefreshControls();
        break;
    case 402:
        Show(false);
        break;
    }
}

LRESULT CALLBACK ThemeEditor::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_CREATE) {
        auto* self = static_cast<ThemeEditor*>(reinterpret_cast<CREATESTRUCT*>(lp)->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        self->m_hwnd = hwnd;
        self->OnCreate(hwnd);
    }
    auto* self = reinterpret_cast<ThemeEditor*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);
    switch (msg) {
    case WM_CLOSE: self->Show(false); return 0;
    case WM_COMMAND: self->OnCommand(wp); return 0;
    case WM_DESTROY: self->m_hwnd = nullptr; return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
