#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <commctrl.h>
#include "config.h"
#include "display_ui.h"
class ChartUI;
class ThemeEditor;
class KeyStateManager;

struct SwatchData { RgbaColor* color = nullptr; };

class SettingsUI {
public:
    SettingsUI() = default;
    ~SettingsUI();
    bool Create(HINSTANCE hInst, AppConfig* cfg, DisplayUI* d, ChartUI* c, ThemeEditor* te, KeyStateManager* k);
    void Show(bool v);
    void SetConfigPath(const wchar_t* p) { m_cfgPath = p; }
    HWND GetHwnd() const { return m_hwnd; }
    void RefreshControls();
    void RebuildWindow();

    int  GetActiveTab() const { return m_activeTab; }
    void SwitchTab(int page);

private:
    static LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
    void OnCreate(HWND);
    void OnCommand(WPARAM wp, LPARAM lp);
    void OnHScroll(WPARAM wp, LPARAM lp);
    void OnDestroy();

    void RefreshKeyList();
    void UpdateColorSwatches(int idx);
    void OnAddKey(); void OnDeleteKey();
    void OnPickColor(int t); void OnPickBoxColor(int cid); void OnPickChartColor(int cid);
    void OnResetTotals(); void OnResetDefaults();
    void OnSave(); void ApplyTheme(int tid);

    void SyncKeySizeEdit(); void SyncSpacingEdit();
    void SyncHistoryHEdit(); void SyncGrowSpdEdit(); void SyncFloatSpdEdit();
    void SyncBlockMaxEdit(); void SyncBorderEdit(); void SyncOpacityEdit();
    void SyncTrackGapEdit(); void SyncBpmMergeEdit();
    void SyncTrackAlphaEdit(); void SyncBlockAlphaEdit();

    HWND m_hwnd=nullptr; HINSTANCE m_hInst=nullptr;
    AppConfig* m_cfg=nullptr; DisplayUI* m_dpy=nullptr;
    ChartUI* m_chart=nullptr; ThemeEditor* m_te=nullptr; KeyStateManager* m_ksm=nullptr;
    int m_selKey=-1; std::wstring m_cfgPath;
    HFONT m_hFont=nullptr, m_hBold=nullptr;

    enum { TAB_DISPLAY=0, TAB_LAYOUT, TAB_THEME, TAB_CHART, TAB_KEYS, TAB_COUNT=5 };
    int  m_activeTab=0;
    std::vector<HWND> m_pages[TAB_COUNT];

    // All controls
    HWND m_chkTotal,m_chkKPS,m_chkSummary,m_chkHistory,m_chkBPM,m_chkThrough;
    HWND m_rNote8,m_rNote16,m_rNote32,m_rNote64;
    HWND m_rLangCN,m_rLangEN,m_rLangJP;
    HWND m_trKeySize,m_edKeySize,m_trSpacing,m_edSpacing;
    HWND m_trHistH,m_edHistH,m_trGrow,m_edGrow,m_trFloat,m_edFloat;
    HWND m_trBlockMax,m_edBlockMax,m_trBorder,m_edBorder,m_trOpacity,m_edOpacity;
    HWND m_trTrackGap,m_edTrackGap,m_trBpmMerge,m_edBpmMerge;
    HWND m_trTrackAlpha,m_edTrackAlpha,m_trBlockAlpha,m_edBlockAlpha;
    HWND m_rTheme[16]; HWND m_btnThemeEdit;
    HWND m_rFps25,m_rFps45,m_rFps60,m_rFps90,m_rFps120;
    HWND m_swBox[6];
    HWND m_chkChart; HWND m_trChartTime,m_edChartTime;
    HWND m_trChartW,m_edChartW,m_trChartH,m_edChartH;
    HWND m_trChartML,m_edChartML,m_trChartMR,m_edChartMR;
    HWND m_trChartMT,m_edChartMT,m_trChartMB,m_edChartMB;
    HWND m_trChartR,m_edChartR;
    HWND m_swChartBg,m_swChartLine,m_lblChartBg,m_lblChartLine;
    HWND m_listKeys,m_btnAdd,m_btnDel;
    HWND m_swFont,m_swNormal,m_swPress,m_lblFont,m_lblNormal,m_lblPress;
    HWND m_btnSave,m_btnResetTotal,m_btnResetAll;

    static constexpr int kWinW=540, kWinH=620, kTabH=42, kCardPad=14;
};
