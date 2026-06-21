#pragma once

#include <windows.h>
#include <string>
#include "config.h"
#include "keyboard.h"
#include "renderer.h"

// State 界面 — 透明悬浮叠加层窗口（支持鼠标拖拽）
class DisplayUI {
public:
    DisplayUI();
    ~DisplayUI();

    bool Create(HINSTANCE hInst);
    void Show(bool visible);
    void UpdateOverlay(const AppConfig& cfg, const KeyStateManager& ksm);
    void MoveTo(int x, int y);
    HWND GetHwnd() const { return m_hwnd; }

    void SetClickThrough(bool enable);
    void SetTopMost(bool enable);
    void SetNotifyHwnd(HWND h) { m_notifyHwnd = h; }
    void SetConfig(AppConfig* cfg) { m_cfg = cfg; }
    void SetConfigPath(const wchar_t* path) { m_configPath = path; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    HWND        m_hwnd        = nullptr;
    HWND        m_notifyHwnd  = nullptr;
    AppConfig*  m_cfg         = nullptr;
    std::wstring m_configPath;
    Renderer    m_renderer;
    bool        m_clickThrough = false;

    // 拖拽状态
    bool  m_dragging   = false;
    POINT m_dragOffset = {0, 0};

    void CleanupLastFrame();
    HBITMAP m_lastBmp   = nullptr;
    HDC     m_lastMemDC = nullptr;
    uint64_t m_lastTopMostCheck = 0;  // 上次重新置顶时间（防全屏遮挡）
};
