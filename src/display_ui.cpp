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

    // V1.6: 计算轨道偏移（反转模式下轨道在按键下方，无需偏移）
    int trackOffsetY = 0;
    if (cfg.showHistory && !cfg.keys.empty()) {
        if (cfg.historyTrackReverse) {
            trackOffsetY = 0;  // 反转模式：轨道在按键下方，窗口无需上移
        } else {
            trackOffsetY = cfg.historyTrackH + cfg.historyTrackGap;
        }
    }
    // 自由模式：渲染器在 bitmap 左侧/顶部加了 10px margin，窗口位置左移/上移补偿
    POINT ptDst = {cfg.displayX - (cfg.freeMode ? 10 : 0),
                   cfg.displayY - trackOffsetY - (cfg.freeMode ? 10 : 0)};
    SIZE   sz    = {w, h};
    POINT  ptSrc = {0, 0};

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

    case WM_LBUTTONDOWN: {
        SetCapture(hwnd);
        self->m_dragging = true;
        GetCursorPos(&self->m_dragOffset);
        if (!self->m_cfg) return 0;

        if (self->m_cfg->freeMode) {
            // 自由模式：检测点击到哪个元素
            POINT pt;
            GetCursorPos(&pt);
            int cx = pt.x;
            int cy = pt.y;
            int ks = self->m_cfg->keySize;
            int trackOffsetY = 0;
            if (self->m_cfg->showHistory && !self->m_cfg->keys.empty()) {
                trackOffsetY = self->m_cfg->historyTrackH + self->m_cfg->historyTrackGap;
            }
            // V1.6: hit test = displayX + freeX (window 和 bitmap 各 +/-10px margin，正负抵消)
            int winX = self->m_cfg->displayX;
            int winY = self->m_cfg->displayY - trackOffsetY;

            self->m_freeDragIdx = -1; // 默认拖拽窗口

            // 检查是否点击到按键（使用自定义宽高）
            for (int i = 0; i < (int)self->m_cfg->keys.size(); ++i) {
                int kw = (self->m_cfg->keys[i].customW > 0) ? self->m_cfg->keys[i].customW : ks;
                int kh = (self->m_cfg->keys[i].customH > 0) ? self->m_cfg->keys[i].customH : ks;
                int kx = winX + self->m_cfg->keys[i].freeX;
                int ky = winY + self->m_cfg->keys[i].freeY;
                if (cx >= kx && cx < kx + kw && cy >= ky && cy < ky + kh) {
                    self->m_freeDragIdx = i;
                    self->m_dragOffset.x = cx - kx;
                    self->m_dragOffset.y = cy - ky;
                    break;
                }
            }
            // 检查 Total 框
            if (self->m_freeDragIdx == -1 && self->m_cfg->showSummary && !self->m_cfg->keys.empty()) {
                int bw = self->m_cfg->totalBoxW > 0 ? self->m_cfg->totalBoxW : ks;
                int bh = self->m_cfg->totalBoxH > 0 ? self->m_cfg->totalBoxH : ks;
                int bx = winX + self->m_cfg->freeTotalX;
                int by = winY + self->m_cfg->freeTotalY;
                if (cx >= bx && cx < bx + bw && cy >= by && cy < by + bh) {
                    self->m_freeDragIdx = -2; // Total
                    self->m_dragOffset.x = cx - bx;
                    self->m_dragOffset.y = cy - by;
                }
            }
            // 检查 KPS 框
            if (self->m_freeDragIdx == -1 && self->m_cfg->showKPS && !self->m_cfg->keys.empty()) {
                int bw = self->m_cfg->kpsBoxW > 0 ? self->m_cfg->kpsBoxW : ks;
                int bh = self->m_cfg->kpsBoxH > 0 ? self->m_cfg->kpsBoxH : ks;
                int bx = winX + self->m_cfg->freeKPSX;
                int by = winY + self->m_cfg->freeKPSY;
                if (cx >= bx && cx < bx + bw && cy >= by && cy < by + bh) {
                    self->m_freeDragIdx = -3; // KPS
                    self->m_dragOffset.x = cx - bx;
                    self->m_dragOffset.y = cy - by;
                }
            }
            // 检查 BPM 框
            if (self->m_freeDragIdx == -1 && self->m_cfg->showBPM && !self->m_cfg->keys.empty()) {
                int bw = self->m_cfg->bpmBoxW > 0 ? self->m_cfg->bpmBoxW : ks;
                int bh = self->m_cfg->bpmBoxH > 0 ? self->m_cfg->bpmBoxH : ks;
                int bx = winX + self->m_cfg->freeBPMX;
                int by = winY + self->m_cfg->freeBPMY;
                if (cx >= bx && cx < bx + bw && cy >= by && cy < by + bh) {
                    self->m_freeDragIdx = -4; // BPM
                    self->m_dragOffset.x = cx - bx;
                    self->m_dragOffset.y = cy - by;
                }
            }

            if (self->m_freeDragIdx == -1) {
                // 拖拽窗口整体（使用逻辑坐标 displayX/displayY，实际窗口位置由 UpdateOverlay 自动加 margin）
                self->m_dragOffset.x = cx - self->m_cfg->displayX;
                self->m_dragOffset.y = cy - (self->m_cfg->displayY - trackOffsetY);
            }
        } else {
            // 常规模式：拖拽整体窗口
            self->m_freeDragIdx = -1;
            self->m_dragOffset.x -= self->m_cfg->displayX;
            self->m_dragOffset.y -= self->m_cfg->displayY;
        }
        return 0;
    }

    case WM_MOUSEMOVE:
        if (self->m_dragging && self->m_cfg) {
            POINT pt;
            GetCursorPos(&pt);
            // V1.6: 网格吸附始终启用（边界显示时生效，隐藏即固定布局）
            auto snapToGrid = [&](int& v) {
                if (self->m_cfg->freeShowBoundary && self->m_cfg->freeGridSize > 4) {
                    int gs = self->m_cfg->freeGridSize;
                    v = ((v + gs / 2) / gs) * gs;
                }
            };
            auto calcFreePos = [&](int rawX, int rawY, int& fx, int& fy) {
                int trackOffsetY = 0;
                if (self->m_cfg->showHistory && !self->m_cfg->keys.empty()) {
                    trackOffsetY = self->m_cfg->historyTrackH + self->m_cfg->historyTrackGap;
                }
                int winX = self->m_cfg->displayX;
                int winY = self->m_cfg->displayY - trackOffsetY;
                fx = rawX - winX;
                fy = rawY - winY;
                snapToGrid(fx);
                snapToGrid(fy);
            };
            auto clampFree = [&](int& fx, int& fy, int elemW, int elemH) {
                int areaW = self->m_cfg->freeAreaW;
                int areaH = self->m_cfg->freeAreaH;
                if (fx < 0) fx = 0;
                if (fx + elemW > areaW) fx = areaW - elemW;
                if (fy < 0) fy = 0;
                if (fy + elemH > areaH) fy = areaH - elemH;
            };

            if (self->m_cfg->freeMode && self->m_freeDragIdx >= 0) {
                // 拖拽单个按键
                int newX = pt.x - self->m_dragOffset.x;
                int newY = pt.y - self->m_dragOffset.y;
                int ks = self->m_cfg->keySize;
                if (self->m_freeDragIdx < (int)self->m_cfg->keys.size()) {
                    int kw = (self->m_cfg->keys[self->m_freeDragIdx].customW > 0) ? self->m_cfg->keys[self->m_freeDragIdx].customW : ks;
                    int kh = (self->m_cfg->keys[self->m_freeDragIdx].customH > 0) ? self->m_cfg->keys[self->m_freeDragIdx].customH : ks;
                    int fx, fy;
                    calcFreePos(newX, newY, fx, fy);
                    clampFree(fx, fy, kw, kh);
                    self->m_cfg->keys[self->m_freeDragIdx].freeX = fx;
                    self->m_cfg->keys[self->m_freeDragIdx].freeY = fy;
                }
            } else if (self->m_cfg->freeMode && self->m_freeDragIdx == -2) {
                int newX = pt.x - self->m_dragOffset.x;
                int newY = pt.y - self->m_dragOffset.y;
                int fx, fy;
                calcFreePos(newX, newY, fx, fy);
                int bw = self->m_cfg->totalBoxW > 0 ? self->m_cfg->totalBoxW : self->m_cfg->keySize;
                int bh = self->m_cfg->totalBoxH > 0 ? self->m_cfg->totalBoxH : self->m_cfg->keySize;
                clampFree(fx, fy, bw, bh);
                self->m_cfg->freeTotalX = fx;
                self->m_cfg->freeTotalY = fy;
            } else if (self->m_cfg->freeMode && self->m_freeDragIdx == -3) {
                int newX = pt.x - self->m_dragOffset.x;
                int newY = pt.y - self->m_dragOffset.y;
                int fx, fy;
                calcFreePos(newX, newY, fx, fy);
                int bw = self->m_cfg->kpsBoxW > 0 ? self->m_cfg->kpsBoxW : self->m_cfg->keySize;
                int bh = self->m_cfg->kpsBoxH > 0 ? self->m_cfg->kpsBoxH : self->m_cfg->keySize;
                clampFree(fx, fy, bw, bh);
                self->m_cfg->freeKPSX = fx;
                self->m_cfg->freeKPSY = fy;
            } else if (self->m_cfg->freeMode && self->m_freeDragIdx == -4) {
                int newX = pt.x - self->m_dragOffset.x;
                int newY = pt.y - self->m_dragOffset.y;
                int fx, fy;
                calcFreePos(newX, newY, fx, fy);
                int bw = self->m_cfg->bpmBoxW > 0 ? self->m_cfg->bpmBoxW : self->m_cfg->keySize;
                int bh = self->m_cfg->bpmBoxH > 0 ? self->m_cfg->bpmBoxH : self->m_cfg->keySize;
                clampFree(fx, fy, bw, bh);
                self->m_cfg->freeBPMX = fx;
                self->m_cfg->freeBPMY = fy;
            } else {
                // 拖拽整体窗口
                self->m_cfg->displayX = pt.x - self->m_dragOffset.x;
                self->m_cfg->displayY = pt.y - self->m_dragOffset.y;
            }
        }
        return 0;

    case WM_LBUTTONUP:
        if (self->m_dragging) {
            ReleaseCapture();
            self->m_dragging = false;
            self->m_freeDragIdx = -1;
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
