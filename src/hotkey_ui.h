#pragma once

#include <windows.h>
#include "config.h"

// 快捷键编辑器 — 查看和修改所有全局快捷键
class HotkeyEditor {
public:
    HotkeyEditor() = default;
    ~HotkeyEditor();

    bool Create(HINSTANCE hInst, AppConfig* cfg, HWND notifyHwnd);
    void Show(bool visible);
    HWND GetHwnd() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void OnCreate(HWND hwnd);
    void OnCommand(WPARAM wp);
    void RefreshKeyButtons();
    void CaptureHotkey(int idx);

    HWND         m_hwnd       = nullptr;
    HINSTANCE    m_hInst      = nullptr;
    AppConfig*   m_cfg        = nullptr;
    HWND         m_notifyHwnd = nullptr;
    HFONT        m_hFont      = nullptr;

    // 7 个快捷键按钮
    HWND m_btnKeys[7] = {};

    struct HotkeyEntry {
        const wchar_t* label;
        int* vkPtr;
    };
    HotkeyEntry m_entries[7];
};
