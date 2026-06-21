#include "settings_ui.h"
#include "chart_ui.h"
#include "theme_editor.h"
#include "lang.h"

// ========== 控件 ID ==========
enum {
    CID_CHK_TOTAL      = 101,
    CID_CHK_KPS        = 102,
    CID_CHK_SUMMARY    = 116,
    CID_CHK_HISTORY    = 117,
    CID_CHK_THROUGH    = 113,
    CID_TRACK_KEYSIZE  = 114,
    CID_TRACK_HISTORYH = 118,
    CID_EDIT_HISTORYH  = 119,
    CID_TRACK_GROWSPD  = 120,
    CID_EDIT_GROWSPD   = 121,
    CID_TRACK_FLOATSPD = 122,
    CID_EDIT_FLOATSPD  = 123,
    CID_TRACK_BLOCKMAX = 124,
    CID_EDIT_BLOCKMAX  = 125,
    CID_CHK_BPM        = 129,
    CID_RADIO_NOTE8    = 130,
    CID_RADIO_NOTE16   = 131,
    CID_RADIO_NOTE32   = 132,
    CID_RADIO_NOTE64   = 133,
    CID_RADIO_LANG_CN  = 134,
    CID_RADIO_LANG_EN  = 135,
    CID_RADIO_LANG_JP  = 136,
    CID_RADIO_THEME0   = 137,
    CID_RADIO_THEME1   = 138,
    CID_RADIO_THEME2   = 139,
    CID_RADIO_THEME3   = 140,
 
    CID_TRACK_BORDER   = 141,
    CID_EDIT_BORDER    = 142,
    CID_TRACK_OPACITY  = 143,
    CID_EDIT_OPACITY   = 144,
    CID_TRACK_TRACKGAP = 145,
    CID_EDIT_TRACKGAP  = 146,
    CID_TRACK_BPMMERGE = 147,
    CID_EDIT_BPMMERGE  = 148,
    CID_TRACK_TRACKALPHA = 153,
    CID_EDIT_TRACKALPHA  = 154,
    CID_TRACK_BLOCKALPHA = 155,
    CID_EDIT_BLOCKALPHA  = 156,
    CID_SW_THEME_FONT   = 157,
    CID_SW_THEME_NORMAL = 158,
    CID_SW_THEME_PRESS  = 159,
    CID_SW_TOTALBG = 160, CID_SW_TOTALFC = 161,
    CID_SW_KPSBG   = 162, CID_SW_KPSFC   = 163,
    CID_SW_BPMBG   = 164, CID_SW_BPMFC   = 165,
    CID_LBL_TOTALBG=166,CID_LBL_TOTALFC=167,CID_LBL_KPSBG=168,CID_LBL_KPSFC=169,
    CID_LBL_BPMBG=170,CID_LBL_BPMFC=171,
    CID_BTN_CUSTOMSV   = 149,
    CID_LIST_CUSTHEME  = 150,
    CID_BTN_CUSTAPPLY  = 151,
    CID_BTN_CUSTDEL    = 152,
    CID_RADIO_FPS25    = 126,
    CID_RADIO_FPS90    = 147,
    CID_RADIO_FPS120   = 148,
    CID_RADIO_FPS45    = 127,
    CID_RADIO_FPS60    = 128,
    CID_EDIT_KEYSIZE   = 115,
    CID_TRACK_SPACING  = 103,
    CID_EDIT_SPACING   = 104,
    CID_LIST_KEYS      = 105,
    CID_BTN_ADD        = 106,
    CID_BTN_DEL        = 107,
    CID_SW_FONT        = 108,
    CID_SW_NORMAL      = 109,
    CID_SW_PRESS       = 110,
    CID_BTN_SAVE       = 111,
    CID_BTN_RESET_TOTAL= 112,
    CID_BTN_RESET      = 174,
    CID_CHK_CHART      = 175,
    CID_TRACK_CHARTTIME = 176,
    CID_EDIT_CHARTTIME  = 177,
    CID_SW_CHARTBG     = 178,
    CID_SW_CHARTLINE   = 179,
    CID_TRACK_CHARTRADIUS = 180,
    CID_EDIT_CHARTRADIUS  = 181,
    CID_TRACK_CHARTW    = 182,
    CID_EDIT_CHARTW     = 183,
    CID_TRACK_CHARTH    = 184,
    CID_EDIT_CHARTH     = 185,
    CID_TRACK_CHARTML   = 186,
    CID_EDIT_CHARTML    = 187,
    CID_TRACK_CHARTMR   = 188,
    CID_EDIT_CHARTMR    = 189,
    CID_TRACK_CHARTMT   = 190,
    CID_EDIT_CHARTMT    = 191,
    CID_TRACK_CHARTMB   = 192,
    CID_EDIT_CHARTMB    = 193,
    CID_RADIO_THEME4    = 194,
    CID_RADIO_THEME5    = 195,
    CID_RADIO_THEME6    = 196,
    CID_RADIO_THEME7    = 197,
    CID_RADIO_THEME8    = 198,
    CID_RADIO_THEME9    = 199,
    CID_RADIO_THEME10   = 200,
    CID_RADIO_THEME11   = 201,
    CID_RADIO_THEME12   = 202,
    CID_RADIO_THEME13   = 203,
    CID_RADIO_THEME14   = 204,
    CID_RADIO_THEME15   = 205,
    CID_BTN_THEMEEDIT   = 206,
    CID_CHK_CHART_SNAP  = 207,
    CID_TRACK_CHART_SNAPX = 208,
    CID_EDIT_CHART_SNAPX  = 209,
    CID_TRACK_CHART_SNAPY = 210,
    CID_EDIT_CHART_SNAPY  = 211,
    CID_CHK_TOP_MOST      = 212,
    CID_BTN_FONT          = 213,
    CID_CHK_TRACK_LINES   = 214,
    CID_CHK_CHART_GRID    = 215,
};

// 色块控件
static const wchar_t* SWATCH_CLASS = L"KeyStateSwatch";

static LRESULT CALLBACK SwatchProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        auto* sd = reinterpret_cast<SwatchData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (sd && sd->color) {
            HBRUSH hBr = CreateSolidBrush(RGB(sd->color->r, sd->color->g, sd->color->b));
            FillRect(hdc, &rc, hBr);
            DeleteObject(hBr);
        }
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HPEN old = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hollow = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH oldBr = (HBRUSH)SelectObject(hdc, hollow);
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, oldBr); SelectObject(hdc, old);
        DeleteObject(hPen);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONUP: {
        HWND parent = GetParent(hwnd);
        int cid = GetDlgCtrlID(hwnd);
        PostMessageW(parent, WM_COMMAND, MAKEWPARAM(cid, 0), 0);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ========== SettingsUI 实现 ==========

SettingsUI::SettingsUI()  = default;
SettingsUI::~SettingsUI() { if (m_hwnd) DestroyWindow(m_hwnd); if (m_hFont) DeleteObject(m_hFont); }

bool SettingsUI::Create(HINSTANCE hInst, AppConfig* cfg, DisplayUI* display, ChartUI* chart, ThemeEditor* te, KeyStateManager* ksm) {
    m_hInst       = hInst;
    m_cfg         = cfg;
    m_display     = display;
    m_chart       = chart;
    m_themeEditor = te;
    m_ksm         = ksm;

    // 注册色块控件
    WNDCLASSEXW swc = {};
    swc.cbSize        = sizeof(swc);
    swc.lpfnWndProc   = SwatchProc;
    swc.hInstance     = hInst;
    swc.lpszClassName = SWATCH_CLASS;
    swc.hCursor       = LoadCursorW(nullptr, (LPCWSTR)IDC_HAND);
    swc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&swc);

    static const wchar_t* CLASS_NAME = L"KeyStateSettingsV3";
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    HICON hIcon = LoadIconW(GetModuleHandleW(nullptr), L"IDI_KEYSTATE");
    if (!hIcon) hIcon = (HICON)LoadImageW(nullptr, L"favicon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    wc.hIcon         = hIcon ? hIcon : LoadIconW(nullptr, (LPCWSTR)IDI_APPLICATION);
    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(0, CLASS_NAME, LANG(0),
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_VSCROLL,
                              CW_USEDEFAULT, CW_USEDEFAULT, 530, kVisibleH,
                              nullptr, nullptr, hInst, this);
    return m_hwnd != nullptr;
}

void SettingsUI::Show(bool visible) {
    if (!m_hwnd && visible) {
        Create(m_hInst, m_cfg, m_display, m_chart, m_themeEditor, m_ksm);
        if (m_hwnd) RefreshControls();
    }
    if (m_hwnd) {
        ShowWindow(m_hwnd, visible ? SW_SHOWNORMAL : SW_HIDE);
        if (visible) SetForegroundWindow(m_hwnd);
    }
}

// ========== 窗口过程 ==========
LRESULT CALLBACK SettingsUI::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    SettingsUI* self = nullptr;
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lp);
        self = static_cast<SettingsUI*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
        self->OnCreate(hwnd);
    } else {
        self = reinterpret_cast<SettingsUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
    case WM_CLOSE:
        self->Show(false);
        return 0;
    case WM_COMMAND: self->OnCommand(wp, lp); return 0;
    case WM_HSCROLL: self->OnHScroll(wp, lp); return 0;
    case WM_VSCROLL: self->OnVScroll(wp, lp); return 0;
    case WM_MOUSEWHEEL: self->OnMouseWheel(wp); return 0;
    case WM_DESTROY: self->OnDestroy();       return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ========== 创建控件 ==========
void SettingsUI::OnCreate(HWND hwnd) {
    auto ctl = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id) -> HWND {
        HWND c = CreateWindowExW(0, cls, text, style | WS_VISIBLE | WS_CHILD, x, y, w, h, hwnd, (HMENU)(INT_PTR)id, m_hInst, nullptr);
        if (m_hFont) SendMessageW(c, WM_SETFONT, (WPARAM)m_hFont, TRUE); return c;
    };
    auto lbl = [&](const wchar_t* text, int x, int y, int w, int h) { return ctl(L"STATIC", text, SS_LEFT, x, y, w, h, 0); };
    auto sep = [&](int y) { ctl(L"STATIC", L"", SS_ETCHEDHORZ | SS_SUNKEN, 15, y, 500, 2, 0); };
    auto mks = [&](int x, int& y, const wchar_t* l, int trId, int edId, HWND& tr, HWND& ed, int lo, int hi, int v, int tic) {
        lbl(l, x, y, 180, 20); y += 22; tr = ctl(TRACKBAR_CLASSW, L"", TBS_HORZ | TBS_AUTOTICKS | WS_TABSTOP, x, y, 370, 28, trId);
        SendMessageW(tr, TBM_SETRANGE, TRUE, MAKELPARAM(lo, hi)); if (tic) SendMessageW(tr, TBM_SETTICFREQ, tic, 0); SendMessageW(tr, TBM_SETPOS, TRUE, v);
        ed = ctl(L"EDIT", L"", ES_CENTER | ES_NUMBER | WS_BORDER | WS_TABSTOP, x + 380, y, 46, 22, edId); y += 38;
    };
    auto div = [&](int& y) { sep(y); y += 10; };
    int y = 12, CX = 20;

    // 1. Language
    lbl(LANG(24), CX, y, 120, 20); y += 24;
    m_radioLangCN = ctl(L"BUTTON", LANG(25), BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP, CX, y, 60, 24, CID_RADIO_LANG_CN);
    m_radioLangEN = ctl(L"BUTTON", LANG(26), BS_AUTORADIOBUTTON | WS_TABSTOP, CX + 68, y, 65, 24, CID_RADIO_LANG_EN);
    m_radioLangJP = ctl(L"BUTTON", LANG(27), BS_AUTORADIOBUTTON | WS_TABSTOP, CX + 140, y, 65, 24, CID_RADIO_LANG_JP);
    y += 34;
    if (m_cfg) { int l = m_cfg->lang;
        SendMessageW(m_radioLangCN, BM_SETCHECK, (l == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_radioLangEN, BM_SETCHECK, (l == 1) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_radioLangJP, BM_SETCHECK, (l == 2) ? BST_CHECKED : BST_UNCHECKED, 0); }

    // 2. Basic Settings
    lbl(LANG(1), CX, y, 150, 20); y += 26;
    // Row 1: 显示选项
    m_chkTotal = ctl(L"BUTTON", LANG(2), BS_AUTOCHECKBOX | WS_TABSTOP, CX, y, 130, 24, CID_CHK_TOTAL);
    m_chkKPS = ctl(L"BUTTON", LANG(3), BS_AUTOCHECKBOX | WS_TABSTOP, CX + 135, y, 120, 24, CID_CHK_KPS);
    m_chkBPM = ctl(L"BUTTON", LANG(6), BS_AUTOCHECKBOX | WS_TABSTOP, CX + 260, y, 120, 24, CID_CHK_BPM);
    y += 28;
    // Row 2: 汇总 + 穿透 + 顶层置顶
    m_chkSummary = ctl(L"BUTTON", LANG(4), BS_AUTOCHECKBOX | WS_TABSTOP, CX, y, 130, 24, CID_CHK_SUMMARY);
    m_chkThrough = ctl(L"BUTTON", LANG(7), BS_AUTOCHECKBOX | WS_TABSTOP, CX + 135, y, 140, 24, CID_CHK_THROUGH);
    m_chkTopMost = ctl(L"BUTTON", LANG(88), BS_AUTOCHECKBOX | WS_TABSTOP, CX + 280, y, 150, 24, CID_CHK_TOP_MOST);
    y += 28;
    // Row 3: BPM 分音基准
    lbl(L"BPM:", CX, y, 45, 24);
    m_radioNote8 = ctl(L"BUTTON", LANG(37), BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP, CX + 48, y, 50, 24, CID_RADIO_NOTE8);
    m_radioNote16 = ctl(L"BUTTON", LANG(38), BS_AUTORADIOBUTTON | WS_TABSTOP, CX + 103, y, 55, 24, CID_RADIO_NOTE16);
    m_radioNote32 = ctl(L"BUTTON", LANG(39), BS_AUTORADIOBUTTON | WS_TABSTOP, CX + 163, y, 55, 24, CID_RADIO_NOTE32);
    m_radioNote64 = ctl(L"BUTTON", LANG(40), BS_AUTORADIOBUTTON | WS_TABSTOP, CX + 223, y, 55, 24, CID_RADIO_NOTE64);
    y += 30;
    // Row 4: 字体选择
    m_btnFont = ctl(L"BUTTON", L"选择字体", BS_PUSHBUTTON | WS_TABSTOP, CX, y, 100, 24, CID_BTN_FONT);
    { wchar_t fb[64]; swprintf(fb, 64, L"%ls", m_cfg->keyFontName.c_str()); lbl(fb, CX + 108, y, 240, 24); }
    y += 34;
    if (m_cfg) {
        SendMessageW(m_chkTotal, BM_SETCHECK, m_cfg->showTotal ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_chkKPS, BM_SETCHECK, m_cfg->showKPS ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_chkBPM, BM_SETCHECK, m_cfg->showBPM ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_chkSummary, BM_SETCHECK, m_cfg->showSummary ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_chkThrough, BM_SETCHECK, m_cfg->clickThrough ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_chkTopMost, BM_SETCHECK, m_cfg->alwaysOnTop ? BST_CHECKED : BST_UNCHECKED, 0);
        int nd = m_cfg->bpmNoteDiv;
        SendMessageW(m_radioNote8, BM_SETCHECK, (nd == 8) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_radioNote16, BM_SETCHECK, (nd != 8 && nd != 32 && nd != 64) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_radioNote32, BM_SETCHECK, (nd == 32) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_radioNote64, BM_SETCHECK, (nd == 64) ? BST_CHECKED : BST_UNCHECKED, 0); }

    // 3. Theme
    div(y);
    int tidCids[16] = { CID_RADIO_THEME0, CID_RADIO_THEME1, CID_RADIO_THEME2, CID_RADIO_THEME3,
        CID_RADIO_THEME4, CID_RADIO_THEME5, CID_RADIO_THEME6, CID_RADIO_THEME7,
        CID_RADIO_THEME8, CID_RADIO_THEME9, CID_RADIO_THEME10, CID_RADIO_THEME11,
        CID_RADIO_THEME12, CID_RADIO_THEME13, CID_RADIO_THEME14, CID_RADIO_THEME15 };
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            int i = r * 4 + c, lid = (i == 0) ? 46 : ((i <= 3) ? (46 + i) : (70 + i - 4));
            const wchar_t* name = (m_cfg && i < (int)m_cfg->themePresets.size() && !m_cfg->themePresets[i].name.empty())
                ? m_cfg->themePresets[i].name.c_str() : LANG(lid);
            HWND hw = ctl(L"BUTTON", name, BS_AUTORADIOBUTTON | WS_TABSTOP | ((i == 0) ? WS_GROUP : 0), CX + c * 122, y + r * 26, 115, 22, tidCids[i]);
            ((HWND*)&m_radioTheme0)[i] = hw;
        }
    y += 4 * 26 + 8;
    m_btnThemeEdit = ctl(L"BUTTON", LANG(82), BS_PUSHBUTTON | WS_TABSTOP, CX, y, 130, 26, CID_BTN_THEMEEDIT);
    y += 38;
    if (m_cfg) { int th = m_cfg->theme;
        for (int i = 0; i < 16; ++i) SendMessageW(((HWND*)&m_radioTheme0)[i], BM_SETCHECK, (th == i) ? BST_CHECKED : BST_UNCHECKED, 0); }

    // 4. Key Mappings + Layout
    div(y);
    lbl(LANG(15), CX, y, 150, 20); y += 26;
    m_listKeys = ctl(L"LISTBOX", L"", LBS_NOTIFY | WS_VSCROLL | WS_BORDER | WS_TABSTOP, CX, y, 370, 100, CID_LIST_KEYS);
    m_btnAdd = ctl(L"BUTTON", LANG(16), BS_PUSHBUTTON | WS_TABSTOP, 400, y, 90, 30, CID_BTN_ADD);
    m_btnDel = ctl(L"BUTTON", LANG(17), BS_PUSHBUTTON | WS_TABSTOP, 400, y + 36, 90, 30, CID_BTN_DEL);
    RefreshKeyList(); y += 118;
    lbl(LANG(18), CX, y, 360, 20); y += 26;
    lbl(LANG(19), CX, y, 120, 24); m_swFont = ctl(SWATCH_CLASS, L"", SS_NOTIFY, CX + 130, y, 36, 24, CID_SW_FONT); m_lblFont = lbl(L"", CX + 172, y, 200, 24); y += 32;
    lbl(LANG(20), CX, y, 120, 24); m_swNormal = ctl(SWATCH_CLASS, L"", SS_NOTIFY, CX + 130, y, 36, 24, CID_SW_NORMAL); m_lblNormal = lbl(L"", CX + 172, y, 200, 24); y += 32;
    lbl(LANG(21), CX, y, 120, 24); m_swPress = ctl(SWATCH_CLASS, L"", SS_NOTIFY, CX + 130, y, 36, 24, CID_SW_PRESS); m_lblPress = lbl(L"", CX + 172, y, 200, 24); y += 44;
    UpdateColorSwatches(-1);

    mks(CX, y, LANG(8), CID_TRACK_KEYSIZE, CID_EDIT_KEYSIZE, m_trackKeySize, m_editKeySize, 30, 200, m_cfg->keySize, 10); SyncKeySizeEdit();
    mks(CX, y, LANG(9), CID_TRACK_SPACING, CID_EDIT_SPACING, m_trackSpacing, m_editSpacing, 0, 40, m_cfg->keySpacing, 4); SyncSpacingEdit();
    mks(CX, y, LANG(53), CID_TRACK_BORDER, CID_EDIT_BORDER, m_trackBorder, m_editBorder, 0, 8, m_cfg->keyBorderW, 1); SyncBorderEdit();
    mks(CX, y, LANG(54), CID_TRACK_OPACITY, CID_EDIT_OPACITY, m_trackOpacity, m_editOpacity, 10, 100, (int)(m_cfg->overlayOpacity * 100), 10); SyncOpacityEdit();

    // 5. Track + BPM + FPS
    div(y);
    m_chkHistory = ctl(L"BUTTON", LANG(5), BS_AUTOCHECKBOX | WS_TABSTOP, CX, y, 130, 24, CID_CHK_HISTORY);
    m_chkTrackLines = ctl(L"BUTTON", LANG(89), BS_AUTOCHECKBOX | WS_TABSTOP, CX + 140, y, 150, 24, CID_CHK_TRACK_LINES); y += 28;
    if (m_cfg) {
        SendMessageW(m_chkHistory, BM_SETCHECK, m_cfg->showHistory ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_chkTrackLines, BM_SETCHECK, m_cfg->historyShowLines ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    mks(CX, y, LANG(10), CID_TRACK_HISTORYH, CID_EDIT_HISTORYH, m_trackHistoryH, m_editHistoryH, 20, 600, m_cfg->historyTrackH, 10); SyncHistoryHEdit();
    mks(CX, y, LANG(11), CID_TRACK_GROWSPD, CID_EDIT_GROWSPD, m_trackGrowSpd, m_editGrowSpd, 10, 300, m_cfg->historyGrowSpeed, 20); SyncGrowSpdEdit();
    mks(CX, y, LANG(12), CID_TRACK_FLOATSPD, CID_EDIT_FLOATSPD, m_trackFloatSpd, m_editFloatSpd, 10, 300, m_cfg->historyFloatSpeed, 20); SyncFloatSpdEdit();
    mks(CX, y, LANG(41), CID_TRACK_TRACKGAP, CID_EDIT_TRACKGAP, m_trackTrackGap, m_editTrackGap, 0, 30, m_cfg->historyTrackGap, 2); SyncTrackGapEdit();
    mks(CX, y, LANG(43), CID_TRACK_BLOCKALPHA, CID_EDIT_BLOCKALPHA, m_trackBlockAlpha, m_editBlockAlpha, 0, 255, m_cfg->historyBlockAlpha, 32); SyncBlockAlphaEdit();
    mks(CX, y, LANG(44), CID_TRACK_BPMMERGE, CID_EDIT_BPMMERGE, m_trackBpmMerge, m_editBpmMerge, 0, 100, m_cfg->bpmMergeMs, 1); SyncBpmMergeEdit();
    mks(CX, y, LANG(13), CID_TRACK_BLOCKMAX, CID_EDIT_BLOCKMAX, m_trackBlockMax, m_editBlockMax, 10, 100, m_cfg->historyBlockMax, 10); SyncBlockMaxEdit();
    lbl(LANG(14), CX, y, 100, 20); y += 24;
    m_radioFps25 = ctl(L"BUTTON", L"25", BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP, CX, y, 45, 22, CID_RADIO_FPS25);
    m_radioFps45 = ctl(L"BUTTON", L"45", BS_AUTORADIOBUTTON | WS_TABSTOP, CX + 52, y, 45, 22, CID_RADIO_FPS45);
    m_radioFps60 = ctl(L"BUTTON", L"60", BS_AUTORADIOBUTTON | WS_TABSTOP, CX + 104, y, 45, 22, CID_RADIO_FPS60);
    m_radioFps90 = ctl(L"BUTTON", L"90", BS_AUTORADIOBUTTON | WS_TABSTOP, CX + 156, y, 45, 22, CID_RADIO_FPS90);
    m_radioFps120 = ctl(L"BUTTON", L"120", BS_AUTORADIOBUTTON | WS_TABSTOP, CX + 208, y, 45, 22, CID_RADIO_FPS120);
    y += 30;
    if (m_cfg) { int f = m_cfg->fps;
        SendMessageW(m_radioFps25, BM_SETCHECK, (f == 25 || (f != 45 && f != 60 && f != 90 && f != 120)) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_radioFps45, BM_SETCHECK, (f == 45) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_radioFps60, BM_SETCHECK, (f == 60) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_radioFps90, BM_SETCHECK, (f == 90) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_radioFps120, BM_SETCHECK, (f == 120) ? BST_CHECKED : BST_UNCHECKED, 0); }

    // 6. Box Colors
    div(y);
    lbl(LANG(50), CX, y, 150, 20); y += 28;
    auto addSwRow = [&](int ry, const wchar_t* label, int bgId, int fcId, RgbaColor* bg, RgbaColor* fc) {
        lbl(label, CX, ry, 60, 24); m_swBox[bgId - CID_SW_TOTALBG] = ctl(SWATCH_CLASS, L"", SS_NOTIFY, CX + 65, ry, 30, 24, bgId);
        lbl(LANG(51), CX + 100, ry, 26, 24); m_swBox[fcId - CID_SW_TOTALBG] = ctl(SWATCH_CLASS, L"", SS_NOTIFY, CX + 130, ry, 30, 24, fcId);
        lbl(LANG(52), CX + 165, ry, 40, 24);
        auto sd = new SwatchData{bg}; SetWindowLongPtrW(m_swBox[bgId - CID_SW_TOTALBG], GWLP_USERDATA, (LONG_PTR)sd);
        auto sd2 = new SwatchData{fc}; SetWindowLongPtrW(m_swBox[fcId - CID_SW_TOTALBG], GWLP_USERDATA, (LONG_PTR)sd2);
        InvalidateRect(m_swBox[bgId - CID_SW_TOTALBG], nullptr, TRUE); InvalidateRect(m_swBox[fcId - CID_SW_TOTALBG], nullptr, TRUE);
    };
    addSwRow(y, L"Total:", CID_SW_TOTALBG, CID_SW_TOTALFC, &m_cfg->totalBoxBg, &m_cfg->totalBoxFc); y += 30;
    addSwRow(y, L"KPS:", CID_SW_KPSBG, CID_SW_KPSFC, &m_cfg->kpsBoxBg, &m_cfg->kpsBoxFc); y += 30;
    addSwRow(y, L"BPM:", CID_SW_BPMBG, CID_SW_BPMFC, &m_cfg->bpmBoxBg, &m_cfg->bpmBoxFc); y += 38;

    // 7. Chart
    div(y);
    lbl(LANG(58), CX, y, 120, 20); y += 26;
    m_chkChart = ctl(L"BUTTON", LANG(59), BS_AUTOCHECKBOX | WS_TABSTOP, CX, y, 140, 24, CID_CHK_CHART);
    m_chkChartGrid = ctl(L"BUTTON", LANG(90), BS_AUTOCHECKBOX | WS_TABSTOP, CX + 150, y, 140, 24, CID_CHK_CHART_GRID); y += 36;
    if (m_cfg) {
        SendMessageW(m_chkChart, BM_SETCHECK, m_cfg->showChart ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(m_chkChartGrid, BM_SETCHECK, m_cfg->chartShowGrid ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    mks(CX, y, LANG(60), CID_TRACK_CHARTTIME, CID_EDIT_CHARTTIME, m_trackChartTime, m_editChartTime, 1, 30, m_cfg->chartTimeRange / 1000, 1);
    { wchar_t tb[16]; swprintf(tb, 16, L"%d", m_cfg->chartTimeRange / 1000); SetWindowTextW(m_editChartTime, tb); }
    lbl(LANG(61), CX, y, 60, 24); m_swChartBg = ctl(SWATCH_CLASS, L"", SS_NOTIFY, CX + 70, y, 30, 24, CID_SW_CHARTBG); m_lblChartBg = lbl(L"", CX + 106, y, 120, 24);
    lbl(LANG(62), CX + 240, y, 55, 24); m_swChartLine = ctl(SWATCH_CLASS, L"", SS_NOTIFY, CX + 300, y, 30, 24, CID_SW_CHARTLINE); m_lblChartLine = lbl(L"", CX + 336, y, 120, 24);
    y += 34;
    { auto d1 = new SwatchData{&m_cfg->chartBgCol}; SetWindowLongPtrW(m_swChartBg, GWLP_USERDATA, (LONG_PTR)d1); InvalidateRect(m_swChartBg, nullptr, TRUE);
        wchar_t cb[64]; swprintf(cb, 64, L"RGB(%d,%d,%d)", m_cfg->chartBgCol.r, m_cfg->chartBgCol.g, m_cfg->chartBgCol.b); SetWindowTextW(m_lblChartBg, cb);
        auto d2 = new SwatchData{&m_cfg->chartLineCol}; SetWindowLongPtrW(m_swChartLine, GWLP_USERDATA, (LONG_PTR)d2); InvalidateRect(m_swChartLine, nullptr, TRUE);
        swprintf(cb, 64, L"RGB(%d,%d,%d)", m_cfg->chartLineCol.r, m_cfg->chartLineCol.g, m_cfg->chartLineCol.b); SetWindowTextW(m_lblChartLine, cb); }
    mks(CX, y, LANG(64), CID_TRACK_CHARTW, CID_EDIT_CHARTW, m_trackChartW, m_editChartW, 200, 800, m_cfg->chartW, 10);
    { wchar_t tb[16]; swprintf(tb, 16, L"%d", m_cfg->chartW); SetWindowTextW(m_editChartW, tb); }
    mks(CX, y, LANG(65), CID_TRACK_CHARTH, CID_EDIT_CHARTH, m_trackChartH, m_editChartH, 100, 600, m_cfg->chartH, 10);
    { wchar_t tb[16]; swprintf(tb, 16, L"%d", m_cfg->chartH); SetWindowTextW(m_editChartH, tb); }
    mks(CX, y, LANG(66), CID_TRACK_CHARTML, CID_EDIT_CHARTML, m_trackChartML, m_editChartML, 0, 60, m_cfg->chartMarginL, 1);
    { wchar_t tb[16]; swprintf(tb, 16, L"%d", m_cfg->chartMarginL); SetWindowTextW(m_editChartML, tb); }
    mks(CX, y, LANG(67), CID_TRACK_CHARTMR, CID_EDIT_CHARTMR, m_trackChartMR, m_editChartMR, 0, 60, m_cfg->chartMarginR, 1);
    { wchar_t tb[16]; swprintf(tb, 16, L"%d", m_cfg->chartMarginR); SetWindowTextW(m_editChartMR, tb); }
    mks(CX, y, LANG(68), CID_TRACK_CHARTMT, CID_EDIT_CHARTMT, m_trackChartMT, m_editChartMT, 0, 60, m_cfg->chartMarginT, 1);
    { wchar_t tb[16]; swprintf(tb, 16, L"%d", m_cfg->chartMarginT); SetWindowTextW(m_editChartMT, tb); }
    mks(CX, y, LANG(69), CID_TRACK_CHARTMB, CID_EDIT_CHARTMB, m_trackChartMB, m_editChartMB, 0, 60, m_cfg->chartMarginB, 1);
    { wchar_t tb[16]; swprintf(tb, 16, L"%d", m_cfg->chartMarginB); SetWindowTextW(m_editChartMB, tb); }
    mks(CX, y, LANG(63), CID_TRACK_CHARTRADIUS, CID_EDIT_CHARTRADIUS, m_trackChartRadius, m_editChartRadius, 0, 20, m_cfg->chartRadius, 1);
    { wchar_t tb[16]; swprintf(tb, 16, L"%d", m_cfg->chartRadius); SetWindowTextW(m_editChartRadius, tb); }

    // 7.5 吸附
    div(y);
    m_chkChartSnap = ctl(L"BUTTON", L"吸附到按键映射下方", BS_AUTOCHECKBOX | WS_TABSTOP, CX, y, 180, 24, CID_CHK_CHART_SNAP);
    if (m_cfg) SendMessageW(m_chkChartSnap, BM_SETCHECK, m_cfg->chartSnap ? BST_CHECKED : BST_UNCHECKED, 0); y += 28;
    mks(CX, y, L"X 偏移 (px)", CID_TRACK_CHART_SNAPX, CID_EDIT_CHART_SNAPX, m_trackChartSnapX, m_editChartSnapX, -200, 200, m_cfg->chartSnapOffsetX, 50);
    { wchar_t tb[16]; swprintf(tb, 16, L"%d", m_cfg->chartSnapOffsetX); SetWindowTextW(m_editChartSnapX, tb); }
    mks(CX, y, L"Y 偏移 (px)", CID_TRACK_CHART_SNAPY, CID_EDIT_CHART_SNAPY, m_trackChartSnapY, m_editChartSnapY, -200, 200, m_cfg->chartSnapOffsetY, 50);
    { wchar_t tb[16]; swprintf(tb, 16, L"%d", m_cfg->chartSnapOffsetY); SetWindowTextW(m_editChartSnapY, tb); }

    // 8. Save / Reset
    div(y);
    m_btnResetAll = ctl(L"BUTTON", LANG(55), BS_PUSHBUTTON, CX, y, 140, 32, CID_BTN_RESET);
    m_btnSave = ctl(L"BUTTON", LANG(22), BS_PUSHBUTTON | WS_TABSTOP, CX + 160, y, 120, 32, CID_BTN_SAVE);
    m_btnResetTotal = ctl(L"BUTTON", LANG(23), BS_PUSHBUTTON | WS_TABSTOP, CX + 300, y, 120, 32, CID_BTN_RESET_TOTAL);
    y += 44;

    m_contentH = y + 40;
    SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_RANGE | SIF_PAGE; si.nMin = 0; si.nMax = m_contentH - 1; si.nPage = kVisibleH;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

// ========== 命令处理 ==========
void SettingsUI::OnCommand(WPARAM wp, LPARAM lp) {
    int cid = LOWORD(wp);
    int code = HIWORD(wp);

    switch (cid) {
    case CID_CHK_TOTAL:
        if (m_cfg) { m_cfg->showTotal = (SendMessageW(m_chkTotal, BM_GETCHECK, 0, 0) == BST_CHECKED); OnSave(); }
        break;
    case CID_CHK_KPS:
        if (m_cfg) { m_cfg->showKPS = (SendMessageW(m_chkKPS, BM_GETCHECK, 0, 0) == BST_CHECKED); OnSave(); }
        break;
    case CID_CHK_SUMMARY:
        if (m_cfg) { m_cfg->showSummary = (SendMessageW(m_chkSummary, BM_GETCHECK, 0, 0) == BST_CHECKED); OnSave(); }
        break;
    case CID_CHK_HISTORY:
        if (m_cfg) { m_cfg->showHistory = (SendMessageW(m_chkHistory, BM_GETCHECK, 0, 0) == BST_CHECKED); OnSave(); }
        break;
    case CID_CHK_TRACK_LINES:
        if (m_cfg) { m_cfg->historyShowLines = (SendMessageW(m_chkTrackLines, BM_GETCHECK, 0, 0) == BST_CHECKED); OnSave(); }
        break;
    case CID_CHK_BPM:
        if (m_cfg) { m_cfg->showBPM = (SendMessageW(m_chkBPM, BM_GETCHECK, 0, 0) == BST_CHECKED); OnSave(); }
        break;
    case CID_RADIO_NOTE8:
        if (m_cfg && SendMessageW(m_radioNote8, BM_GETCHECK, 0, 0) == BST_CHECKED)
            { m_cfg->bpmNoteDiv = 8; OnSave(); } break;
    case CID_RADIO_NOTE16:
        if (m_cfg && SendMessageW(m_radioNote16, BM_GETCHECK, 0, 0) == BST_CHECKED)
            { m_cfg->bpmNoteDiv = 16; OnSave(); } break;
    case CID_RADIO_NOTE32:
        if (m_cfg && SendMessageW(m_radioNote32, BM_GETCHECK, 0, 0) == BST_CHECKED)
            { m_cfg->bpmNoteDiv = 32; OnSave(); } break;
    case CID_RADIO_NOTE64:
        if (m_cfg && SendMessageW(m_radioNote64, BM_GETCHECK, 0, 0) == BST_CHECKED)
            { m_cfg->bpmNoteDiv = 64; OnSave(); } break;
    case CID_RADIO_LANG_CN:
        if (m_cfg) { m_cfg->lang = 0; SetLanguage(0); OnSave(); RebuildWindow(); } break;
    case CID_RADIO_LANG_EN:
        if (m_cfg) { m_cfg->lang = 1; SetLanguage(1); OnSave(); RebuildWindow(); } break;
    case CID_RADIO_LANG_JP:
        if (m_cfg) { m_cfg->lang = 2; SetLanguage(2); OnSave(); RebuildWindow(); } break;
    case CID_RADIO_THEME0: ApplyTheme(0); break;
    case CID_RADIO_THEME1: ApplyTheme(1); break;
    case CID_RADIO_THEME2: ApplyTheme(2); break;
    case CID_RADIO_THEME3: ApplyTheme(3); break;
    case CID_RADIO_THEME4: ApplyTheme(4); break;
    case CID_RADIO_THEME5: ApplyTheme(5); break;
    case CID_RADIO_THEME6: ApplyTheme(6); break;
    case CID_RADIO_THEME7: ApplyTheme(7); break;
    case CID_RADIO_THEME8:  ApplyTheme(8);  break;
    case CID_RADIO_THEME9:  ApplyTheme(9);  break;
    case CID_RADIO_THEME10: ApplyTheme(10); break;
    case CID_RADIO_THEME11: ApplyTheme(11); break;
    case CID_RADIO_THEME12: ApplyTheme(12); break;
    case CID_RADIO_THEME13: ApplyTheme(13); break;
    case CID_RADIO_THEME14: ApplyTheme(14); break;
    case CID_RADIO_THEME15: ApplyTheme(15); break;
    case CID_CHK_THROUGH:
        if (m_cfg) {
            m_cfg->clickThrough = (SendMessageW(m_chkThrough, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (m_display) m_display->SetClickThrough(m_cfg->clickThrough);
            OnSave();
        }
        break;
    case CID_CHK_TOP_MOST:
        if (m_cfg) {
            m_cfg->alwaysOnTop = (SendMessageW(m_chkTopMost, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (m_display) m_display->SetTopMost(m_cfg->alwaysOnTop);
            if (m_chart)   m_chart->SetTopMost(m_cfg->alwaysOnTop);
            OnSave();
        }
        break;
    case CID_LIST_KEYS:
        if (code == LBN_SELCHANGE) {
            m_selectedKey = (int)SendMessageW(m_listKeys, LB_GETCURSEL, 0, 0);
            if (m_selectedKey >= 0 && m_cfg && m_selectedKey < (int)m_cfg->keys.size())
                UpdateColorSwatches(m_selectedKey);
        }
        break;
    case CID_BTN_ADD:    OnAddKey();   OnSave(); break;
    case CID_BTN_DEL:    OnDeleteKey(); OnSave(); break;
    case CID_BTN_SAVE:   OnSave();     break;
    case CID_BTN_RESET_TOTAL: OnResetTotals(); break;
    case CID_BTN_RESET: OnResetDefaults(); break;
    case CID_BTN_THEMEEDIT: if (m_themeEditor) m_themeEditor->Show(true); break;
    case CID_BTN_FONT: {
        if (!m_cfg) break;
        LOGFONTW lf = {};
        lf.lfHeight = -16;
        wcscpy_s(lf.lfFaceName, LF_FACESIZE, m_cfg->keyFontName.c_str());
        CHOOSEFONTW cf = {sizeof(cf)};
        cf.hwndOwner = m_hwnd;
        cf.lpLogFont = &lf;
        cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOVECTORFONTS;
        if (ChooseFontW(&cf)) {
            m_cfg->keyFontName = lf.lfFaceName;
            OnSave();
            RebuildWindow();
        }
        break;
    }
    case CID_RADIO_FPS25:
        if (m_cfg && SendMessageW(m_radioFps25, BM_GETCHECK, 0, 0) == BST_CHECKED)
            { m_cfg->fps = 25; OnSave(); } break;
    case CID_RADIO_FPS45:
        if (m_cfg && SendMessageW(m_radioFps45, BM_GETCHECK, 0, 0) == BST_CHECKED)
            { m_cfg->fps = 45; OnSave(); } break;
    case CID_RADIO_FPS60:
        if (m_cfg && SendMessageW(m_radioFps60, BM_GETCHECK, 0, 0) == BST_CHECKED)
            { m_cfg->fps = 60; OnSave(); } break;
    case CID_RADIO_FPS90:
        if (m_cfg && SendMessageW(m_radioFps90, BM_GETCHECK, 0, 0) == BST_CHECKED)
            { m_cfg->fps = 90; OnSave(); } break;
    case CID_RADIO_FPS120:
        if (m_cfg && SendMessageW(m_radioFps120, BM_GETCHECK, 0, 0) == BST_CHECKED)
            { m_cfg->fps = 120; OnSave(); } break;
    case CID_SW_FONT:    OnPickColor(0); OnSave(); break;
    case CID_SW_NORMAL:  OnPickColor(1); OnSave(); break;
    case CID_SW_PRESS:   OnPickColor(2); OnSave(); break;
    case CID_SW_TOTALBG: case CID_SW_TOTALFC: case CID_SW_KPSBG: case CID_SW_KPSFC:
    case CID_SW_BPMBG:   case CID_SW_BPMFC:   OnPickBoxColor(cid); break;
    case CID_CHK_CHART:
        if (m_cfg) {
            m_cfg->showChart = (SendMessageW(m_chkChart, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (m_chart) m_chart->Show(m_cfg->showChart);
            OnSave();
        }
        break;
    case CID_CHK_CHART_GRID:
        if (m_cfg) { m_cfg->chartShowGrid = (SendMessageW(m_chkChartGrid, BM_GETCHECK, 0, 0) == BST_CHECKED); OnSave(); }
        break;
    case CID_SW_CHARTBG:  case CID_SW_CHARTLINE:  OnPickChartColor(cid); break;
    case CID_CHK_CHART_SNAP:
        if (m_cfg) {
            m_cfg->chartSnap = (SendMessageW(m_chkChartSnap, BM_GETCHECK, 0, 0) == BST_CHECKED);
            OnSave();
        }
        break;
    }
}

// ========== 滑块处理 ==========
void SettingsUI::OnHScroll(WPARAM wp, LPARAM lp) {
    HWND hwndTrack = (HWND)lp;
    if (hwndTrack == m_trackKeySize && m_cfg) {
        m_cfg->keySize = (int)SendMessageW(m_trackKeySize, TBM_GETPOS, 0, 0);
        SyncKeySizeEdit();
        OnSave();
    } else if (hwndTrack == m_trackSpacing && m_cfg) {
        m_cfg->keySpacing = (int)SendMessageW(m_trackSpacing, TBM_GETPOS, 0, 0);
        SyncSpacingEdit();
        OnSave();
    } else if (hwndTrack == m_trackHistoryH && m_cfg) {
        m_cfg->historyTrackH = (int)SendMessageW(m_trackHistoryH, TBM_GETPOS, 0, 0);
        SyncHistoryHEdit();
        OnSave();
    } else if (hwndTrack == m_trackGrowSpd && m_cfg) {
        m_cfg->historyGrowSpeed = (int)SendMessageW(m_trackGrowSpd, TBM_GETPOS, 0, 0);
        SyncGrowSpdEdit(); OnSave();
    } else if (hwndTrack == m_trackFloatSpd && m_cfg) {
        m_cfg->historyFloatSpeed = (int)SendMessageW(m_trackFloatSpd, TBM_GETPOS, 0, 0);
        SyncFloatSpdEdit(); OnSave();
    } else if (hwndTrack == m_trackBlockMax && m_cfg) {
        m_cfg->historyBlockMax = (int)SendMessageW(m_trackBlockMax, TBM_GETPOS, 0, 0);
        SyncBlockMaxEdit(); OnSave();
    } else if (hwndTrack == m_trackBorder && m_cfg) {
        m_cfg->keyBorderW = (int)SendMessageW(m_trackBorder, TBM_GETPOS, 0, 0);
        SyncBorderEdit(); OnSave();
    } else if (hwndTrack == m_trackOpacity && m_cfg) {
        m_cfg->overlayOpacity = SendMessageW(m_trackOpacity, TBM_GETPOS, 0, 0) / 100.0f;
        SyncOpacityEdit(); OnSave();
    } else if (hwndTrack == m_trackTrackGap && m_cfg) {
        m_cfg->historyTrackGap = (int)SendMessageW(m_trackTrackGap, TBM_GETPOS, 0, 0);
        SyncTrackGapEdit(); OnSave();
    } else if (hwndTrack == m_trackBpmMerge && m_cfg) {
        m_cfg->bpmMergeMs = (int)SendMessageW(m_trackBpmMerge, TBM_GETPOS, 0, 0);
        SyncBpmMergeEdit(); OnSave();
    } else if (hwndTrack == m_trackTrackAlpha && m_cfg) {
        m_cfg->historyTrackAlpha = (int)SendMessageW(m_trackTrackAlpha, TBM_GETPOS, 0, 0);
        SyncTrackAlphaEdit(); OnSave();
    } else if (hwndTrack == m_trackBlockAlpha && m_cfg) {
        m_cfg->historyBlockAlpha = (int)SendMessageW(m_trackBlockAlpha, TBM_GETPOS, 0, 0);
        SyncBlockAlphaEdit(); OnSave();
    } else if (hwndTrack == m_trackChartTime && m_cfg) {
        int val = (int)SendMessageW(m_trackChartTime, TBM_GETPOS, 0, 0);
        if (val < 1) val = 1;
        m_cfg->chartTimeRange = val * 1000;
        wchar_t tb[16]; swprintf(tb,16,L"%d",val); SetWindowTextW(m_editChartTime,tb);
        OnSave();
    } else if (hwndTrack == m_trackChartRadius && m_cfg) {
        m_cfg->chartRadius = (int)SendMessageW(m_trackChartRadius, TBM_GETPOS, 0, 0);
        wchar_t rd[16]; swprintf(rd,16,L"%d",m_cfg->chartRadius); SetWindowTextW(m_editChartRadius,rd);
        OnSave();
    } else if (hwndTrack == m_trackChartW && m_cfg) {
        m_cfg->chartW = (int)SendMessageW(m_trackChartW, TBM_GETPOS, 0, 0);
        wchar_t wb[16]; swprintf(wb,16,L"%d",m_cfg->chartW); SetWindowTextW(m_editChartW,wb);
        OnSave();
    } else if (hwndTrack == m_trackChartH && m_cfg) {
        m_cfg->chartH = (int)SendMessageW(m_trackChartH, TBM_GETPOS, 0, 0);
        wchar_t hb[16]; swprintf(hb,16,L"%d",m_cfg->chartH); SetWindowTextW(m_editChartH,hb);
        OnSave();
    } else if (hwndTrack == m_trackChartML && m_cfg) {
        m_cfg->chartMarginL = (int)SendMessageW(m_trackChartML, TBM_GETPOS, 0, 0);
        wchar_t b[16]; swprintf(b,16,L"%d",m_cfg->chartMarginL); SetWindowTextW(m_editChartML,b); OnSave();
    } else if (hwndTrack == m_trackChartMR && m_cfg) {
        m_cfg->chartMarginR = (int)SendMessageW(m_trackChartMR, TBM_GETPOS, 0, 0);
        wchar_t b[16]; swprintf(b,16,L"%d",m_cfg->chartMarginR); SetWindowTextW(m_editChartMR,b); OnSave();
    } else if (hwndTrack == m_trackChartMT && m_cfg) {
        m_cfg->chartMarginT = (int)SendMessageW(m_trackChartMT, TBM_GETPOS, 0, 0);
        wchar_t b[16]; swprintf(b,16,L"%d",m_cfg->chartMarginT); SetWindowTextW(m_editChartMT,b); OnSave();
    } else if (hwndTrack == m_trackChartMB && m_cfg) {
        m_cfg->chartMarginB = (int)SendMessageW(m_trackChartMB, TBM_GETPOS, 0, 0);
        wchar_t b[16]; swprintf(b,16,L"%d",m_cfg->chartMarginB); SetWindowTextW(m_editChartMB,b); OnSave();
    } else if (hwndTrack == m_trackChartSnapX && m_cfg) {
        m_cfg->chartSnapOffsetX = (int)SendMessageW(m_trackChartSnapX, TBM_GETPOS, 0, 0);
        wchar_t tb[16]; swprintf(tb,16,L"%d",m_cfg->chartSnapOffsetX); SetWindowTextW(m_editChartSnapX,tb);
        OnSave();
    } else if (hwndTrack == m_trackChartSnapY && m_cfg) {
        m_cfg->chartSnapOffsetY = (int)SendMessageW(m_trackChartSnapY, TBM_GETPOS, 0, 0);
        wchar_t tb[16]; swprintf(tb,16,L"%d",m_cfg->chartSnapOffsetY); SetWindowTextW(m_editChartSnapY,tb);
        OnSave();
    }
}

// ========== 滚动条处理 ==========
void SettingsUI::OnVScroll(WPARAM wp, LPARAM lp) {
    int action = LOWORD(wp);
    int newY   = m_scrollY;
    switch (action) {
    case SB_LINEUP:        newY -= 30; break;
    case SB_LINEDOWN:      newY += 30; break;
    case SB_PAGEUP:        newY -= kVisibleH; break;
    case SB_PAGEDOWN:      newY += kVisibleH; break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: newY = HIWORD(wp); break;
    }
    if (newY < 0) newY = 0;
    if (newY > m_contentH - (int)kVisibleH) newY = m_contentH - kVisibleH;
    if (newY == m_scrollY) return;

    int dy = m_scrollY - newY;
    m_scrollY = newY;
    // 手动偏移所有子控件位置（避免 ScrollWindowEx 导致的绘制错位）
    for (HWND child = GetWindow(m_hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        RECT rc; GetWindowRect(child, &rc);
        POINT pt = {rc.left, rc.top};
        ScreenToClient(m_hwnd, &pt);
        SetWindowPos(child, nullptr, pt.x, pt.y + dy, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    SetScrollPos(m_hwnd, SB_VERT, m_scrollY, TRUE);
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void SettingsUI::OnMouseWheel(WPARAM wp) {
    int delta = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
    int newY = m_scrollY - delta * 30;
    if (newY < 0) newY = 0;
    if (newY > m_contentH - (int)kVisibleH) newY = m_contentH - kVisibleH;
    if (newY == m_scrollY) return;

    int dy = m_scrollY - newY;
    m_scrollY = newY;
    for (HWND child = GetWindow(m_hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        RECT rc; GetWindowRect(child, &rc);
        POINT pt = {rc.left, rc.top};
        ScreenToClient(m_hwnd, &pt);
        SetWindowPos(child, nullptr, pt.x, pt.y + dy, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    SetScrollPos(m_hwnd, SB_VERT, m_scrollY, TRUE);
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void SettingsUI::SyncSpacingEdit() {
    if (m_cfg && m_editSpacing) {
        wchar_t buf[16];
        swprintf(buf, 16, L"%d", m_cfg->keySpacing);
        SetWindowTextW(m_editSpacing, buf);
    }
}

void SettingsUI::SyncKeySizeEdit() {
    if (m_cfg && m_editKeySize) {
        wchar_t buf[16];
        swprintf(buf, 16, L"%d", m_cfg->keySize);
        SetWindowTextW(m_editKeySize, buf);
    }
}

void SettingsUI::SyncHistoryHEdit() {
    if (m_cfg && m_editHistoryH) {
        wchar_t buf[16];
        swprintf(buf, 16, L"%d", m_cfg->historyTrackH);
        SetWindowTextW(m_editHistoryH, buf);
    }
}

void SettingsUI::SyncGrowSpdEdit() {
    if (m_cfg && m_editGrowSpd) {
        wchar_t buf[16];
        swprintf(buf, 16, L"%d", m_cfg->historyGrowSpeed);
        SetWindowTextW(m_editGrowSpd, buf);
    }
}

void SettingsUI::SyncFloatSpdEdit() {
    if (m_cfg && m_editFloatSpd) {
        wchar_t buf[16];
        swprintf(buf, 16, L"%d", m_cfg->historyFloatSpeed);
        SetWindowTextW(m_editFloatSpd, buf);
    }
}

void SettingsUI::SyncBlockMaxEdit() {
    if (m_cfg && m_editBlockMax) {
        wchar_t buf[16];
        swprintf(buf, 16, L"%d", m_cfg->historyBlockMax);
        SetWindowTextW(m_editBlockMax, buf);
    }
}

void SettingsUI::SyncBorderEdit() {
    if (m_cfg && m_editBorder) {
        wchar_t buf[16];
        swprintf(buf, 16, L"%d", m_cfg->keyBorderW);
        SetWindowTextW(m_editBorder, buf);
    }
}

void SettingsUI::SyncOpacityEdit() {
    if (m_cfg && m_editOpacity) {
        wchar_t buf[16];
        swprintf(buf, 16, L"%d", (int)(m_cfg->overlayOpacity * 100));
        SetWindowTextW(m_editOpacity, buf);
    }
}

void SettingsUI::SyncTrackGapEdit() {
    if (m_cfg && m_editTrackGap) {
        wchar_t buf[16];
        swprintf(buf, 16, L"%d", m_cfg->historyTrackGap);
        SetWindowTextW(m_editTrackGap, buf);
    }
}

void SettingsUI::SyncBpmMergeEdit() {
    if (m_cfg && m_editBpmMerge) {
        wchar_t buf[16];
        swprintf(buf, 16, L"%d", m_cfg->bpmMergeMs);
        SetWindowTextW(m_editBpmMerge, buf);
    }
}

void SettingsUI::SyncTrackAlphaEdit() {
    if (m_cfg && m_editTrackAlpha) {
        wchar_t buf[16]; swprintf(buf,16,L"%d",m_cfg->historyTrackAlpha);
        SetWindowTextW(m_editTrackAlpha,buf);
    }
}

void SettingsUI::SyncBlockAlphaEdit() {
    if (m_cfg && m_editBlockAlpha) {
        wchar_t buf[16]; swprintf(buf,16,L"%d",m_cfg->historyBlockAlpha);
        SetWindowTextW(m_editBlockAlpha,buf);
    }
}

// ========== 按键列表刷新 ==========
void SettingsUI::RefreshKeyList() {
    if (!m_listKeys || !m_cfg) return;
    SendMessageW(m_listKeys, LB_RESETCONTENT, 0, 0);
    for (auto& kc : m_cfg->keys) {
        wchar_t buf[128];
        swprintf(buf, 128, L"%s  (VK:%d)", kc.label.c_str(), kc.keyCode);
        SendMessageW(m_listKeys, LB_ADDSTRING, 0, (LPARAM)buf);
    }
    m_selectedKey = -1;
    UpdateColorSwatches(-1);
}

// ========== 色块更新 ==========
void SettingsUI::UpdateColorSwatches(int selIdx) {
    if (!m_cfg) return;
    wchar_t buf[64];

    if (selIdx >= 0 && selIdx < (int)m_cfg->keys.size()) {
        auto& kc = m_cfg->keys[selIdx];
        auto setSw = [&](HWND sw, RgbaColor* c) {
            // 清理旧数据
            auto* old = reinterpret_cast<SwatchData*>(GetWindowLongPtrW(sw, GWLP_USERDATA));
            delete old;
            auto* sd = new SwatchData{c};
            SetWindowLongPtrW(sw, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(sd));
            InvalidateRect(sw, nullptr, TRUE);
        };
        setSw(m_swFont,   &kc.colorFont);
        setSw(m_swNormal, &kc.colorNormal);
        setSw(m_swPress,  &kc.colorPress);

        swprintf(buf, 64, L"RGB(%d,%d,%d)", kc.colorFont.r, kc.colorFont.g, kc.colorFont.b);
        SetWindowTextW(m_lblFont, buf);
        swprintf(buf, 64, L"RGB(%d,%d,%d)", kc.colorNormal.r, kc.colorNormal.g, kc.colorNormal.b);
        SetWindowTextW(m_lblNormal, buf);
        swprintf(buf, 64, L"RGB(%d,%d,%d)", kc.colorPress.r, kc.colorPress.g, kc.colorPress.b);
        SetWindowTextW(m_lblPress, buf);
    } else {
        auto setEmpty = [&](HWND sw, HWND lbl) {
            delete reinterpret_cast<SwatchData*>(GetWindowLongPtrW(sw, GWLP_USERDATA));
            SetWindowLongPtrW(sw, GWLP_USERDATA, 0);
            InvalidateRect(sw, nullptr, TRUE);
            SetWindowTextW(lbl, L"-");
        };
        setEmpty(m_swFont,   m_lblFont);
        setEmpty(m_swNormal, m_lblNormal);
        setEmpty(m_swPress,  m_lblPress);
    }
}

// ========== 添加 / 删除 按键 ==========
void SettingsUI::OnAddKey() {
    if (!m_cfg) return;

    HWND capWnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"STATIC", LANG(28),
        WS_POPUP | WS_CAPTION | SS_CENTER | WS_VISIBLE,
        400, 300, 320, 80, m_hwnd, nullptr, m_hInst, nullptr);
    if (!capWnd) return;

    // 暂停全局钩子，避免按键被两边同时处理
    KeyboardHook::SetBlocked(true);

    int capturedVK = 0;
    MSG msg;
    while (capturedVK == 0 && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            int vk = (int)msg.wParam;
            // 跳过 IME 虚拟键和无效键
            if (vk == 229) { TranslateMessage(&msg); DispatchMessageW(&msg); continue; }
            capturedVK = vk;
            if (capturedVK == VK_ESCAPE) capturedVK = -1;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    DestroyWindow(capWnd);

    // 恢复全局钩子
    KeyboardHook::SetBlocked(false);

    if (capturedVK <= 0) return;

    // ===== 获取按键名（多层回退） =====
    wchar_t keyName[64] = {};

    // 第 1 层：GetKeyNameTextW（标准方式）
    UINT scanCode = MapVirtualKeyW(capturedVK, MAPVK_VK_TO_VSC);
    bool isExtended = false;
    switch (capturedVK) {
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
    case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
    case VK_INSERT: case VK_DELETE: case VK_DIVIDE:
    case VK_NUMLOCK:
        isExtended = true; break;
    }
    LONG lParam = (scanCode << 16);
    if (isExtended) lParam |= 0x01000000;
    int nameLen = GetKeyNameTextW(lParam, keyName, 64);

    // 第 2 层：MapVirtualKeyW(MAPVK_VK_TO_CHAR) 获取字符
    if (nameLen == 0) {
        UINT ch = MapVirtualKeyW(capturedVK, MAPVK_VK_TO_CHAR);
        if (ch > 0) { keyName[0] = (wchar_t)ch; keyName[1] = L'\0'; nameLen = 1; }
    }

    // 第 3 层：ToUnicode（可打印字符）
    if (nameLen == 0) {
        BYTE state[256] = {};
        wchar_t ch = 0;
        GetKeyboardState(state);
        if (ToUnicode(capturedVK, scanCode, state, &ch, 1, 0) >= 1 && ch >= 0x20) {
            keyName[0] = ch; keyName[1] = L'\0'; nameLen = 1;
        }
    }

    // 第 4 层：已知虚拟键名表
    if (nameLen == 0) {
        struct VKName { int vk; const wchar_t* n; };
        static const VKName table[] = {
            {VK_LBUTTON,L"LBtn"},{VK_RBUTTON,L"RBtn"},{VK_MBUTTON,L"MBtn"},
            {VK_BACK,L"Backspace"},{VK_TAB,L"Tab"},{VK_RETURN,L"Enter"},
            {VK_SHIFT,L"Shift"},{VK_CONTROL,L"Ctrl"},{VK_MENU,L"Alt"},
            {VK_PAUSE,L"Pause"},{VK_CAPITAL,L"CapsLock"},{VK_ESCAPE,L"Esc"},
            {VK_SPACE,L"Space"},
            {VK_PRIOR,L"PgUp"},{VK_NEXT,L"PgDn"},{VK_END,L"End"},{VK_HOME,L"Home"},
            {VK_LEFT,L"←"},{VK_UP,L"↑"},{VK_RIGHT,L"→"},{VK_DOWN,L"↓"},
            {VK_SNAPSHOT,L"PrtSc"},{VK_INSERT,L"Ins"},{VK_DELETE,L"Del"},
            {VK_LWIN,L"LWin"},{VK_RWIN,L"RWin"},{VK_APPS,L"Menu"},
            {VK_NUMPAD0,L"Num0"},{VK_NUMPAD1,L"Num1"},{VK_NUMPAD2,L"Num2"},
            {VK_NUMPAD3,L"Num3"},{VK_NUMPAD4,L"Num4"},{VK_NUMPAD5,L"Num5"},
            {VK_NUMPAD6,L"Num6"},{VK_NUMPAD7,L"Num7"},{VK_NUMPAD8,L"Num8"},
            {VK_NUMPAD9,L"Num9"},{VK_MULTIPLY,L"Num*"},{VK_ADD,L"Num+"},
            {VK_SUBTRACT,L"Num-"},{VK_DECIMAL,L"Num."},{VK_DIVIDE,L"Num/"},
            {VK_NUMLOCK,L"NumLock"},{VK_SCROLL,L"ScrLk"},
            {VK_LSHIFT,L"LShift"},{VK_RSHIFT,L"RShift"},
            {VK_LCONTROL,L"LCtrl"},{VK_RCONTROL,L"RCtrl"},
            {VK_LMENU,L"LAlt"},{VK_RMENU,L"RAlt"},
            {VK_OEM_1,L";:"},{VK_OEM_2,L"/?"},{VK_OEM_3,L"`~"},
            {VK_OEM_4,L"[{"},{VK_OEM_5,L"\\|"},{VK_OEM_6,L"]}"},
            {VK_OEM_7,L"'\"'"},{VK_OEM_COMMA,L",<"},{VK_OEM_PERIOD,L".>"},
            {VK_OEM_MINUS,L"-_"},{VK_OEM_PLUS,L"=+"},
        };
        for (auto& e : table) {
            if (e.vk == capturedVK) { wcscpy_s(keyName, 64, e.n); nameLen = (int)wcslen(keyName); break; }
        }
    }

    // 第 5 层：F1-F24
    if (nameLen == 0 && capturedVK >= VK_F1 && capturedVK <= VK_F24) {
        swprintf(keyName, 64, L"F%d", capturedVK - VK_F1 + 1);
        nameLen = (int)wcslen(keyName);
    }

    // 最后回退
    if (nameLen == 0) {
        swprintf(keyName, 64, L"VK_%d", capturedVK);
    }

    // 检查重复
    for (auto& kc : m_cfg->keys) {
        if (kc.keyCode == capturedVK) {
            MessageBoxW(m_hwnd, LANG(29), LANG(30), MB_ICONWARNING);
            return;
        }
    }

    KeyConfig kc;
    kc.keyCode     = capturedVK;
    kc.label       = keyName;
    kc.colorFont   = {235, 235, 245, 255};
    kc.colorNormal = {48,  48,  48,  200};
    kc.colorPress  = {255, 95,  95,  255};
    m_cfg->keys.push_back(kc);
    RefreshKeyList();
}

void SettingsUI::OnDeleteKey() {
    if (!m_cfg) return;
    int sel = (int)SendMessageW(m_listKeys, LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)m_cfg->keys.size()) {
        MessageBoxW(m_hwnd, LANG(31), LANG(32), MB_ICONINFORMATION);
        return;
    }
    if (m_cfg->keys.size() <= 1) {
        MessageBoxW(m_hwnd, LANG(33), LANG(34), MB_ICONWARNING);
        return;
    }
    m_cfg->keys.erase(m_cfg->keys.begin() + sel);
    RefreshKeyList();
}

// ========== Box 颜色选取 ==========
void SettingsUI::OnPickBoxColor(int cid) {
    if (!m_cfg) return;
    RgbaColor* p = nullptr;
    int swIdx = cid - CID_SW_TOTALBG;
    switch(cid) {
        case CID_SW_TOTALBG: p=&m_cfg->totalBoxBg; break;
        case CID_SW_TOTALFC: p=&m_cfg->totalBoxFc; break;
        case CID_SW_KPSBG:   p=&m_cfg->kpsBoxBg;   break;
        case CID_SW_KPSFC:   p=&m_cfg->kpsBoxFc;   break;
        case CID_SW_BPMBG:   p=&m_cfg->bpmBoxBg;   break;
        case CID_SW_BPMFC:   p=&m_cfg->bpmBoxFc;   break;
    }
    if(!p) return;
    static COLORREF cust[16]={};
    CHOOSECOLORW cc={}; cc.lStructSize=sizeof(cc); cc.hwndOwner=m_hwnd;
    cc.rgbResult=RGB(p->r,p->g,p->b); cc.lpCustColors=cust; cc.Flags=CC_RGBINIT|CC_FULLOPEN;
    if(ChooseColorW(&cc)){
        p->r=GetRValue(cc.rgbResult); p->g=GetGValue(cc.rgbResult); p->b=GetBValue(cc.rgbResult);
        if(swIdx>=0&&swIdx<6){ InvalidateRect(m_swBox[swIdx],nullptr,TRUE); }
        OnSave();
    }
}

// ========== Chart 颜色选取 ==========
void SettingsUI::OnPickChartColor(int cid) {
    if (!m_cfg) return;
    RgbaColor* p = nullptr;
    HWND sw = nullptr, lbl = nullptr;
    if (cid == CID_SW_CHARTBG)  { p = &m_cfg->chartBgCol;   sw = m_swChartBg;   lbl = m_lblChartBg; }
    if (cid == CID_SW_CHARTLINE){ p = &m_cfg->chartLineCol; sw = m_swChartLine; lbl = m_lblChartLine; }
    if (!p) return;
    static COLORREF cust[16]={};
    CHOOSECOLORW cc={}; cc.lStructSize=sizeof(cc); cc.hwndOwner=m_hwnd;
    cc.rgbResult=RGB(p->r,p->g,p->b); cc.lpCustColors=cust; cc.Flags=CC_RGBINIT|CC_FULLOPEN;
    if (ChooseColorW(&cc)) {
        p->r=GetRValue(cc.rgbResult); p->g=GetGValue(cc.rgbResult); p->b=GetBValue(cc.rgbResult);
        InvalidateRect(sw,nullptr,TRUE);
        wchar_t cb[64]; swprintf(cb,64,L"RGB(%d,%d,%d)",p->r,p->g,p->b); SetWindowTextW(lbl,cb);
        OnSave();
    }
}

// ========== 颜色选取 ==========
void SettingsUI::OnPickColor(int target) {
    if (!m_cfg) return;
    int sel = (int)SendMessageW(m_listKeys, LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)m_cfg->keys.size()) return;

    auto& kc = m_cfg->keys[sel];
    RgbaColor* pColor = nullptr;
    switch (target) {
    case 0: pColor = &kc.colorFont;   break;
    case 1: pColor = &kc.colorNormal; break;
    case 2: pColor = &kc.colorPress;  break;
    }
    if (!pColor) return;

    static COLORREF custom[16] = {};
    CHOOSECOLORW cc = {};
    cc.lStructSize  = sizeof(cc);
    cc.hwndOwner    = m_hwnd;
    cc.rgbResult    = RGB(pColor->r, pColor->g, pColor->b);
    cc.lpCustColors = custom;
    cc.Flags        = CC_RGBINIT | CC_FULLOPEN;

    if (ChooseColorW(&cc)) {
        pColor->r = GetRValue(cc.rgbResult);
        pColor->g = GetGValue(cc.rgbResult);
        pColor->b = GetBValue(cc.rgbResult);
        UpdateColorSwatches(sel);
    }
}

// ========== 重置所有 Total ==========
void SettingsUI::OnResetTotals() {
    if (!m_cfg) return;
    for (auto& kc : m_cfg->keys) kc.totalPresses = 0;
    if (m_ksm) m_ksm->ResetTotals();
    OnSave();
    MessageBoxW(m_hwnd, LANG(35), LANG(36), MB_ICONINFORMATION);
}

// ========== 重置默认 ==========
void SettingsUI::OnResetDefaults() {
    if (!m_cfg) return;
    if (MessageBoxW(m_hwnd, LANG(56), LANG(57), MB_YESNO|MB_ICONWARNING) != IDYES) return;
    bool chartWasVisible = m_cfg->showChart;
    *m_cfg = AppConfig{};
    m_cfg->keys.push_back({32, L"Space", {235,235,245,255}, {48,48,48,200}, {255,95,95,255}});
    m_cfg->keys.push_back({65, L"A",     {235,235,245,255}, {48,48,48,200}, {95,255,95,255}});
    m_cfg->keys.push_back({83, L"S",     {235,235,245,255}, {48,48,48,200}, {95,130,255,255}});
    m_cfg->keys.push_back({68, L"D",     {235,235,245,255}, {48,48,48,200}, {255,210,60,255}});
    // 同步关闭图表窗口，保持状态一致
    if (m_chart && chartWasVisible) m_chart->Show(false);
    m_cfg->Save(m_configPath.empty()?L"KeyStateSetting.json":m_configPath.c_str());
    RebuildWindow();
}

// ========== 保存 ==========
void SettingsUI::OnSave() {
    if (!m_cfg) return;

    // 同步编辑框 → 滑条
    wchar_t buf[16];

    GetWindowTextW(m_editKeySize, buf, 16);
    int ks = _wtoi(buf);
    if (ks >= 30 && ks <= 200) {
        m_cfg->keySize = ks;
        SendMessageW(m_trackKeySize, TBM_SETPOS, TRUE, ks);
    }

    GetWindowTextW(m_editSpacing, buf, 16);
    int sp = _wtoi(buf);
    if (sp >= 0 && sp <= 40) {
        m_cfg->keySpacing = sp;
        SendMessageW(m_trackSpacing, TBM_SETPOS, TRUE, sp);
    }

    GetWindowTextW(m_editHistoryH, buf, 16);
    int hh = _wtoi(buf);
    if (hh >= 20 && hh <= 600) { m_cfg->historyTrackH = hh; SendMessageW(m_trackHistoryH, TBM_SETPOS, TRUE, hh); }

    GetWindowTextW(m_editGrowSpd, buf, 16);
    int gs = _wtoi(buf);
    if (gs >= 10 && gs <= 300) { m_cfg->historyGrowSpeed = gs; SendMessageW(m_trackGrowSpd, TBM_SETPOS, TRUE, gs); }

    GetWindowTextW(m_editFloatSpd, buf, 16);
    int fs = _wtoi(buf);
    if (fs >= 10 && fs <= 300) { m_cfg->historyFloatSpeed = fs; SendMessageW(m_trackFloatSpd, TBM_SETPOS, TRUE, fs); }

    GetWindowTextW(m_editBlockMax, buf, 16);
    int bm = _wtoi(buf);
    if (bm >= 10 && bm <= 100) { m_cfg->historyBlockMax = bm; SendMessageW(m_trackBlockMax, TBM_SETPOS, TRUE, bm); }

    GetWindowTextW(m_editBorder, buf, 16);
    int bw = _wtoi(buf);
    if (bw >= 0 && bw <= 8) { m_cfg->keyBorderW = bw; SendMessageW(m_trackBorder, TBM_SETPOS, TRUE, bw); }

    GetWindowTextW(m_editOpacity, buf, 16);
    int op = _wtoi(buf);
    if (op >= 10 && op <= 100) { m_cfg->overlayOpacity = op / 100.0f; SendMessageW(m_trackOpacity, TBM_SETPOS, TRUE, op); }

    GetWindowTextW(m_editBpmMerge, buf, 16);
    int bp = _wtoi(buf);
    if (bp >= 0 && bp <= 100) { m_cfg->bpmMergeMs = bp; SendMessageW(m_trackBpmMerge, TBM_SETPOS, TRUE, bp); }

    GetWindowTextW(m_editTrackAlpha, buf, 16);
    int ta = _wtoi(buf);
    if (ta >= 0 && ta <= 100) { m_cfg->historyTrackAlpha = ta; SendMessageW(m_trackTrackAlpha, TBM_SETPOS, TRUE, ta); }

    GetWindowTextW(m_editBlockAlpha, buf, 16);
    int ba = _wtoi(buf);
    if (ba >= 0 && ba <= 255) { m_cfg->historyBlockAlpha = ba; SendMessageW(m_trackBlockAlpha, TBM_SETPOS, TRUE, ba); }

    GetWindowTextW(m_editChartTime, buf, 16);
    int ct = _wtoi(buf);
    if (ct >= 1 && ct <= 30) { m_cfg->chartTimeRange = ct * 1000; SendMessageW(m_trackChartTime, TBM_SETPOS, TRUE, ct); }

    GetWindowTextW(m_editChartRadius, buf, 16);
    int cr = _wtoi(buf);
    if (cr >= 0 && cr <= 20) { m_cfg->chartRadius = cr; SendMessageW(m_trackChartRadius, TBM_SETPOS, TRUE, cr); }

    GetWindowTextW(m_editChartW, buf, 16);
    int cw = _wtoi(buf);
    if (cw >= 200 && cw <= 800) { m_cfg->chartW = cw; SendMessageW(m_trackChartW, TBM_SETPOS, TRUE, cw); }

    GetWindowTextW(m_editChartH, buf, 16);
    int ch = _wtoi(buf);
    if (ch >= 100 && ch <= 600) { m_cfg->chartH = ch; SendMessageW(m_trackChartH, TBM_SETPOS, TRUE, ch); }

    auto syncMargEdit = [&](HWND ed, HWND tr, int& v, int lo, int hi) {
        GetWindowTextW(ed, buf, 16); int val = _wtoi(buf);
        if (val >= lo && val <= hi) { v = val; SendMessageW(tr, TBM_SETPOS, TRUE, val); }
    };
    syncMargEdit(m_editChartML, m_trackChartML, m_cfg->chartMarginL, 0, 60);
    syncMargEdit(m_editChartMR, m_trackChartMR, m_cfg->chartMarginR, 0, 60);
    syncMargEdit(m_editChartMT, m_trackChartMT, m_cfg->chartMarginT, 0, 60);
    syncMargEdit(m_editChartMB, m_trackChartMB, m_cfg->chartMarginB, 0, 60);

    GetWindowTextW(m_editChartSnapX, buf, 16);
    int sx = _wtoi(buf);
    if (sx >= -200 && sx <= 200) { m_cfg->chartSnapOffsetX = sx; SendMessageW(m_trackChartSnapX, TBM_SETPOS, TRUE, sx); }
    GetWindowTextW(m_editChartSnapY, buf, 16);
    int sy = _wtoi(buf);
    if (sy >= -200 && sy <= 200) { m_cfg->chartSnapOffsetY = sy; SendMessageW(m_trackChartSnapY, TBM_SETPOS, TRUE, sy); }

    const wchar_t* savePath = m_configPath.empty() ? L"KeyStateSetting.json" : m_configPath.c_str();
    m_cfg->Save(savePath);

    if (m_display && m_cfg) {
        m_display->SetClickThrough(m_cfg->clickThrough);
    }
}

// ========== 应用 Theme 预设 ==========
void SettingsUI::ApplyTheme(int tid) {
    if (!m_cfg || m_cfg->keys.empty()) return;
    if (tid < 0 || tid > 15) return;
    if ((int)m_cfg->themePresets.size() <= tid) m_cfg->InitDefaultThemes();
    m_cfg->theme = tid;
    auto& tp = m_cfg->themePresets[tid];
    for (auto& kc : m_cfg->keys) {
        if (tp.font.a > 0)   kc.colorFont   = tp.font;
        if (tp.normal.a > 0) kc.colorNormal = tp.normal;
        if (tp.press.a > 0)  kc.colorPress  = tp.press;
    }
    if (tp.chartBg.a > 0)   m_cfg->chartBgCol   = tp.chartBg;
    if (tp.chartLine.a > 0) m_cfg->chartLineCol = tp.chartLine;
    OnSave();
    UpdateColorSwatches(m_selectedKey);
    // 刷新 chart 颜色显示
    if (m_swChartBg && m_swChartLine) {
        {
            delete reinterpret_cast<SwatchData*>(GetWindowLongPtrW(m_swChartBg, GWLP_USERDATA));
            auto sd = new SwatchData{&m_cfg->chartBgCol};
            SetWindowLongPtrW(m_swChartBg, GWLP_USERDATA, (LONG_PTR)sd);
            InvalidateRect(m_swChartBg, nullptr, TRUE);
        }
        {
            delete reinterpret_cast<SwatchData*>(GetWindowLongPtrW(m_swChartLine, GWLP_USERDATA));
            auto sd = new SwatchData{&m_cfg->chartLineCol};
            SetWindowLongPtrW(m_swChartLine, GWLP_USERDATA, (LONG_PTR)sd);
            InvalidateRect(m_swChartLine, nullptr, TRUE);
        }
        wchar_t cb[64];
        swprintf(cb,64,L"RGB(%d,%d,%d)",m_cfg->chartBgCol.r,m_cfg->chartBgCol.g,m_cfg->chartBgCol.b);
        SetWindowTextW(m_lblChartBg, cb);
        swprintf(cb,64,L"RGB(%d,%d,%d)",m_cfg->chartLineCol.r,m_cfg->chartLineCol.g,m_cfg->chartLineCol.b);
        SetWindowTextW(m_lblChartLine, cb);
    }
}

void SettingsUI::RebuildWindow() {
    if (!m_hwnd) return;
    DestroyWindow(m_hwnd); m_hwnd = nullptr;
    m_scrollY = 0;
    Create(m_hInst, m_cfg, m_display, m_chart, m_themeEditor, m_ksm);
    if (m_hwnd) RefreshControls();
}

void SettingsUI::RefreshControls() {
    if (!m_cfg) return;
    SendMessageW(m_chkTotal,   BM_SETCHECK, m_cfg->showTotal    ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_chkKPS,     BM_SETCHECK, m_cfg->showKPS      ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_chkSummary, BM_SETCHECK, m_cfg->showSummary  ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_chkHistory, BM_SETCHECK, m_cfg->showHistory  ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_chkTrackLines, BM_SETCHECK, m_cfg->historyShowLines ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_chkBPM,     BM_SETCHECK, m_cfg->showBPM      ? BST_CHECKED : BST_UNCHECKED, 0);
    int nd = m_cfg->bpmNoteDiv;
    SendMessageW(m_radioNote8,  BM_SETCHECK, (nd == 8)  ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_radioNote16, BM_SETCHECK, (nd != 8 && nd != 32 && nd != 64) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_radioNote32, BM_SETCHECK, (nd == 32) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_radioNote64, BM_SETCHECK, (nd == 64) ? BST_CHECKED : BST_UNCHECKED, 0);
    int l = m_cfg->lang;
    SendMessageW(m_radioLangCN, BM_SETCHECK, (l == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_radioLangEN, BM_SETCHECK, (l == 1) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_radioLangJP, BM_SETCHECK, (l == 2) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_chkThrough, BM_SETCHECK, m_cfg->clickThrough ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_chkTopMost, BM_SETCHECK, m_cfg->alwaysOnTop ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_trackKeySize, TBM_SETPOS, TRUE, m_cfg->keySize);
    SendMessageW(m_trackSpacing, TBM_SETPOS, TRUE, m_cfg->keySpacing);
    SendMessageW(m_trackHistoryH, TBM_SETPOS, TRUE, m_cfg->historyTrackH);
    SendMessageW(m_trackGrowSpd,  TBM_SETPOS, TRUE, m_cfg->historyGrowSpeed);
    SendMessageW(m_trackFloatSpd, TBM_SETPOS, TRUE, m_cfg->historyFloatSpeed);
    SendMessageW(m_trackBlockMax, TBM_SETPOS, TRUE, m_cfg->historyBlockMax);
    SendMessageW(m_trackTrackGap,TBM_SETPOS, TRUE, m_cfg->historyTrackGap);
    SendMessageW(m_trackBpmMerge,TBM_SETPOS, TRUE, m_cfg->bpmMergeMs);
    SendMessageW(m_trackTrackAlpha,TBM_SETPOS,TRUE,m_cfg->historyTrackAlpha);
    SendMessageW(m_trackBlockAlpha,TBM_SETPOS,TRUE,m_cfg->historyBlockAlpha);
    SendMessageW(m_trackBorder,   TBM_SETPOS, TRUE, m_cfg->keyBorderW);
    SendMessageW(m_trackOpacity,  TBM_SETPOS, TRUE, (int)(m_cfg->overlayOpacity * 100));
    int th = m_cfg->theme;
    for (int i = 0; i < 16; ++i) {
        SendMessageW(((HWND*)&m_radioTheme0)[i], BM_SETCHECK, (th == i) ? BST_CHECKED : BST_UNCHECKED, 0);
        int lid = (i == 0) ? 46 : ((i <= 3) ? (46 + i) : (70 + i - 4));
        const wchar_t* name = (i < (int)m_cfg->themePresets.size() && !m_cfg->themePresets[i].name.empty())
            ? m_cfg->themePresets[i].name.c_str() : LANG(lid);
        SetWindowTextW(((HWND*)&m_radioTheme0)[i], name);
    }
    SyncKeySizeEdit();
    // 更新字体名称显示
    if (m_btnFont) {
        HWND hLabel = GetWindow(m_btnFont, GW_HWNDNEXT);
        if (hLabel) { wchar_t fb[64]; swprintf(fb, 64, L"%ls", m_cfg->keyFontName.c_str()); SetWindowTextW(hLabel, fb); }
    }
    SyncSpacingEdit();
    SyncHistoryHEdit();
    SyncGrowSpdEdit();
    SyncFloatSpdEdit();
    SyncBlockMaxEdit();

    int f = m_cfg->fps;
    SendMessageW(m_radioFps25, BM_SETCHECK, (f == 25)  ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_radioFps45, BM_SETCHECK, (f == 45)  ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_radioFps60, BM_SETCHECK, (f == 60)  ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_radioFps90, BM_SETCHECK, (f == 90)  ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_radioFps120,BM_SETCHECK, (f == 120) ? BST_CHECKED : BST_UNCHECKED, 0);

    SyncBorderEdit();
    SyncOpacityEdit();

    // Chart controls
    SendMessageW(m_chkChart, BM_SETCHECK, m_cfg->showChart ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_chkChartGrid, BM_SETCHECK, m_cfg->chartShowGrid ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_trackChartTime, TBM_SETPOS, TRUE, m_cfg->chartTimeRange / 1000);
    wchar_t ctb[16]; swprintf(ctb,16,L"%d",m_cfg->chartTimeRange/1000); SetWindowTextW(m_editChartTime,ctb);
    SendMessageW(m_trackChartRadius, TBM_SETPOS, TRUE, m_cfg->chartRadius);
    wchar_t crb[16]; swprintf(crb,16,L"%d",m_cfg->chartRadius); SetWindowTextW(m_editChartRadius,crb);
    SendMessageW(m_trackChartW, TBM_SETPOS, TRUE, m_cfg->chartW);
    { wchar_t wb[16]; swprintf(wb,16,L"%d",m_cfg->chartW); SetWindowTextW(m_editChartW,wb); }
    SendMessageW(m_trackChartH, TBM_SETPOS, TRUE, m_cfg->chartH);
    { wchar_t hb[16]; swprintf(hb,16,L"%d",m_cfg->chartH); SetWindowTextW(m_editChartH,hb); }
    auto synMarg = [](HWND tr, HWND ed, int v) {
        SendMessageW(tr, TBM_SETPOS, TRUE, v);
        wchar_t b[16]; swprintf(b,16,L"%d",v); SetWindowTextW(ed,b);
    };
    synMarg(m_trackChartML, m_editChartML, m_cfg->chartMarginL);
    synMarg(m_trackChartMR, m_editChartMR, m_cfg->chartMarginR);
    synMarg(m_trackChartMT, m_editChartMT, m_cfg->chartMarginT);
    synMarg(m_trackChartMB, m_editChartMB, m_cfg->chartMarginB);

    // Snap controls
    SendMessageW(m_chkChartSnap, BM_SETCHECK, m_cfg->chartSnap ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(m_trackChartSnapX, TBM_SETPOS, TRUE, m_cfg->chartSnapOffsetX);
    { wchar_t xb[16]; swprintf(xb,16,L"%d",m_cfg->chartSnapOffsetX); SetWindowTextW(m_editChartSnapX,xb); }
    SendMessageW(m_trackChartSnapY, TBM_SETPOS, TRUE, m_cfg->chartSnapOffsetY);
    { wchar_t yb[16]; swprintf(yb,16,L"%d",m_cfg->chartSnapOffsetY); SetWindowTextW(m_editChartSnapY,yb); }

    RefreshKeyList();
}

void SettingsUI::OnDestroy() {
    auto cleanSwatch = [](HWND sw) {
        if (sw) delete reinterpret_cast<SwatchData*>(GetWindowLongPtrW(sw, GWLP_USERDATA));
    };
    cleanSwatch(m_swFont);
    cleanSwatch(m_swNormal);
    cleanSwatch(m_swPress);
    cleanSwatch(m_swChartBg);
    cleanSwatch(m_swChartLine);
    m_hwnd = nullptr;
}
