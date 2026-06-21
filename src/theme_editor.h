#pragma once

#include <windows.h>
#include <string>
#include "config.h"

class SettingsUI;
class ThemeEditor {
public:
    ThemeEditor() = default;
    ~ThemeEditor();
    bool Create(HINSTANCE hInst, AppConfig* cfg);
    void Show(bool visible);
    void SetConfigPath(const wchar_t* p) { m_themePath = p; }
    void SetSettings(SettingsUI* s) { m_settings = s; }
    HWND GetHwnd() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void OnCreate(HWND hwnd);
    void OnCommand(WPARAM wp);
    void RefreshDisplay();
    void PickAndSet(RgbaColor* col);

    HWND         m_hwnd      = nullptr;
    HINSTANCE    m_hInst     = nullptr;
    AppConfig*   m_cfg       = nullptr;
    SettingsUI*  m_settings  = nullptr;
    std::wstring m_themePath;
    HFONT        m_hFont     = nullptr;
    int          m_selTheme  = 1;

    HWND m_combo;
    HWND m_swFont, m_swNormal, m_swPress, m_swBg, m_swLine;
    HWND m_lblFont, m_lblNormal, m_lblPress, m_lblBg, m_lblLine;
    HWND m_editName;
    HWND m_btnSave, m_btnClose;

    static constexpr int kSwW = 36, kSwH = 24;
};
