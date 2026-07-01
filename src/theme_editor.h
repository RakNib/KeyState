#pragma once
#include <windows.h>
#include <string>
#include "config.h"
class SettingsUI;

// V1.6: ImGui 主题编辑器
class ThemeEditor {
public:
    ThemeEditor();
    ~ThemeEditor();

    bool Create(HINSTANCE hInst, AppConfig* cfg);
    void Show(bool visible);
    bool IsVisible() const { return m_visible; }

    void SetConfigPath(const wchar_t* path) { m_configPath = path; }
    void SetSettings(SettingsUI* s) { m_settings = s; }

    // 由 SettingsUI 在帧内调用渲染
    void RenderContent();

private:
    void OnSave();

    HINSTANCE     m_hInst    = nullptr;
    AppConfig*    m_cfg      = nullptr;
    SettingsUI*   m_settings = nullptr;
    std::wstring  m_configPath;
    bool          m_visible  = false;
    int           m_selTheme = 0;
};
