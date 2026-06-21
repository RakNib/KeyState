#include "chart_ui.h"
#include <algorithm>
#include <cmath>

static const wchar_t* CHART_CLASS = L"KeyStateChartV1";
static constexpr int  kChartW_def = 400;
static constexpr int  kChartH_def = 180;
static constexpr int  kMarginL    = 35;   // Y轴标签区 (右对齐)
static constexpr int  kMarginR    = 15;   // 右侧留白 (与左侧视觉平衡)
static constexpr int  kMarginT    = 14;   // 顶部留白
static constexpr int  kMarginB    = 22;   // X轴标签区
static constexpr int  kYmax       = 20;

ChartUI::ChartUI() {
    Gdiplus::GdiplusStartupInput si;
    Gdiplus::GdiplusStartup(&m_gdiToken, &si, nullptr);

    m_font      = new Gdiplus::Font(L"Segoe UI", 9,  Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    m_smallFont = new Gdiplus::Font(L"Segoe UI", 7,  Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    m_rightFormat = new Gdiplus::StringFormat();
    m_rightFormat->SetAlignment(Gdiplus::StringAlignmentFar);
    m_rightFormat->SetLineAlignment(Gdiplus::StringAlignmentCenter);
}

ChartUI::~ChartUI() {
    CleanupLastFrame();
    delete m_rightFormat;
    delete m_smallFont;
    delete m_font;
    Gdiplus::GdiplusShutdown(m_gdiToken);
    if (m_hwnd) DestroyWindow(m_hwnd);
}

void ChartUI::CleanupLastFrame() {
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

bool ChartUI::Create(HINSTANCE hInst) {
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CHART_CLASS;
    wc.hCursor       = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        CHART_CLASS, L"KeyState Chart",
        WS_POPUP,
        100, 330, kChartW_def, kChartH_def,
        nullptr, nullptr, hInst, this);

    return m_hwnd != nullptr;
}

void ChartUI::Show(bool visible) {
    ShowWindow(m_hwnd, visible ? SW_SHOWNA : SW_HIDE);
}

// 圆角矩形路径（用引用传出，避免 GDI+ 拷贝构造私有化问题）
static void MakeRoundRect(Gdiplus::GraphicsPath& path, int x, int y, int w, int h, int r) {
    if (r <= 0) { path.AddRectangle(Gdiplus::Rect(x, y, w, h)); return; }
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    path.AddArc(x, y, r * 2, r * 2, 180, 90);
    path.AddArc(x + w - r * 2, y, r * 2, r * 2, 270, 90);
    path.AddArc(x + w - r * 2, y + h - r * 2, r * 2, r * 2, 0, 90);
    path.AddArc(x, y + h - r * 2, r * 2, r * 2, 90, 90);
    path.CloseFigure();
}

// ========== 折线图渲染 ==========
bool ChartUI::RenderChart(const AppConfig& cfg, const KeyStateManager& ksm,
                           HBITMAP& outBmp, HDC& outMemDC, int w, int h) {
    Gdiplus::Bitmap bmp(w, h, PixelFormat32bppARGB);
    Gdiplus::Graphics g(&bmp);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    int chartR = cfg.chartRadius;
    int tRange = cfg.chartTimeRange;
    if (tRange < 1000) tRange = 1000;
    if (tRange > 30000) tRange = 30000;

    // 动态边距
    int mgL = cfg.chartMarginL > 0 ? cfg.chartMarginL : kMarginL;
    int mgR = cfg.chartMarginR > 0 ? cfg.chartMarginR : kMarginR;
    int mgT = cfg.chartMarginT > 0 ? cfg.chartMarginT : kMarginT;
    int mgB = cfg.chartMarginB > 0 ? cfg.chartMarginB : kMarginB;

    // 采样当前 KPS
    uint64_t now = GetTickCount64();
    double curKPS = 0;
    for (size_t i = 0; i < cfg.keys.size(); ++i)
        curKPS += ksm.GetKPS(cfg.keys[i].keyCode);

    // 采样间隔 = 时间范围 / 像素宽度（约 1 数据点/像素）
    int plotW = w - mgL - mgR;
    int plotH = h - mgT - mgB;
    uint64_t sampleInterval = (uint64_t)(tRange / (std::max)(1, plotW));
    if (sampleInterval < 16) sampleInterval = 16;
    if (m_history.empty() || now - m_lastSample >= sampleInterval) {
        m_history.push_back({now, curKPS});
        m_lastSample = now;
    }
    // 清理过期数据
    while (!m_history.empty() && now - m_history.front().time > (uint64_t)tRange)
        m_history.pop_front();

    // === 动态 Y 轴上限 (摄像头) ===
    double peak = 0;
    for (auto& p : m_history)
        if (p.kps > peak) peak = p.kps;
    float targetYmax = (float)(std::max)(20.0, peak);   // 不低于 20
    targetYmax = std::ceil(targetYmax / 5.0f) * 5.0f;   // 取整到 5 的倍数 (25,30,35...)
    // 上升快、回落慢
    float blend = (targetYmax > m_yMaxSmooth) ? 0.12f : 0.04f;
    m_yMaxSmooth += (targetYmax - m_yMaxSmooth) * blend;
    float yMax = m_yMaxSmooth;

    // === 阴影（先画，在背景后） ===
    if (chartR > 0) {
        for (int dy = 1; dy <= 3; ++dy) {
            BYTE a = (BYTE)(12 - dy * 3);
            Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(a, 0, 0, 0));
            Gdiplus::GraphicsPath shadowPath;
            MakeRoundRect(shadowPath, dy, dy, w - 1, h - 1, chartR + 1);
            g.FillPath(&shadowBrush, &shadowPath);
        }
    }

    // === 背景 + 圆角裁剪 ===
    auto& bgc = cfg.chartBgCol;
    Gdiplus::SolidBrush bgBrush(Gdiplus::Color(bgc.a, bgc.r, bgc.g, bgc.b));
    if (chartR > 0) {
        Gdiplus::GraphicsPath bgPath;
        MakeRoundRect(bgPath, 0, 0, w - 1, h - 1, chartR);
        g.FillPath(&bgBrush, &bgPath);
    } else {
        g.FillRectangle(&bgBrush, 0, 0, w, h);
    }

    // === 裁剪到 plot 区域 ===
    Gdiplus::GraphicsPath clipPath;
    MakeRoundRect(clipPath, 0, 0, w - 1, h - 1, chartR > 0 ? chartR : 0);
    g.SetClip(&clipPath);

    // === 网格线（位置固定，不随数据变化） ===
    Gdiplus::Color gridColor(28, 120, 120, 120);
    Gdiplus::Pen gridPen(gridColor, 0.5f);
    // 水平线 (Y)
    for (int i = 0; i <= 4; ++i) {
        int gy = mgT + plotH - (plotH * i / 4);
        g.DrawLine(&gridPen, mgL, gy, mgL + plotW, gy);
    }
    // 垂直线 (X) — 5 条，代表时间分隔，位置固定
    int nXLines = 5;
    for (int i = 0; i <= nXLines; ++i) {
        int gx = mgL + (plotW * i / nXLines);
        g.DrawLine(&gridPen, gx, mgT, gx, mgT + plotH);
    }

    // === Y 轴标签（固定） ===
    Gdiplus::SolidBrush labelBrush(Gdiplus::Color(100, 120, 120, 120));
    wchar_t buf[32];
    for (int i = 0; i <= 4; ++i) {
        int val = (int)(yMax * i / 4);
        int gy = mgT + plotH - (plotH * i / 4);
        swprintf(buf, 32, L"%d", val);
        Gdiplus::RectF labelR(0.0f, (float)(gy - 8), (float)(mgL - 4), 16.0f);
        g.DrawString(buf, -1, m_smallFont, labelR, m_rightFormat, &labelBrush);
    }

    // === 标题 ===
    Gdiplus::SolidBrush titleBrush(Gdiplus::Color(140, 160, 160, 160));
    Gdiplus::RectF titleR((float)mgL, 1.0f, (float)plotW, (float)mgT);
    g.DrawString(L"KPS", 3, m_smallFont, titleR, nullptr, &titleBrush);

    // === X 轴标签（固定，相对时间） ===
    int tSec = (tRange + 999) / 1000;  // 总秒数
    for (int i = 0; i <= nXLines; ++i) {
        int sec = -tSec + (tSec * i / nXLines);
        int gx = mgL + (plotW * i / nXLines);
        swprintf(buf, 32, L"%ds", sec);
        Gdiplus::RectF xLabelR((float)(gx - 15), (float)(mgT + plotH + 2), 30.0f, 12.0f);
        Gdiplus::StringFormat xFmt;
        xFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
        g.DrawString(buf, -1, m_smallFont, xLabelR, &xFmt, &labelBrush);
    }

    // === 折线绘制 ===
    if (m_history.size() >= 2) {
        auto mapX = [&](uint64_t t) -> float {
            float ratio = 1.0f - (float)(now - t) / (float)tRange;
            if (ratio < 0) ratio = 0;
            if (ratio > 1) ratio = 1;
            return mgL + ratio * plotW;
        };
        auto mapY = [&](double kps) -> int {
            return mgT + plotH - (int)(plotH * kps / yMax);
        };

        auto& lc = cfg.chartLineCol;

        // 填充区域
        Gdiplus::GraphicsPath fillPath;
        auto& first = m_history.front();
        fillPath.AddLine(mapX(first.time), (float)(mgT + plotH),
                          mapX(first.time), (float)mapY(first.kps));
        for (auto& p : m_history)
            fillPath.AddLine(mapX(p.time), (float)mapY(p.kps), mapX(p.time), (float)mapY(p.kps));
        auto& last = m_history.back();
        fillPath.AddLine(mapX(last.time), (float)mapY(last.kps),
                          mapX(last.time), (float)(mgT + plotH));
        fillPath.CloseFigure();

        Gdiplus::SolidBrush fillBrush(Gdiplus::Color(35, lc.r, lc.g, lc.b));
        Gdiplus::Rect clipRect(mgL, mgT, plotW, plotH);
        g.SetClip(clipRect, Gdiplus::CombineModeIntersect);
        g.FillPath(&fillBrush, &fillPath);

        // 折线
        Gdiplus::Pen linePen(Gdiplus::Color(lc.a, lc.r, lc.g, lc.b), 1.8f);
        linePen.SetLineJoin(Gdiplus::LineJoinRound);
        for (size_t i = 1; i < m_history.size(); ++i) {
            float x0 = mapX(m_history[i - 1].time);
            float y0 = (float)mapY(m_history[i - 1].kps);
            float x1 = mapX(m_history[i].time);
            float y1 = (float)mapY(m_history[i].kps);
            g.DrawLine(&linePen, x0, y0, x1, y1);
        }
        g.SetClip(&clipPath);  // 恢复圆角裁剪

        // 当前值标注（固定在最右侧，不随数据点位置抖动）
        Gdiplus::SolidBrush curBrush(Gdiplus::Color(240, lc.r, lc.g, lc.b));
        swprintf(buf, 32, L"%.1f", curKPS);
        float labelX = (float)(mgL + plotW - 35);
        float labelY = (float)(mapY(last.kps) - 12);
        if (labelY < mgT) labelY = (float)mgT;
        Gdiplus::RectF curLabelR(labelX, labelY, 40.0f, 14.0f);
        Gdiplus::StringFormat rightFmt;
        rightFmt.SetAlignment(Gdiplus::StringAlignmentFar);
        g.DrawString(buf, -1, m_font, curLabelR, &rightFmt, &curBrush);

        // 当前点小圆
        float cx = mapX(last.time);
        float cy = (float)mapY(last.kps);
        Gdiplus::SolidBrush dotBrush(Gdiplus::Color(240, lc.r, lc.g, lc.b));
        g.FillEllipse(&dotBrush, cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    }

    g.ResetClip();

    // === 圆角边框 ===
    if (chartR > 0) {
        Gdiplus::Pen borderPen(Gdiplus::Color(50, 80, 80, 80), 0.5f);
        Gdiplus::GraphicsPath borderPath;
        MakeRoundRect(borderPath, 0, 0, w - 1, h - 1, chartR);
        g.DrawPath(&borderPen, &borderPath);
    }

    // 转到 HBITMAP
    Gdiplus::BitmapData bd;
    Gdiplus::Rect lockRect(0, 0, w, h);
    bmp.LockBits(&lockRect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bd);

    HDC hdcScreen = GetDC(nullptr);
    outMemDC = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth    = w;
    bi.bmiHeader.biHeight   = -h;
    bi.bmiHeader.biPlanes   = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    outBmp = CreateDIBSection(outMemDC, &bi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    SelectObject(outMemDC, outBmp);

    for (int y = 0; y < h; ++y) {
        const BYTE* src = static_cast<const BYTE*>(bd.Scan0) + y * bd.Stride;
        BYTE*       dst = static_cast<BYTE*>(pBits) + y * w * 4;
        for (int x = 0; x < w; ++x) {
            dst[x * 4 + 0] = src[x * 4 + 0];  // B
            dst[x * 4 + 1] = src[x * 4 + 1];  // G
            dst[x * 4 + 2] = src[x * 4 + 2];  // R
            dst[x * 4 + 3] = src[x * 4 + 3];  // A
        }
    }

    bmp.UnlockBits(&bd);
    ReleaseDC(nullptr, hdcScreen);
    return true;
}

// ========== 叠加层更新 ==========
void ChartUI::UpdateChart(const AppConfig& cfg, const KeyStateManager& ksm) {
    if (!m_hwnd) return;

    int w = cfg.chartW > 0 ? cfg.chartW : kChartW_def;
    int h = cfg.chartH > 0 ? cfg.chartH : kChartH_def;
    HBITMAP hBmp = nullptr;
    HDC     hMemDC = nullptr;

    if (!RenderChart(cfg, ksm, hBmp, hMemDC, w, h))
        return;
    if (!hBmp || !hMemDC) return;

    HDC hdcScreen = GetDC(nullptr);

    BLENDFUNCTION blend = {};
    blend.BlendOp             = AC_SRC_OVER;
    blend.BlendFlags          = 0;
    blend.SourceConstantAlpha = (BYTE)(cfg.overlayOpacity * 255);
    blend.AlphaFormat         = AC_SRC_ALPHA;

    SIZE  sz    = {w, h};
    POINT ptSrc = {0, 0};
    int cx = cfg.chartX;
    int cy = cfg.chartY;
    POINT ptDst = {cx, cy};

    UpdateLayeredWindow(m_hwnd, hdcScreen, &ptDst, &sz,
                        hMemDC, &ptSrc, 0, &blend, ULW_ALPHA);

    ReleaseDC(nullptr, hdcScreen);

    CleanupLastFrame();
    m_lastBmp   = hBmp;
    m_lastMemDC = hMemDC;
}

// ========== 窗口过程 ==========
LRESULT CALLBACK ChartUI::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lp);
        auto* self = static_cast<ChartUI*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        if (self) self->m_hwnd = hwnd;
    }
    auto* self = reinterpret_cast<ChartUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
    case WM_RBUTTONUP:
        if (self->m_notifyHwnd)
            PostMessageW(self->m_notifyHwnd, WM_APP + 2, 0, 0);
        return 0;

    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        self->m_dragging = true;
        GetCursorPos(&self->m_dragOffset);
        if (self->m_cfg) {
            self->m_dragOffset.x -= self->m_cfg->chartX;
            self->m_dragOffset.y -= self->m_cfg->chartY;
        }
        return 0;

    case WM_MOUSEMOVE:
        if (self->m_dragging && self->m_cfg) {
            POINT pt;
            GetCursorPos(&pt);
            int newX = pt.x - self->m_dragOffset.x;
            int newY = pt.y - self->m_dragOffset.y;
            self->m_cfg->chartX = newX;
            self->m_cfg->chartY = newY;
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
