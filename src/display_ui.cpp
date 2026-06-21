#include "display_ui.h"

static const wchar_t* DISPLAY_CLASS = L"KeyStateDisplayV3";

DisplayUI::DisplayUI()  = default;
DisplayUI::~DisplayUI() {
    CleanupLastFrame();
    if (m_hwnd) DestroyWindow(m_hwnd);
}

void DisplayUI::CleanupLastFrame() {
    if (m_lastMemDC) {
        if (m_lastBmp) SelectObject(m_lastMemDC, GetStockObject(DC_PEN));
        DeleteDC(m_lastMemDC);
        m_lastMemDC = nullptr;
    }
    if (m_lastBmp) {
        DeleteObject(m_lastBmp);
        m_lastBmp = nullptr;
    }
}

bool DisplayUI::Create(HINSTANCE hInst) {
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = DISPLAY_CLASS;
    wc.hCursor       = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        DISPLAY_CLASS, L"KeyState Overlay",
        WS_POPUP,
        100, 100, 400, 120,
        nullptr, nullptr, hInst, this);

    return m_hwnd != nullptr;
}

void DisplayUI::Show(bool visible) {
    ShowWindow(m_hwnd, visible ? SW_SHOWNA : SW_HIDE);
}

void DisplayUI::UpdateOverlay(const AppConfig& cfg, const KeyStateManager& ksm) {
    if (!m_hwnd) return;

    HBITMAP hBmp = nullptr;
    HDC     hMemDC = nullptr;
    int w = 0, h = 0;

    if (!m_renderer.Render(cfg, ksm, hBmp, hMemDC, w, h))
        return;
    if (!hBmp || !hMemDC || w <= 0 || h <= 0)
        return;

    HDC hdcScreen = GetDC(nullptr);

    BLENDFUNCTION blend = {};
    blend.BlendOp             = AC_SRC_OVER;
    blend.BlendFlags          = 0;
    blend.SourceConstantAlpha = (BYTE)(cfg.overlayOpacity * 255);
    blend.AlphaFormat         = AC_SRC_ALPHA;

    // 计算轨道偏移：开启轨道时窗口上移，保持按键映射位置不变
    int trackOffsetY = 0;
    if (cfg.showHistory && !cfg.keys.empty()) {
        trackOffsetY = cfg.historyTrackH + cfg.historyTrackGap;
    }
    SIZE   sz    = {w, h};
    POINT  ptSrc = {0, 0};
    POINT  ptDst = {cfg.displayX, cfg.displayY - trackOffsetY};

    UpdateLayeredWindow(m_hwnd, hdcScreen, &ptDst, &sz,
                        hMemDC, &ptSrc, 0, &blend, ULW_ALPHA);

    ReleaseDC(nullptr, hdcScreen);

    // 每 500ms 重新置顶一次，对抗全屏应用遮挡
    if (cfg.alwaysOnTop) {
        uint64_t now = GetTickCount64();
        if (now - m_lastTopMostCheck > 500) {
            m_lastTopMostCheck = now;
            SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    CleanupLastFrame();
    m_lastBmp   = hBmp;
    m_lastMemDC = hMemDC;
}

void DisplayUI::MoveTo(int x, int y) {
    if (m_cfg) { m_cfg->displayX = x; m_cfg->displayY = y; }
    SetWindowPos(m_hwnd, nullptr, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void DisplayUI::SetClickThrough(bool enable) {
    m_clickThrough = enable;
    if (!m_hwnd) return;
    LONG ex = GetWindowLongW(m_hwnd, GWL_EXSTYLE);
    if (enable) ex |= WS_EX_TRANSPARENT;
    else        ex &= ~WS_EX_TRANSPARENT;
    SetWindowLongW(m_hwnd, GWL_EXSTYLE, ex);
    SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void DisplayUI::SetTopMost(bool enable) {
    if (!m_hwnd) return;
    SetWindowPos(m_hwnd, enable ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

// ========== 窗口过程 ==========
LRESULT CALLBACK DisplayUI::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lp);
        auto* self = static_cast<DisplayUI*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        if (self) self->m_hwnd = hwnd;
    }
    auto* self = reinterpret_cast<DisplayUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
    case WM_RBUTTONUP:
        if (self->m_notifyHwnd)
            PostMessageW(self->m_notifyHwnd, WM_APP + 2, 0, 0);
        return 0;

    case WM_LBUTTONDOWN:
        // 开始拖拽
        SetCapture(hwnd);
        self->m_dragging = true;
        GetCursorPos(&self->m_dragOffset);
        // 偏移 = 鼠标屏幕坐标 - 窗口当前左上角
        if (self->m_cfg) {
            self->m_dragOffset.x -= self->m_cfg->displayX;
            self->m_dragOffset.y -= self->m_cfg->displayY;
        }
        return 0;

    case WM_MOUSEMOVE:
        if (self->m_dragging && self->m_cfg) {
            POINT pt;
            GetCursorPos(&pt);
            int newX = pt.x - self->m_dragOffset.x;
            int newY = pt.y - self->m_dragOffset.y;
            self->m_cfg->displayX = newX;
            self->m_cfg->displayY = newY;
        }
        return 0;

    case WM_LBUTTONUP:
        if (self->m_dragging) {
            ReleaseCapture();
            self->m_dragging = false;
            if (self->m_cfg) {
                const wchar_t* sp = self->m_configPath.empty()
                    ? L"KeyStateSetting.json" : self->m_configPath.c_str();
                self->m_cfg->Save(sp);
            }
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}
