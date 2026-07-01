#pragma once
#include <windows.h>
#include <string>
#include "config.h"
class DisplayUI;
class ChartUI;
class ThemeEditor;
class KeyStateManager;

// 按键捕获：WndProc 写入，RenderImGui 读取
extern int g_capturedVK;

// V1.6: ImGui 设置面板
class SettingsUI {
public:
    SettingsUI();
    ~SettingsUI();

    bool Create(HINSTANCE hInst, AppConfig* cfg, DisplayUI* display, ChartUI* chart,
                ThemeEditor* te, KeyStateManager* ksm);
    void Show(bool visible);
    bool IsVisible() const { return m_visible; }

    void SetConfigPath(const wchar_t* path) { m_configPath = path; }
    HWND GetHwnd() const { return m_hwnd; }

    // 模态对话框保护（由主循环检查，避免嵌套崩溃）
    static bool   s_inModalDialog;

    bool RenderImGui();

    void RefreshControls() {}
    void RebuildWindow() {}
    void UpdateRecordStatus(bool recording, uint64_t startTime);
    void CycleTheme(int direction);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void OnCreate(HWND hwnd);
    void OnDestroy();

    void DrawPageBasic();
    void DrawPageTrack();
    void DrawPageKeys();
    void DrawPageTheme();
    void DrawPageChart();
    void DrawPageFreeMode();
    void DrawPageRecording();
    void DrawPageHotkeys();

    void ApplyTheme(int tid);
    void OnSave();

    HWND          m_hwnd        = nullptr;
    HINSTANCE     m_hInst       = nullptr;
    AppConfig*    m_cfg         = nullptr;
    DisplayUI*    m_display     = nullptr;
    ChartUI*      m_chart       = nullptr;
    ThemeEditor*  m_themeEditor = nullptr;
    KeyStateManager* m_ksm      = nullptr;
    std::wstring  m_configPath;
    bool          m_visible     = false;

    int           m_activeTab   = 0;
    int           m_selectedKey = -1;

    // ImGui 按键捕获状态（无需嵌套消息循环）
    bool          m_capturingKey    = false;
    bool          m_capturingRecord = false;

    // 录制状态
    bool          m_recording   = false;
    uint64_t      m_recStartTime = 0;
};
