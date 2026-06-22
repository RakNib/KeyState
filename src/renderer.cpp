#include "renderer.h"
#include <cstring>
#include <algorithm>
#include <climits>

Renderer::Renderer() {
    Gdiplus::GdiplusStartupInput si;
    Gdiplus::GdiplusStartup(&m_gdiToken, &si, nullptr);

    m_fontFamily = new Gdiplus::FontFamily(L"Segoe UI");
    if (!m_fontFamily->IsAvailable()) {
        delete m_fontFamily;
        m_fontFamily = new Gdiplus::FontFamily();
    }
    m_keyFont  = new Gdiplus::Font(m_fontFamily, 26, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    m_statFont = new Gdiplus::Font(m_fontFamily, 12, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

    m_centerFormat = new Gdiplus::StringFormat();
    m_centerFormat->SetAlignment(Gdiplus::StringAlignmentCenter);
    m_centerFormat->SetLineAlignment(Gdiplus::StringAlignmentCenter);
    m_centerFormat->SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
}

Renderer::~Renderer() {
    delete m_centerFormat;
    delete m_statFont;
    delete m_keyFont;
    delete m_fontFamily;
    Gdiplus::GdiplusShutdown(m_gdiToken);
}

// ========== 缓动因子计算 ==========
float Renderer::EaseFactor(int keyCode, bool nowPressed) const {
    auto& st = const_cast<std::unordered_map<int, AnimState>&>(m_anims)[keyCode];

    // 检测状态变化
    if (nowPressed != st.prevPressed) {
        st.prevPressed = nowPressed;
        st.changeTime  = GetTickCount64();
    }

    DWORD64 elapsed = GetTickCount64() - st.changeTime;
    const DWORD64 duration = nowPressed ? 80 : 100; // 按下 80ms，松开 100ms

    float raw = (elapsed >= duration) ? 1.0f : (float)elapsed / (float)duration;

    // smoothstep(x) = x²(3 - 2x) : 0→1 平滑过渡
    auto smoothstep = [](float x) { return x * x * (3.0f - 2.0f * x); };

    float t = nowPressed
        ? smoothstep(raw)            // 按下: 0 → 1 缓入
        : 1.0f - smoothstep(raw);    // 松开: 1 → 0 缓出（从当前颜色消退）

    if (!nowPressed && elapsed >= duration) return 0.0f;
    if (nowPressed  && elapsed >= duration) return 1.0f;
    return (std::max)(0.0f, (std::min)(1.0f, t));
}

RgbaColor Renderer::LerpColor(const RgbaColor& a, const RgbaColor& b, float t) {
    return {
        (uint8_t)(a.r + (b.r - a.r) * t),
        (uint8_t)(a.g + (b.g - a.g) * t),
        (uint8_t)(a.b + (b.b - a.b) * t),
        (uint8_t)(a.a + (b.a - a.a) * t),
    };
}

// ========== 渲染 ==========
bool Renderer::Render(const AppConfig& cfg, const KeyStateManager& ksm,
                      HBITMAP& outBmp, HDC& outMemDC, int& outW, int& outH) {
    int nKeys = (int)cfg.keys.size();
    if (nKeys == 0) return false;

    int ks  = cfg.keySize;
    int gap = cfg.keySpacing;
    bool hasHistory  = cfg.showHistory && nKeys > 0;
    int  trackH      = hasHistory ? cfg.historyTrackH : 0;
    int  trackGap    = hasHistory ? cfg.historyTrackGap : 0;
    // 额外框：Total(showSummary) + KPS(showKPS) + BPM(showBPM)
    int  extraBoxes  = (cfg.showSummary?1:0) + (cfg.showKPS&&nKeys>0?1:0) + (cfg.showBPM&&nKeys>0?1:0);

    // === 自由模式 vs 常规模式尺寸计算 ===
    if (cfg.freeMode) {
        const int fm = 10;
        int faw = cfg.freeAreaW > ks ? cfg.freeAreaW : ks + 80;
        int fah = cfg.freeAreaH > ks ? cfg.freeAreaH : ks + 80;
        outW = faw + fm * 2;
        outH = fah + fm * 2;
    } else {
        int  nBoxes      = nKeys + extraBoxes;
        outW = nBoxes * ks + (nBoxes - 1) * gap + gap * 2;
        outH = ks + gap * 2 + trackH + trackGap;
    }

    Gdiplus::Bitmap bmp(outW, outH, PixelFormat32bppARGB);
    Gdiplus::Graphics g(&bmp);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    // 字体变更时热更新
    if (cfg.keyFontName != m_lastFontName) {
        m_lastFontName = cfg.keyFontName;
        delete m_fontFamily;
        m_fontFamily = new Gdiplus::FontFamily(cfg.keyFontName.c_str());
        if (!m_fontFamily->IsAvailable()) {
            delete m_fontFamily;
            m_fontFamily = new Gdiplus::FontFamily(L"Segoe UI");
            m_lastFontName = L"Segoe UI";
        }
        delete m_keyFont;
        delete m_statFont;
        m_keyFont  = new Gdiplus::Font(m_fontFamily, 26, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        m_statFont = new Gdiplus::Font(m_fontFamily, 12, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    }

    g.Clear(Gdiplus::Color(0, 0, 0, 0));

    // 汇总数据
    uint64_t sumTotal = 0;
    double   sumKPS   = 0.0;
    double   bpmKPS   = 0.0;
    for (int i = 0; i < nKeys; ++i) {
        sumTotal += ksm.GetTotal(cfg.keys[i].keyCode);
        double k = ksm.GetKPS(cfg.keys[i].keyCode);
        sumKPS += k;
    }
    // BPM 合并同时按下
    if (cfg.showBPM && nKeys > 0 && cfg.bpmMergeMs > 0) {
        std::vector<uint64_t> allTs;
        for (int i = 0; i < nKeys; ++i) {
            const auto& ts = ksm.GetPressTimestamps(cfg.keys[i].keyCode);
            allTs.insert(allTs.end(), ts.begin(), ts.end());
        }
        if (!allTs.empty()) {
            std::sort(allTs.begin(), allTs.end());
            uint64_t now = GetTickCount64();
            int merged = 0; uint64_t last = 0;
            for (auto t : allTs) {
                if (now - t > 1000) continue;
                if (last == 0 || t - last >= (uint64_t)cfg.bpmMergeMs) { merged++; last = t; }
            }
            bpmKPS = (double)merged;
        }
    } else bpmKPS = sumKPS;

    int keyY = trackH + gap + trackGap; // 按键框 Y 偏移（轨道 + 间距）

    // 先绘制轨道（在按键背景层之下）
    if (hasHistory && !cfg.freeMode) {
        for (int i = 0; i < nKeys; ++i) {
            int tx = gap + i * (ks + gap);
            DrawHistoryTrack(g, cfg, cfg.keys[i].keyCode, ksm, tx, gap, ks, trackH,
                             cfg.keys[i].colorPress);
        }
    }

    // === 自由模式渲染 ===
    if (cfg.freeMode) {
        int areaW = cfg.freeAreaW > ks ? cfg.freeAreaW : ks + 80;
        int areaH = cfg.freeAreaH > ks ? cfg.freeAreaH : ks + 80;
        const int margin = 10;

        // 绘制区域边界（可选）
        if (cfg.freeShowBoundary) {
            Gdiplus::Pen boundPen(Gdiplus::Color(120, 255, 100, 100), 1.0f);
            Gdiplus::SolidBrush boundFill(Gdiplus::Color(12, 255, 80, 80));
            g.DrawRectangle(&boundPen, margin, margin, areaW, areaH);
            g.FillRectangle(&boundFill, margin, margin, areaW, areaH);
            Gdiplus::SolidBrush labelBrush(Gdiplus::Color(160, 255, 140, 140));
            Gdiplus::Font markerFont(L"Segoe UI", 8, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            wchar_t bl[64];
            swprintf(bl, 64, L"Area (%d x %d)", areaW, areaH);
            g.DrawString(bl, -1, &markerFont,
                Gdiplus::RectF((float)(margin + 4), (float)(margin + 2), 160.0f, 12.0f),
                nullptr, &labelBrush);
        }

        // 限制每个元素在区域内绘制
        auto clampVal = [&](int v) -> int {
            if (v < 0) return 0;
            if (v + ks > areaW) return areaW - ks;
            return v;
        };
        auto clampValH = [&](int v) -> int {
            if (v < 0) return 0;
            if (v + ks > areaH) return areaH - ks;
            return v;
        };

        for (int i = 0; i < nKeys; ++i) {
            int cx = clampVal(cfg.keys[i].freeX);
            int cy = clampValH(cfg.keys[i].freeY);
            DrawKeyBox(g, cfg.keys[i], ksm,
                       cx + margin, cy + margin,
                       ks, cfg.keyRadius, cfg.keyBorderW,
                       cfg.showTotal, cfg.showKPS, cfg.keyFontSize);
        }
        wchar_t buf[32];

        if (cfg.showSummary && nKeys > 0) {
            int cx = clampVal(cfg.freeTotalX);
            int cy = clampValH(cfg.freeTotalY);
            swprintf(buf, 32, L"%llu", sumTotal);
            DrawDataBox(g, cx + margin, cy + margin, ks, cfg.keyRadius,
                        cfg.totalBoxBg, cfg.totalBoxFc, buf, L"Total");
        }
        if (cfg.showKPS && nKeys > 0) {
            int cx = clampVal(cfg.freeKPSX);
            int cy = clampValH(cfg.freeKPSY);
            swprintf(buf, 32, L"%d", (int)sumKPS);
            DrawDataBox(g, cx + margin, cy + margin, ks, cfg.keyRadius,
                        cfg.kpsBoxBg, cfg.kpsBoxFc, buf, L"KPS/s");
        }
        if (cfg.showBPM && nKeys > 0) {
            int cx = clampVal(cfg.freeBPMX);
            int cy = clampValH(cfg.freeBPMY);
            swprintf(buf, 32, L"%d", (int)(bpmKPS * 240.0 / cfg.bpmNoteDiv));
            wchar_t sub[32];
            swprintf(sub, 32, L"BPM | %d", cfg.bpmNoteDiv);
            DrawDataBox(g, cx + margin, cy + margin, ks, cfg.keyRadius,
                        cfg.bpmBoxBg, cfg.bpmBoxFc, buf, sub);
        }
    } else {
        // === 常规模式渲染 ===
        for (int i = 0; i < nKeys; ++i) {
            int x = gap + i * (ks + gap);
            DrawKeyBox(g, cfg.keys[i], ksm, x, keyY, ks, cfg.keyRadius, cfg.keyBorderW,
                       cfg.showTotal, cfg.showKPS, cfg.keyFontSize);
        }

        // 追加数据框
        int extraIdx = 0;
        wchar_t buf[32];

        if (cfg.showSummary && nKeys > 0) {
            int sx = gap + (nKeys + extraIdx) * (ks + gap);
            swprintf(buf, 32, L"%llu", sumTotal);
            DrawDataBox(g, sx, keyY, ks, cfg.keyRadius, cfg.totalBoxBg, cfg.totalBoxFc, buf, L"Total");
            extraIdx++;
        }

        if (cfg.showKPS && nKeys > 0) {
            int sx = gap + (nKeys + extraIdx) * (ks + gap);
            swprintf(buf, 32, L"%d", (int)sumKPS);
            DrawDataBox(g, sx, keyY, ks, cfg.keyRadius, cfg.kpsBoxBg, cfg.kpsBoxFc, buf, L"KPS/s");
            extraIdx++;
        }

        if (cfg.showBPM && nKeys > 0) {
            int sx = gap + (nKeys + extraIdx) * (ks + gap);
            swprintf(buf, 32, L"%d", (int)(bpmKPS * 240.0 / cfg.bpmNoteDiv));
            wchar_t sub[32];
            swprintf(sub, 32, L"BPM | %d", cfg.bpmNoteDiv);
            DrawDataBox(g, sx, keyY, ks, cfg.keyRadius, cfg.bpmBoxBg, cfg.bpmBoxFc, buf, sub);
            extraIdx++;
        }
    }

    Gdiplus::BitmapData bd;
    Gdiplus::Rect lockRect(0, 0, outW, outH);
    bmp.LockBits(&lockRect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bd);

    HDC hdcScreen = GetDC(nullptr);
    outMemDC = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth    = outW;
    bi.bmiHeader.biHeight   = -outH;
    bi.bmiHeader.biPlanes   = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    outBmp = CreateDIBSection(outMemDC, &bi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    SelectObject(outMemDC, outBmp);

    for (int y = 0; y < outH; ++y) {
        const BYTE* src = static_cast<const BYTE*>(bd.Scan0) + y * bd.Stride;
        BYTE*       dst = static_cast<BYTE*>(pBits) + y * outW * 4;
        for (int x = 0; x < outW; ++x) {
            BYTE b = src[x * 4 + 0];
            BYTE g = src[x * 4 + 1];
            BYTE r = src[x * 4 + 2];
            BYTE a = src[x * 4 + 3];
            dst[x * 4 + 0] = (BYTE)((b * a) / 255);
            dst[x * 4 + 1] = (BYTE)((g * a) / 255);
            dst[x * 4 + 2] = (BYTE)((r * a) / 255);
            dst[x * 4 + 3] = a;
        }
    }

    bmp.UnlockBits(&bd);
    ReleaseDC(nullptr, hdcScreen);
    return true;
}

// ========== 绘制单个按键框 ==========
void Renderer::DrawKeyBox(Gdiplus::Graphics& g, const KeyConfig& kc,
                           const KeyStateManager& ksm,
                           int x, int y, int size, int radius, int borderW,
                           bool showTotal, bool showKPS, int globalFontSize) {
    bool pressed = ksm.IsDown(kc.keyCode);

    // 缓动因子：0=完全未按，1=完全按下，中间平滑过渡
    float t = EaseFactor(kc.keyCode, pressed);

    // 插值颜色
    RgbaColor curBg   = LerpColor(kc.colorNormal, kc.colorPress, t);
    RgbaColor curFont = kc.colorFont; // 字体颜色不变

    int w = size, h = size;
    int bx = x, by = y; // 框不移动不缩放
    int r  = radius;
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;

    // 圆角矩形路径
    struct RoundedRect {
        Gdiplus::GraphicsPath path;
        RoundedRect(int px, int py, int pw, int ph, int r) {
            path.AddArc(px, py, r * 2, r * 2, 180, 90);
            path.AddArc(px + pw - r * 2, py, r * 2, r * 2, 270, 90);
            path.AddArc(px + pw - r * 2, py + ph - r * 2, r * 2, r * 2, 0, 90);
            path.AddArc(px, py + ph - r * 2, r * 2, r * 2, 90, 90);
            path.CloseFigure();
        }
        operator Gdiplus::GraphicsPath*() { return &path; }
    };

    // 光晕（缓动透明度）
    if (t > 0.001f) {
        // 外层柔光
        int outerAlpha = (int)(40 * t);
        if (outerAlpha > 0) {
            Gdiplus::SolidBrush outerGlow(Gdiplus::Color(
                (BYTE)outerAlpha, curBg.r, curBg.g, curBg.b));
            RoundedRect outerPath(bx - 5, by - 5, w + 10, h + 10, r + 2);
            g.FillPath(&outerGlow, outerPath);
        }
        // 中层光晕
        int midAlpha = (int)(80 * t);
        if (midAlpha > 0) {
            Gdiplus::SolidBrush midGlow(Gdiplus::Color(
                (BYTE)midAlpha, curBg.r, curBg.g, curBg.b));
            RoundedRect midPath(bx - 2, by - 2, w + 4, h + 4, r + 1);
            g.FillPath(&midGlow, midPath);
        }
        // 内层高亮边框
        int brightAlpha = (int)(220 * t);
        if (brightAlpha > 0) {
            Gdiplus::Color brightColor((BYTE)brightAlpha,
                (std::min)(curBg.r + 60, 255),
                (std::min)(curBg.g + 60, 255),
                (std::min)(curBg.b + 60, 255));
            Gdiplus::Pen highlightPen(brightColor, 2.0f);
            RoundedRect innerPath(bx + 2, by + 2, w - 4, h - 4, r - 1);
            g.DrawPath(&highlightPen, innerPath);
        }
    }

    // 主体背景（缓动颜色）
    Gdiplus::SolidBrush bgBrush(Gdiplus::Color(curBg.a, curBg.r, curBg.g, curBg.b));
    RoundedRect boxPath(bx, by, w, h, r);
    g.FillPath(&bgBrush, boxPath);

    // 边框
    if (borderW > 0) {
        Gdiplus::Color borderColor(180, curFont.r, curFont.g, curFont.b);
        Gdiplus::Pen borderPen(borderColor, (float)borderW);
        g.DrawPath(&borderPen, boxPath);
    }

    // 按键标签（全局字体）
    auto& fc = curFont;
    int fs = globalFontSize;
    if (fs < 8) fs = 8;
    Gdiplus::Font keyFont(m_fontFamily, (float)fs, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush fontBrush(Gdiplus::Color(fc.a, fc.r, fc.g, fc.b));
    int labelH = (int)(h * 0.6f);
    Gdiplus::RectF labelRect((float)bx, (float)(by + 2), (float)w, (float)labelH);
    g.DrawString(kc.label.c_str(), (int)kc.label.size(),
                 &keyFont, labelRect, m_centerFormat, &fontBrush);

    // 统计信息
    if (showTotal || showKPS) {
        std::wstring stats;
        if (showTotal) stats += std::to_wstring(ksm.GetTotal(kc.keyCode));
        if (showKPS) {
            if (!stats.empty()) stats += L" | ";
            wchar_t buf[32];
            swprintf(buf, 32, L"%d/s", (int)ksm.GetKPS(kc.keyCode));
            stats += buf;
        }
        Gdiplus::RectF statRect((float)bx, (float)(by + labelH),
                                 (float)w, (float)(h - labelH - 2));
        Gdiplus::SolidBrush statBrush(Gdiplus::Color(
            (BYTE)(fc.a * 0.7f), fc.r, fc.g, fc.b));
        g.DrawString(stats.c_str(), (int)stats.size(),
                     m_statFont, statRect, m_centerFormat, &statBrush);
    }
}

// ========== 通用数据框 ==========
void Renderer::DrawDataBox(Gdiplus::Graphics& g, int x, int y, int size, int radius,
                            RgbaColor bg, RgbaColor fc, const wchar_t* symbol,
                            const wchar_t* value, int /*unused*/) {
    int w = size, h = size, bx = x, by = y, r = radius;
    if (r * 2 > w) r = w / 2;
    struct RR { Gdiplus::GraphicsPath p; RR(int px,int py,int pw,int ph,int r) {
        p.AddArc(px,py,r*2,r*2,180,90); p.AddArc(px+pw-r*2,py,r*2,r*2,270,90);
        p.AddArc(px+pw-r*2,py+ph-r*2,r*2,r*2,0,90); p.AddArc(px,py+ph-r*2,r*2,r*2,90,90);
        p.CloseFigure(); } operator Gdiplus::GraphicsPath*(){return &p;} };
    Gdiplus::SolidBrush bgBrush(Gdiplus::Color(bg.a,bg.r,bg.g,bg.b));
    g.FillPath(&bgBrush, RR(bx,by,w,h,r));
    int labelH = (int)(h*0.55f);
    Gdiplus::SolidBrush fb(Gdiplus::Color(fc.a,fc.r,fc.g,fc.b));
    g.DrawString(symbol,(int)wcslen(symbol),m_keyFont,
        Gdiplus::RectF((float)bx,(float)(by+2),(float)w,(float)labelH),m_centerFormat,&fb);
    Gdiplus::SolidBrush sb(Gdiplus::Color((BYTE)(fc.a*0.7f),fc.r,fc.g,fc.b));
    g.DrawString(value,(int)wcslen(value),m_statFont,
        Gdiplus::RectF((float)bx,(float)(by+labelH),(float)w,(float)(h-labelH-2)),m_centerFormat,&sb);
}

// 前向声明
static void DrawRoundedBlock(Gdiplus::Graphics& g,
                              int cx, int cy, int cw, int ch, int r,
                              Gdiplus::Brush* brush);

// ========== 绘制历史轨道 ==========
void Renderer::DrawHistoryTrack(Gdiplus::Graphics& g, const AppConfig& cfg, int keyCode,
                                 const KeyStateManager& ksm,
                                 int x, int y, int keyW, int trackH,
                                 const RgbaColor& pressColor) {
    // 轨道背景
    int ta = (int)(cfg.historyTrackAlpha * 2.55f); if(ta<0)ta=0; if(ta>255)ta=255;
    Gdiplus::SolidBrush trackBg(Gdiplus::Color((BYTE)ta, 255, 255, 255));
    g.FillRectangle(&trackBg, x, y, keyW, trackH);

    // 轨道边界线
    if (cfg.historyShowLines) {
        Gdiplus::Pen edgePen(Gdiplus::Color(35, 255, 255, 255), 1.0f);
        g.DrawLine(&edgePen, x, y, x, y + trackH);
        g.DrawLine(&edgePen, x + keyW, y, x + keyW, y + trackH);
    }

    uint64_t now = GetTickCount64();
    const int   blockW  = keyW - 4;
    const int   cx      = x + 2;
    const float growSpeed  = (float)cfg.historyGrowSpeed  / 1000.0f; // px/ms
    const float floatSpeed = (float)cfg.historyFloatSpeed / 1000.0f;
    const int   maxBlockH  = trackH * cfg.historyBlockMax / 100;  // 轨道百分比上限

    // ===== 1. 正在按住的方块（底部向上增长） =====
    if (ksm.IsDown(keyCode)) {
        uint64_t startTime = ksm.GetPressStartTime(keyCode);
        if (startTime > 0) {
            uint64_t heldMs = now - startTime;
            int blockH = (int)(heldMs * growSpeed);
            if (blockH > maxBlockH) blockH = maxBlockH;
            if (blockH < 2) blockH = 2;

            int blockY = y + trackH - blockH;
            int ba = cfg.historyBlockAlpha; if(ba<0)ba=0; if(ba>255)ba=255;
            Gdiplus::SolidBrush blockBrush(Gdiplus::Color(
                (BYTE)ba, pressColor.r, pressColor.g, pressColor.b));
            DrawRoundedBlock(g, cx, blockY, blockW, blockH, 3, &blockBrush);
        }
    }

    // ===== 2. 已松开的方块（匀速上浮） =====
    const auto& durations = ksm.GetPressDurations(keyCode);
    for (auto const& ps : durations) {
        uint64_t age = now - ps.releaseTime; // 松开后经过的时间

        // 高度由按住时的增长决定
        int blockH = (int)(ps.durationMs * growSpeed);
        if (blockH > maxBlockH) blockH = maxBlockH;
        if (blockH < 2) blockH = 2;

        // 方块底部位置 = 轨道底部 - 已浮起距离
        float floated = age * floatSpeed;
        int blockY = y + trackH - blockH - (int)floated;

        // 完全浮出顶部则消失
        if (blockY + blockH <= y) continue;

        // 顶部裁剪
        if (blockY < y) {
            blockH -= (y - blockY);
            blockY  = y;
        }

        // 透明度：刚松开不透明，越远越淡
        int alpha = (int)(cfg.historyBlockAlpha * (1.0f - (floated / (trackH + blockH)) * 0.7f));
        if (alpha < 15) alpha = 15;
        if (alpha > cfg.historyBlockAlpha) alpha = cfg.historyBlockAlpha;

        Gdiplus::SolidBrush blockBrush(Gdiplus::Color(
            (BYTE)alpha, pressColor.r, pressColor.g, pressColor.b));
        DrawRoundedBlock(g, cx, blockY, blockW, blockH, 3, &blockBrush);
    }
}

// 绘制圆角矩形方块
static void DrawRoundedBlock(Gdiplus::Graphics& g,
                              int cx, int cy, int cw, int ch, int r,
                              Gdiplus::Brush* brush) {
    Gdiplus::GraphicsPath path;
    if (r * 2 > cw) r = cw / 2;
    if (r * 2 > ch) r = ch / 2;
    path.AddArc(cx, cy, r * 2, r * 2, 180, 90);
    path.AddArc(cx + cw - r * 2, cy, r * 2, r * 2, 270, 90);
    path.AddArc(cx + cw - r * 2, cy + ch - r * 2, r * 2, r * 2, 0, 90);
    path.AddArc(cx, cy + ch - r * 2, r * 2, r * 2, 90, 90);
    path.CloseFigure();
    g.FillPath(brush, &path);
}
