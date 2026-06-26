#pragma once

#include <windows.h>
#include <string>
#include <commctrl.h>
#include "config.h"
#include "display_ui.h"
class ChartUI;
class ThemeEditor;
class HotkeyEditor;
class KeyStateManager;

struct SwatchData { RgbaColor* color = nullptr; };

// Settings 界面 — 配置管理窗口
class SettingsUI {
public:
    SettingsUI();
    ~SettingsUI();

    bool Create(HINSTANCE hInst, AppConfig* cfg, DisplayUI* display, ChartUI* chart, ThemeEditor* te, KeyStateManager* ksm);
    void Show(bool visible);
    void SetConfigPath(const wchar_t* path) { m_configPath = path; }
    HWND GetHwnd() const { return m_hwnd; }

    void RefreshControls();
    void RebuildWindow();
    void UpdateRecordStatus(bool recording, uint64_t startTime);
    void CycleTheme(int direction);  // direction: 1=下一个, -1=上一个
    void SetHotkeyEditor(HotkeyEditor* he) { m_hotkeyEditor = he; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void OnCreate(HWND hwnd);
    void OnCommand(WPARAM wp, LPARAM lp);
    void OnHScroll(WPARAM wp, LPARAM lp);
    void OnVScroll(WPARAM wp, LPARAM lp);
    void OnMouseWheel(WPARAM wp);
    void OnDestroy();

    void RefreshKeyList();
    void UpdateColorSwatches(int selIdx);
    void OnAddKey();
    void OnDeleteKey();
    void OnPickColor(int target);
    void OnPickBoxColor(int cid);
    void OnPickChartColor(int cid);
    void OnResetTotals();
    void OnResetDefaults();
    void OnSave();
    void ApplyTheme(int tid);
    void SyncSpacingEdit();
    void SyncHistoryHEdit();
    void SyncGrowSpdEdit();
    void SyncFloatSpdEdit();
    void SyncBlockMaxEdit();
    void SyncBorderEdit();
    void SyncOpacityEdit();
    void SyncTrackGapEdit();
    void SyncBpmMergeEdit();
    void SyncTrackAlphaEdit();
    void SyncBlockAlphaEdit();

    HWND          m_hwnd        = nullptr;
    HINSTANCE     m_hInst       = nullptr;
    AppConfig*    m_cfg         = nullptr;
    DisplayUI*    m_display     = nullptr;
    ChartUI*      m_chart       = nullptr;
    ThemeEditor*  m_themeEditor = nullptr;
    HotkeyEditor* m_hotkeyEditor = nullptr;
    KeyStateManager* m_ksm      = nullptr;
    int           m_selectedKey = -1;
    std::wstring  m_configPath;

    // 控件句柄
    HWND m_chkTotal, m_chkKPS, m_chkSummary, m_chkHistory, m_chkTrackLines, m_chkBPM, m_chkThrough, m_chkTopMost;
    HWND m_radioNote8, m_radioNote16, m_radioNote32, m_radioNote64;
    HWND m_radioLangCN, m_radioLangEN, m_radioLangJP;
    HWND m_trackSpacing, m_editSpacing;
    HWND m_trackHistoryH, m_editHistoryH;
    HWND m_trackGrowSpd,  m_editGrowSpd;
    HWND m_trackFloatSpd, m_editFloatSpd;
    HWND m_trackBlockMax, m_editBlockMax;
    HWND m_trackBorder,   m_editBorder;
    HWND m_trackOpacity,  m_editOpacity;
    HWND m_trackTrackGap, m_editTrackGap;
    HWND m_trackBpmMerge, m_editBpmMerge;
    HWND m_trackTrackAlpha, m_editTrackAlpha;
    HWND m_trackBlockAlpha, m_editBlockAlpha;
    HWND m_radioTheme0, m_radioTheme1, m_radioTheme2, m_radioTheme3;
    HWND m_radioTheme4, m_radioTheme5, m_radioTheme6, m_radioTheme7;
    HWND m_radioTheme8, m_radioTheme9, m_radioTheme10, m_radioTheme11;
    HWND m_radioTheme12, m_radioTheme13, m_radioTheme14, m_radioTheme15;
    HWND m_radioFps25, m_radioFps45, m_radioFps60, m_radioFps90, m_radioFps120;
    HWND m_listKeys, m_btnAdd, m_btnDel;
    HWND m_swFont, m_swNormal, m_swPress;
    HWND m_lblFont, m_lblNormal, m_lblPress;
    HWND m_btnSave, m_btnResetTotal, m_btnResetAll, m_btnThemeEdit, m_btnFont;
    // Tab 导航
    HWND m_tabCtrl;
    int  m_activeTab = 0;
    std::vector<HWND> m_tabCtrls[5];  // 每页的控件列表
    void SwitchTab(int page);
    enum { TAB_DISPLAY=0, TAB_LAYOUT, TAB_THEME, TAB_CHART, TAB_KEYS, TAB_COUNT=5 };
    HWND m_swBox[6];
    // V1.5: 数据框自定义宽高
    HWND m_editTotalBoxW, m_editTotalBoxH;
    HWND m_editKpsBoxW,   m_editKpsBoxH;
    HWND m_editBpmBoxW,   m_editBpmBoxH;
    // Chart 控件
    HWND m_chkChart, m_chkChartGrid;
    HWND m_radioChartLine, m_radioChartScatter, m_radioChartBar;
    HWND m_chkChartGradient;
    HWND m_trackChartTime, m_editChartTime;
    HWND m_trackChartW, m_editChartW;
    HWND m_trackChartH, m_editChartH;
    HWND m_trackChartML, m_editChartML;
    HWND m_trackChartMR, m_editChartMR;
    HWND m_trackChartMT, m_editChartMT;
    HWND m_trackChartMB, m_editChartMB;
    HWND m_swChartBg, m_swChartLine, m_lblChartBg, m_lblChartLine;
    HWND m_trackChartRadius, m_editChartRadius;
    HWND m_chkChartSnap;
    HWND m_trackChartSnapX, m_editChartSnapX;
    HWND m_trackChartSnapY, m_editChartSnapY;
    // V1.5: 自定义按键名称 + 宽高
    HWND m_editKeyName;
    HWND m_editKeyWidth, m_editKeyHeight;
    // 自由模式
    HWND m_radioNormalMode, m_radioFreeMode;
    HWND m_trackFreeAreaW, m_editFreeAreaW;
    HWND m_trackFreeAreaH, m_editFreeAreaH;
    HWND m_chkFreeBoundary;
    // V1.5: 自由模式网格吸附
    HWND m_chkGridSnap;
    HWND m_trackGridSize, m_editGridSize;
    // 录制
    HWND m_btnRecordHotkey;
    HWND m_lblRecordStatus;

    HFONT m_hFont      = nullptr;
    int   m_scrollY    = 0;       // 滚动偏移
    int   m_contentH   = 0;       // 内容总高度
    static constexpr int kVisibleH = 780; // 可见窗口高度
};
