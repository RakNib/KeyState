#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <deque>
#include <string>
#include "config.h"
#include "keyboard.h"

// Chart 界面 — KPS 实时折线图窗口（透明叠加层 + 可拖拽）
class ChartUI {
public:
    ChartUI();
    ~ChartUI();

    bool Create(HINSTANCE hInst);
    void Show(bool visible);
    void UpdateChart(const AppConfig& cfg, const KeyStateManager& ksm);
    HWND GetHwnd() const { return m_hwnd; }

    void SetNotifyHwnd(HWND h) { m_notifyHwnd = h; }
    void SetConfig(AppConfig* cfg) { m_cfg = cfg; }
    void SetConfigPath(const wchar_t* path) { m_configPath = path; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND          m_hwnd       = nullptr;
    HWND          m_notifyHwnd = nullptr;
    AppConfig*    m_cfg        = nullptr;
    std::wstring  m_configPath;

    // GDI+
    ULONG_PTR           m_gdiToken = 0;
    Gdiplus::Font*      m_font     = nullptr;
    Gdiplus::Font*      m_smallFont = nullptr;
    Gdiplus::StringFormat* m_rightFormat = nullptr;

    // 数据历史 (时间戳, KPS)
    struct DataPoint { uint64_t time; double kps; };
    std::deque<DataPoint> m_history;
    uint64_t m_lastSample = 0;
    float   m_yMaxSmooth  = 20.0f;  // Y轴上限（动态平滑缩放）

    // 拖拽
    bool  m_dragging   = false;
    POINT m_dragOffset = {0, 0};

    // 上一帧清理
    HBITMAP m_lastBmp   = nullptr;
    HDC     m_lastMemDC = nullptr;
    void CleanupLastFrame();

    // 渲染折线图到 GDI+ Bitmap
    bool RenderChart(const AppConfig& cfg, const KeyStateManager& ksm,
                     HBITMAP& outBmp, HDC& outMemDC, int w, int h);
};
