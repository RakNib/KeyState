#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <unordered_map>
#include "config.h"
#include "keyboard.h"

// 每个按键的颜色缓动状态
struct AnimState {
    bool     prevPressed  = false;
    DWORD64  changeTime   = 0;   // GetTickCount64 时间戳
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Render(const AppConfig& cfg, const KeyStateManager& ksm,
                HBITMAP& outBmp, HDC& outMemDC, int& outW, int& outH);

private:
    ULONG_PTR              m_gdiToken     = 0;
    Gdiplus::FontFamily*   m_fontFamily   = nullptr;
    Gdiplus::Font*         m_keyFont      = nullptr;
    Gdiplus::Font*         m_statFont     = nullptr;
    Gdiplus::StringFormat* m_centerFormat = nullptr;

    // 动画状态映射 keyCode → AnimState
    std::unordered_map<int, AnimState> m_anims;
    std::wstring m_lastFontName;       // 当前加载的字体名，用于检测变更

    // 计算缓动因子 (0~1)，按下=100ms 缓入，松开=150ms 缓出
    float EaseFactor(int keyCode, bool nowPressed) const;

    // 颜色线性插值
    static RgbaColor LerpColor(const RgbaColor& a, const RgbaColor& b, float t);

    void DrawKeyBox(Gdiplus::Graphics& g, const KeyConfig& kc, const KeyStateManager& ksm,
                    int x, int y, int w, int h, int radius, int borderW,
                    bool showTotal, bool showKPS, int globalFontSize);
    void DrawHistoryTrack(Gdiplus::Graphics& g, const AppConfig& cfg, int keyCode,
                          const KeyStateManager& ksm, int x, int y, int keyW, int trackH,
                          const RgbaColor& pressColor);
    void DrawDataBox(Gdiplus::Graphics& g, int x, int y, int w, int h, int radius,
                     RgbaColor bg, RgbaColor fc, const wchar_t* symbol,
                     const wchar_t* value, int symbolSz = 0);

};
