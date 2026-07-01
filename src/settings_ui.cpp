#include "settings_ui.h"
#include "display_ui.h"
#include "chart_ui.h"
#include "theme_editor.h"
#include "keyboard.h"
#include "lang.h"
#include "imgui.h"
#include "imgui_setup.h"
#include <cstdio>
#include <shlobj.h>
#include <vector>
#include <string>
#include <functional>

// ========== 多语言辅助函数 ==========
static const char* S(int key) {
    static char buf[64][256];
    static int idx = 0;
    idx = (idx + 1) % 64;
    buf[idx][0] = '\0';
    const wchar_t* ws = LANG(key);
    if (!ws) return "";
    WideCharToMultiByte(CP_UTF8, 0, ws, -1, buf[idx], 256, nullptr, nullptr);
    return buf[idx];
}

static const char* ToUtf8(const std::wstring& ws) {
    static char buf[64][1024];
    static int idx = 0;
    idx = (idx + 1) % 64;
    buf[idx][0] = '\0';
    if (ws.empty()) return "";
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, buf[idx], 1024, nullptr, nullptr);
    return buf[idx];
}

static ImVec4 ToImCol(const RgbaColor& c) {
    return ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}
static RgbaColor ToRgba(const ImVec4& v) {
    return {(uint8_t)(v.x * 255), (uint8_t)(v.y * 255), (uint8_t)(v.z * 255), (uint8_t)(v.w * 255)};
}

static void GetKeyNameText(int vk, char* buf, int bufSize) {
    wchar_t keyName[64] = {};
    UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    bool ext = false;
    switch (vk) {
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
        case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
        case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK: ext = true;
    }
    LONG lp = (sc << 16) | (ext ? 0x01000000 : 0);
    if (GetKeyNameTextW(lp, keyName, 64) > 0) {
        WideCharToMultiByte(CP_UTF8, 0, keyName, -1, buf, bufSize, nullptr, nullptr);
        return;
    }
    snprintf(buf, bufSize, "VK_%d", vk);
}

// ========== 全局 ==========
int g_capturedVK = 0;
bool SettingsUI::s_inModalDialog = false;

// 捕获状态：0=无, 1=添加按键, 2=录制快捷键
static int s_captureState = 0;
// 快捷键设置页捕获状态：-1=无
static int s_hkCapturing = -1;

// ========== 统一滑块布局：标签(左对齐) + 滑块 + 手动输入 + 空行 ==========
static void SliderRow(const char* label, int* val, int lo, int hi,
                      std::function<void()> onSave, int labelW) {
    ImGui::TextUnformatted(label);
    ImGui::SameLine((float)labelW);
    ImGui::SetNextItemWidth(210);
    char sid[64]; snprintf(sid, 64, "##s%s", label);
    if (ImGui::SliderInt(sid, val, lo, hi, "%d")) {}
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        if (*val < lo) *val = lo;
        if (*val > hi) *val = hi;
        if (onSave) onSave();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(85);
    char iid[64]; snprintf(iid, 64, "##i%s", label);
    if (ImGui::InputInt(iid, val, 1, 10)) {
        if (*val < lo) *val = lo;
        if (*val > hi) *val = hi;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        if (onSave) onSave();
    }
    ImGui::Spacing();
}

// ========== 构造/析构 ==========
SettingsUI::SettingsUI()  = default;
SettingsUI::~SettingsUI() { if (m_hwnd) DestroyWindow(m_hwnd); }

bool SettingsUI::Create(HINSTANCE hInst, AppConfig* cfg, DisplayUI* display,
                        ChartUI* chart, ThemeEditor* te, KeyStateManager* ksm) {
    m_hInst       = hInst;
    m_cfg         = cfg;
    m_display     = display;
    m_chart       = chart;
    m_themeEditor = te;
    m_ksm         = ksm;
    static const wchar_t* CLASS_NAME = L"KeyStateSettingsImgui";
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc); wc.lpfnWndProc = WndProc; wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME; wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassExW(&wc);
    RECT cr = {0, 0, 660, 650};
    AdjustWindowRect(&cr, WS_OVERLAPPEDWINDOW, FALSE);
    m_hwnd = CreateWindowExW(0, CLASS_NAME, L"KeyState V1.6",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        cr.right - cr.left, cr.bottom - cr.top, nullptr, nullptr, hInst, this);
    if (!m_hwnd) return false;
    if (!ImGui_Init(m_hwnd)) { DestroyWindow(m_hwnd); m_hwnd = nullptr; return false; }
    return true;
}

void SettingsUI::Show(bool visible) {
    m_visible = visible;
    if (!m_hwnd) return;
    if (visible) {
        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
    } else {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

// ========== 窗口过程 ==========
LRESULT CALLBACK SettingsUI::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    SettingsUI* self = nullptr;
    if (msg == WM_CREATE) {
        auto* cs = (CREATESTRUCT*)lp;
        self = (SettingsUI*)cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        self->m_hwnd = hwnd;
    } else {
        self = (SettingsUI*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }
    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);

    // 按键捕获
    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        int vk = (int)wp;
        if (vk != 229) g_capturedVK = vk;
    }

    // ImGui 输入处理
    if (ImGui_ProcessMessage(hwnd, msg, wp, lp))
        return true;

    switch (msg) {
    case WM_CLOSE: self->Show(false); return 0;
    case WM_DESTROY: ImGui_Shutdown(); self->m_hwnd = nullptr; return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ========== 捕获完成回调 ==========
static void OnKeyCaptured(int vk, AppConfig* cfg, SettingsUI* self) {
    KeyboardHook::SetBlocked(false);
    MouseHook::SetBlocked(false);
    g_capturedVK = 0;
    s_captureState = 0;
    if (vk == VK_ESCAPE) return;
    if (cfg) {
        // 添加按键
        bool dup = false;
        for (auto& kc : cfg->keys) { if (kc.keyCode == vk) { dup = true; break; } }
        if (!dup) {
            char keyName[64] = {}; GetKeyNameText(vk, keyName, 64);
            std::wstring wname;
            int len = MultiByteToWideChar(CP_UTF8, 0, keyName, -1, nullptr, 0);
            wname.resize(len);
            MultiByteToWideChar(CP_UTF8, 0, keyName, -1, &wname[0], len);
            while (!wname.empty() && wname.back() == L'\0') wname.pop_back();
            KeyConfig kc; kc.keyCode = vk; kc.label = wname;
            kc.customW = 64; kc.customH = 64;
            cfg->keys.push_back(kc);
        }
    }
}

// ========== ImGui 渲染主入口（含主题编辑器/快捷键编辑器集成） ==========
bool SettingsUI::RenderImGui() {
    if (!m_visible || !m_hwnd || !m_cfg) return false;
    if (s_inModalDialog) return false;

    ImGui_BeginFrame();

    // ===== 设置面板主窗口 =====
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(660, 650), ImGuiCond_Once);
    ImGui::Begin("KeyState V1.6", &m_visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

    // ImGui 关闭按钮联动隐藏
    if (!m_visible) { ImGui::End(); ImGui_EndFrame(); return true; }

    // 使用 ## 隐藏 ID 确保语言变化不导致 ImGui 重新识别标签
    static int s_tab = 0;
    m_activeTab = s_tab;

    if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
        char tabLabel[64];
        snprintf(tabLabel, 64, "%s##t0", S(83));
        if (ImGui::BeginTabItem(tabLabel)) { s_tab = 0; DrawPageBasic();  ImGui::EndTabItem(); }
        snprintf(tabLabel, 64, "%s##t1", S(84));
        if (ImGui::BeginTabItem(tabLabel)) { s_tab = 1; DrawPageTrack();  ImGui::EndTabItem(); }
        snprintf(tabLabel, 64, "%s##t2", S(87));
        if (ImGui::BeginTabItem(tabLabel)) { s_tab = 2; DrawPageKeys();   ImGui::EndTabItem(); }
        snprintf(tabLabel, 64, "%s##t3", S(85));
        if (ImGui::BeginTabItem(tabLabel)) { s_tab = 3; DrawPageTheme();  ImGui::EndTabItem(); }
        snprintf(tabLabel, 64, "%s##t4", S(86));
        if (ImGui::BeginTabItem(tabLabel)) { s_tab = 4; DrawPageChart();  ImGui::EndTabItem(); }
        snprintf(tabLabel, 64, "%s##t5", S(98));
        if (ImGui::BeginTabItem(tabLabel)) { s_tab = 5; DrawPageRecording(); ImGui::EndTabItem(); }
        snprintf(tabLabel, 64, "%s##t6", S(151));
        if (ImGui::BeginTabItem(tabLabel)) { s_tab = 6; DrawPageHotkeys(); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }

    // === 按键捕获弹窗（每帧检查，持续显示直到捕获完成） ===
    if (s_captureState == 1 || s_captureState == 2) {
        const char* popupName = (s_captureState == 1) ? S(135) : S(136);
        ImGui::OpenPopup(popupName);
        if (ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(S(28));
            int vk = g_capturedVK;
            if (vk > 0) {
                if (s_captureState == 1) {
                    OnKeyCaptured(vk, m_cfg, this);
                } else {
                    KeyboardHook::SetBlocked(false);
                    MouseHook::SetBlocked(false);
                    g_capturedVK = 0;
                    s_captureState = 0;
                    if (vk != VK_ESCAPE) { m_cfg->recordingHotkeyVK = vk; OnSave(); }
                }
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button(S(133), ImVec2(80, 0))) {
                KeyboardHook::SetBlocked(false);
                MouseHook::SetBlocked(false);
                g_capturedVK = 0;
                s_captureState = 0;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();

    // ===== 同一帧内渲染主题编辑器（如有需要） =====
    if (m_themeEditor && m_themeEditor->IsVisible())
        m_themeEditor->RenderContent();

    ImGui_EndFrame();
    return true;
}

// ========== 基本设置页 ==========
void SettingsUI::DrawPageBasic() {
    if (!m_cfg) return;
    ImGui::SeparatorText(S(24));
    int langs[3] = {0, 1, 2};
    const char* langLabels[] = {S(25), S(26), S(27)};
    for (int i = 0; i < 3; ++i) {
        if (i > 0) ImGui::SameLine();
        if (ImGui::RadioButton(langLabels[i], &m_cfg->lang, langs[i])) { SetLanguage(m_cfg->lang); OnSave(); }
    }
    ImGui::Spacing(); ImGui::SeparatorText(S(1));
    if (ImGui::Checkbox(S(2), &m_cfg->showTotal)) OnSave();
    ImGui::SameLine(120);
    if (ImGui::Checkbox(S(3), &m_cfg->showKPS)) OnSave();
    ImGui::SameLine(240);
    if (ImGui::Checkbox(S(4), &m_cfg->showSummary)) OnSave();
    if (ImGui::Checkbox(S(6), &m_cfg->showBPM)) OnSave();
    ImGui::SameLine(120);
    if (ImGui::Checkbox(S(7), &m_cfg->clickThrough)) {
        if (m_display) m_display->SetClickThrough(m_cfg->clickThrough);
        if (m_cfg->freeMode && m_cfg->clickThrough) m_cfg->freeShowBoundary = false;
        OnSave();
    }
    ImGui::SameLine(240);
    if (ImGui::Checkbox(S(88), &m_cfg->alwaysOnTop)) {
        if (m_display) m_display->SetTopMost(m_cfg->alwaysOnTop);
        if (m_chart) m_chart->SetTopMost(m_cfg->alwaysOnTop);
        OnSave();
    }

    ImGui::Spacing(); ImGui::SeparatorText("BPM");
    ImGui::TextUnformatted(S(138)); ImGui::SameLine(80);
    int noteDivs[] = {8, 16, 32, 64};
    const char* noteLabels[] = {S(37), S(38), S(39), S(40)};
    for (int i = 0; i < 4; ++i) {
        if (i > 0) ImGui::SameLine();
        if (ImGui::RadioButton(noteLabels[i], &m_cfg->bpmNoteDiv, noteDivs[i])) OnSave();
    }
    SliderRow(S(44), &m_cfg->bpmMergeMs, 0, 100, [&]{ OnSave(); }, 160);

    ImGui::Spacing(); ImGui::SeparatorText(S(146));
    ImGui::Text("%s", ToUtf8(m_cfg->keyFontName)); ImGui::SameLine();
    if (ImGui::Button(S(140))) {
        LOGFONTW lf = {}; lf.lfHeight = -16;
        wcscpy_s(lf.lfFaceName, LF_FACESIZE, m_cfg->keyFontName.c_str());
        CHOOSEFONTW cf = {sizeof(cf)}; cf.hwndOwner = m_hwnd; cf.lpLogFont = &lf;
        cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOVECTORFONTS;
        s_inModalDialog = true;
        if (ChooseFontW(&cf)) { m_cfg->keyFontName = lf.lfFaceName; OnSave(); }
        s_inModalDialog = false;
    }

    ImGui::Spacing(); ImGui::SeparatorText("FPS");
    int fpsVals[] = {25, 45, 60, 90, 120};
    const char* fpsLabels[] = {"25", "45", "60", "90", "120"};
    for (int i = 0; i < 5; ++i) {
        if (i > 0) ImGui::SameLine();
        if (ImGui::RadioButton(fpsLabels[i], &m_cfg->fps, fpsVals[i])) OnSave();
    }

    // 不透明度：使用 0~100 整数，自动转换
    ImGui::Spacing(); ImGui::SeparatorText(S(145));
    int opacityPct = (int)(m_cfg->overlayOpacity * 100);
    ImGui::TextUnformatted(S(54)); ImGui::SameLine(160);
    ImGui::SetNextItemWidth(220);
    if (ImGui::SliderInt("##sOpacity", &opacityPct, 0, 100, "%d%%")) {
        m_cfg->overlayOpacity = opacityPct / 100.0f;  // 同步到配置，防止下帧重置
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) { OnSave(); }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("##iOpacity", &opacityPct, 1, 10)) {
        if (opacityPct < 0) opacityPct = 0;
        if (opacityPct > 100) opacityPct = 100;
        m_cfg->overlayOpacity = opacityPct / 100.0f;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) { OnSave(); }

    ImGui::Spacing(); ImGui::SeparatorText(S(152));
    int curTheme = m_cfg->uiTheme;
    if (ImGui::RadioButton(S(153), curTheme == 0)) { m_cfg->uiTheme = 0; ApplyImGuiTheme(0); OnSave(); }
    ImGui::SameLine();
    if (ImGui::RadioButton(S(154), curTheme == 1)) { m_cfg->uiTheme = 1; ApplyImGuiTheme(1); OnSave(); }
    ImGui::SameLine();
    if (ImGui::RadioButton(S(155), curTheme == 2)) { m_cfg->uiTheme = 2; ApplyImGuiTheme(2); OnSave(); }
    ImGui::SameLine();
    if (ImGui::RadioButton(S(156), curTheme == 3)) { m_cfg->uiTheme = 3; ApplyImGuiTheme(3); OnSave(); }
    ImGui::SameLine();
    if (ImGui::RadioButton(S(157), curTheme == 4)) { m_cfg->uiTheme = 4; ApplyImGuiTheme(4); OnSave(); }
    ImGui::SameLine();
    if (ImGui::RadioButton(S(158), curTheme == 5)) { m_cfg->uiTheme = 5; ApplyImGuiTheme(5); OnSave(); }
    ImGui::SameLine();
    if (ImGui::RadioButton(S(159), curTheme == 6)) { m_cfg->uiTheme = 6; ApplyImGuiTheme(6); OnSave(); }

    ImGui::Spacing(); ImGui::Separator();
    if (ImGui::Button(S(22), ImVec2(140, 32))) OnSave();
    ImGui::SameLine();
    if (ImGui::Button(S(23), ImVec2(120, 32))) { if (m_ksm) m_ksm->ResetTotals(); OnSave(); }
    ImGui::SameLine();
    if (ImGui::Button(S(55), ImVec2(140, 32))) ImGui::OpenPopup(S(134));
    if (ImGui::BeginPopupModal(S(134), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(S(56));
        if (ImGui::Button(S(132), ImVec2(80, 0))) {
            bool chartWasVisible = m_cfg->showChart;
            *m_cfg = AppConfig{};
            ApplyImGuiTheme(0);
            m_cfg->keys.push_back({32, L"Space", {235,235,245,255}, {48,48,48,200}, {255,95,95,255}});
            m_cfg->keys.push_back({65, L"A",     {235,235,245,255}, {48,48,48,200}, {95,255,95,255}});
            m_cfg->keys.push_back({83, L"S",     {235,235,245,255}, {48,48,48,200}, {95,130,255,255}});
            m_cfg->keys.push_back({68, L"D",     {235,235,245,255}, {48,48,48,200}, {255,210,60,255}});
            if (m_chart && chartWasVisible) m_chart->Show(false);
            OnSave(); ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(S(133), ImVec2(80, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ========== 轨道设置页 ==========
void SettingsUI::DrawPageTrack() {
    if (!m_cfg) return;
    ImGui::SeparatorText(S(5));
    ImGui::BeginDisabled(m_cfg->freeMode);
    if (ImGui::Checkbox(S(5), &m_cfg->showHistory)) OnSave();
    ImGui::EndDisabled();
    ImGui::SameLine(140);
    if (ImGui::Checkbox(S(89), &m_cfg->historyShowLines)) OnSave();
    ImGui::Spacing(); ImGui::SeparatorText(S(141));
    ImGui::TextUnformatted(S(164)); ImGui::SameLine(60);
    bool up = !m_cfg->historyTrackReverse, down = m_cfg->historyTrackReverse;
    if (ImGui::RadioButton(S(114), up)) { m_cfg->historyTrackReverse = false; OnSave(); }
    ImGui::SameLine(160);
    if (ImGui::RadioButton(S(115), down)) { m_cfg->historyTrackReverse = true; OnSave(); }
    auto sv = [&]{ OnSave(); };
    ImGui::SeparatorText(S(116));
    SliderRow(S(10), &m_cfg->historyTrackH, 20, 600, sv, 180);
    SliderRow(S(41), &m_cfg->historyTrackGap, 0, 30, sv, 180);
    ImGui::SeparatorText(S(117));
    SliderRow(S(11), &m_cfg->historyGrowSpeed, 10, 900, sv, 180);
    SliderRow(S(165), &m_cfg->historyFloatSpeed, 10, 900, sv, 180);
    SliderRow(S(13), &m_cfg->historyBlockMax, 10, 100, sv, 180);
    ImGui::SeparatorText(S(147));
    SliderRow(S(118), &m_cfg->historyTrackAlpha, 0, 100, sv, 180);
    SliderRow(S(43), &m_cfg->historyBlockAlpha, 0, 255, sv, 180);
}

// ========== 按键映射页 ==========
void SettingsUI::DrawPageKeys() {
    if (!m_cfg) return;
    auto sv = [&]{ OnSave(); };
    int nKeys = (int)m_cfg->keys.size();
    ImGui::SeparatorText(S(95));
    bool isNormal = !m_cfg->freeMode, isFree = m_cfg->freeMode;
    if (ImGui::RadioButton(S(96), isNormal)) { m_cfg->freeMode = false; OnSave(); }
    ImGui::SameLine(120);
    if (ImGui::RadioButton(S(97), isFree)) {
        m_cfg->freeMode = true; m_cfg->showHistory = false; m_cfg->chartSnap = false;
        if (!m_cfg->keys.empty()) {
            int ks = m_cfg->keySize, gap = m_cfg->keySpacing, pad = 10;
            for (int i = 0; i < nKeys; ++i) {
                m_cfg->keys[i].freeX = pad + i * (ks + gap);
                m_cfg->keys[i].freeY = pad;
            }
            int off = pad + nKeys * (ks + gap);
            m_cfg->freeTotalX = off; m_cfg->freeTotalY = pad;
            m_cfg->freeKPSX = off + ks + gap; m_cfg->freeKPSY = pad;
            m_cfg->freeBPMX = off + (ks + gap) * 2; m_cfg->freeBPMY = pad;
        }
        OnSave();
    }
    ImGui::Spacing(); ImGui::SeparatorText(S(142));
    SliderRow(S(166), &m_cfg->freeAreaW, 200, 1200, sv, 180);
    SliderRow(S(167), &m_cfg->freeAreaH, 100, 800, sv, 180);
    SliderRow(S(109), &m_cfg->freeGridSize, 4, 64, sv, 180);
    bool showB = m_cfg->freeShowBoundary;
    if (ImGui::Checkbox(S(119), &showB)) {
        m_cfg->freeShowBoundary = showB;
        if (showB) { m_cfg->clickThrough = false; if (m_display) m_display->SetClickThrough(false); }
        else { m_cfg->clickThrough = true; if (m_display) m_display->SetClickThrough(true); }
        OnSave();
    }
    ImGui::SeparatorText(S(143));
    SliderRow(S(9), &m_cfg->keySpacing, 0, 40, sv, 180);
    SliderRow(S(53), &m_cfg->keyBorderW, 0, 8, sv, 180);
    ImGui::Spacing(); ImGui::SeparatorText(S(15));
    if (ImGui::BeginChild("KeyList", ImVec2(0, 120), true)) {
        for (int i = 0; i < nKeys; ++i) {
            char label[128];
            snprintf(label, 128, "%s  (VK:%d)", ToUtf8(m_cfg->keys[i].label), m_cfg->keys[i].keyCode);
            if (ImGui::Selectable(label, m_selectedKey == i)) m_selectedKey = i;
        }
    }
    ImGui::EndChild();
    if (ImGui::Button(S(16), ImVec2(120, 28))) { KeyboardHook::SetBlocked(true); MouseHook::SetBlocked(true); g_capturedVK = 0; s_captureState = 1; }
    ImGui::SameLine();
    if (ImGui::Button(S(17), ImVec2(120, 28))) { if (m_selectedKey >= 0 && m_selectedKey < nKeys && nKeys > 1) { m_cfg->keys.erase(m_cfg->keys.begin() + m_selectedKey); m_selectedKey = -1; OnSave(); } }
    if (m_selectedKey >= 0 && m_selectedKey < nKeys) {
        auto& kc = m_cfg->keys[m_selectedKey];
        ImGui::Spacing(); ImGui::SeparatorText(S(18));
        char nameBuf[64]; strncpy(nameBuf, ToUtf8(kc.label), 63); nameBuf[63] = 0;
        if (ImGui::InputText(S(137), nameBuf, 64)) {
            if (strlen(nameBuf) > 0) {
                std::wstring wname; int len = MultiByteToWideChar(CP_UTF8, 0, nameBuf, -1, nullptr, 0);
                wname.resize(len); MultiByteToWideChar(CP_UTF8, 0, nameBuf, -1, &wname[0], len);
                while (!wname.empty() && wname.back() == L'\0') wname.pop_back();
                kc.label = wname;
            }
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) OnSave();
        ImGui::TextUnformatted("W:"); ImGui::SameLine(30);
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("##kW", &kc.customW, 1, 10)) { if (kc.customW < 0) kc.customW = 0; if (kc.customW > 500) kc.customW = 500; }
        if (ImGui::IsItemDeactivatedAfterEdit()) OnSave();
        ImGui::SameLine(140); ImGui::TextUnformatted("H:"); ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("##kH", &kc.customH, 1, 10)) { if (kc.customH < 0) kc.customH = 0; if (kc.customH > 500) kc.customH = 500; }
        if (ImGui::IsItemDeactivatedAfterEdit()) OnSave();
        ImVec4 cf = ToImCol(kc.colorFont), cn = ToImCol(kc.colorNormal), cp = ToImCol(kc.colorPress);
        ImGui::TextUnformatted(S(19)); ImGui::SameLine(100);
        if (ImGui::ColorEdit3("##fc", (float*)&cf, ImGuiColorEditFlags_NoInputs)) { kc.colorFont = ToRgba(cf); }
        if (ImGui::IsItemDeactivatedAfterEdit()) OnSave();
        ImGui::TextUnformatted(S(20)); ImGui::SameLine(100);
        if (ImGui::ColorEdit3("##nc", (float*)&cn, ImGuiColorEditFlags_NoInputs)) { kc.colorNormal = ToRgba(cn); }
        if (ImGui::IsItemDeactivatedAfterEdit()) OnSave();
        ImGui::TextUnformatted(S(21)); ImGui::SameLine(100);
        if (ImGui::ColorEdit3("##pc", (float*)&cp, ImGuiColorEditFlags_NoInputs)) { kc.colorPress = ToRgba(cp); }
        if (ImGui::IsItemDeactivatedAfterEdit()) OnSave();
        ImVec4 cb = ToImCol(kc.colorBorder);
        ImGui::TextUnformatted(S(150)); ImGui::SameLine(100);
        if (ImGui::ColorEdit3("##bc", (float*)&cb, ImGuiColorEditFlags_NoInputs)) { kc.colorBorder = ToRgba(cb); }
        if (ImGui::IsItemDeactivatedAfterEdit()) OnSave();
    }
}

// ========== 主题页 ==========
void SettingsUI::DrawPageTheme() {
    if (!m_cfg) return;
    ImGui::SeparatorText(S(120));
    int nThemes = (int)m_cfg->themePresets.size();
    if (nThemes < 16) { m_cfg->InitDefaultThemes(); nThemes = 16; }
    int cols = 4;
    for (int i = 0; i < nThemes; ++i) {
        if (i % cols != 0) ImGui::SameLine();
        const char* name = ToUtf8(m_cfg->themePresets[i].name);
        if (name[0] == 0) { char fb[32]; snprintf(fb, 32, "预设%d", i); name = fb; }
        char id[32]; snprintf(id, 32, "##th%d", i);
        ImVec4 c = ToImCol(m_cfg->themePresets[i].normal);
        ImGui::PushStyleColor(ImGuiCol_Button, c); ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c);
        if (ImGui::Button(id, ImVec2(18, 18))) ApplyTheme(i);
        ImGui::PopStyleColor(2);
        ImGui::SameLine(); ImGui::TextUnformatted(name);
    }
    ImGui::Spacing();
    if (ImGui::Button(S(82), ImVec2(140, 30))) { if (m_themeEditor) m_themeEditor->Show(true); }
    ImGui::Spacing(); ImGui::SeparatorText(S(50));
    // 与按键映射一致的布局：每个属性一行，名称 + 控件
    for (int bi = 0; bi < 3; ++bi) {
        const char* boxNames[] = {"Total", "KPS", "BPM"};
        RgbaColor* boxBgs[]   = {&m_cfg->totalBoxBg, &m_cfg->kpsBoxBg, &m_cfg->bpmBoxBg};
        RgbaColor* boxFcs[]   = {&m_cfg->totalBoxFc, &m_cfg->kpsBoxFc, &m_cfg->bpmBoxFc};
        int* boxWs[]          = {&m_cfg->totalBoxW,   &m_cfg->kpsBoxW,   &m_cfg->bpmBoxW};
        int* boxHs[]          = {&m_cfg->totalBoxH,   &m_cfg->kpsBoxH,   &m_cfg->bpmBoxH};
        ImGui::TextUnformatted(boxNames[bi]);
        ImGui::PushID(bi);
        ImGui::Indent(20);
        ImGui::TextUnformatted(S(121)); ImGui::SameLine(100);
        { ImVec4 col = ToImCol(*boxBgs[bi]);
        if (ImGui::ColorEdit3("##bg", (float*)&col, ImGuiColorEditFlags_NoInputs)) { *boxBgs[bi] = ToRgba(col); }
        if (ImGui::IsItemDeactivatedAfterEdit()) OnSave(); }
        ImGui::TextUnformatted(S(122)); ImGui::SameLine(100);
        { ImVec4 col = ToImCol(*boxFcs[bi]);
        if (ImGui::ColorEdit3("##fc", (float*)&col, ImGuiColorEditFlags_NoInputs)) { *boxFcs[bi] = ToRgba(col); }
        if (ImGui::IsItemDeactivatedAfterEdit()) OnSave(); }
        ImGui::TextUnformatted(S(124)); ImGui::SameLine(100);
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("##w", boxWs[bi], 1, 10)) { if (*boxWs[bi] < 0) *boxWs[bi] = 0; if (*boxWs[bi] > 500) *boxWs[bi] = 500; }
        if (ImGui::IsItemDeactivatedAfterEdit()) OnSave();
        ImGui::TextUnformatted(S(125)); ImGui::SameLine(100);
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("##h", boxHs[bi], 1, 10)) { if (*boxHs[bi] < 0) *boxHs[bi] = 0; if (*boxHs[bi] > 500) *boxHs[bi] = 500; }
        if (ImGui::IsItemDeactivatedAfterEdit()) OnSave();
        ImGui::Unindent(20);
        ImGui::Spacing();
        ImGui::PopID();
    }
}

// ========== 图表页 ==========
void SettingsUI::DrawPageChart() {
    if (!m_cfg) return;
    auto sv = [&]{ OnSave(); };
    ImGui::SeparatorText("KPS");
    if (ImGui::Checkbox(S(59), &m_cfg->showChart)) { if (m_chart) m_chart->Show(m_cfg->showChart); OnSave(); }
    ImGui::SameLine(160);
    if (ImGui::Checkbox(S(90), &m_cfg->chartShowGrid)) OnSave();
    ImGui::Spacing(); ImGui::SeparatorText(S(126));
    int chartTypes[] = {0, 1, 2};
    const char* chartLabels[] = {S(91), S(92), S(93)};
    for (int i = 0; i < 3; ++i) {
        if (i > 0) ImGui::SameLine();
        if (ImGui::RadioButton(chartLabels[i], &m_cfg->chartType, chartTypes[i])) OnSave();
    }
    ImGui::SameLine(280);
    if (ImGui::Checkbox(S(94), &m_cfg->chartGradientFill)) OnSave();
    ImGui::SeparatorText(S(144));
    int tr = m_cfg->chartTimeRange / 1000;
    SliderRow(S(60), &tr, 1, 30, [&]{
        m_cfg->chartTimeRange = tr * 1000;
        OnSave();
    }, 170);
    SliderRow(S(168), &m_cfg->chartW, 200, 800, sv, 170);
    SliderRow(S(169), &m_cfg->chartH, 100, 600, sv, 170);
    ImGui::SeparatorText(S(127));
    SliderRow(S(170), &m_cfg->chartMarginL, 0, 60, sv, 170);
    SliderRow(S(171), &m_cfg->chartMarginR, 0, 60, sv, 170);
    SliderRow(S(172), &m_cfg->chartMarginT, 0, 60, sv, 170);
    SliderRow(S(173), &m_cfg->chartMarginB, 0, 60, sv, 170);
    ImGui::SeparatorText(S(128));
    ImVec4 cbg = ToImCol(m_cfg->chartBgCol), cln = ToImCol(m_cfg->chartLineCol);
    ImGui::TextUnformatted(S(121)); ImGui::SameLine(100);
    if (ImGui::ColorEdit3("##cbg", (float*)&cbg, ImGuiColorEditFlags_NoInputs)) { m_cfg->chartBgCol = ToRgba(cbg); }
    if (ImGui::IsItemDeactivatedAfterEdit()) OnSave();
    ImGui::TextUnformatted(S(123)); ImGui::SameLine(100);
    if (ImGui::ColorEdit3("##cln", (float*)&cln, ImGuiColorEditFlags_NoInputs)) { m_cfg->chartLineCol = ToRgba(cln); }
    if (ImGui::IsItemDeactivatedAfterEdit()) OnSave();
    ImGui::SeparatorText(S(129));
    SliderRow(S(174), &m_cfg->chartRadius, 0, 20, sv, 170);
    ImGui::SeparatorText(S(130));
    ImGui::BeginDisabled(m_cfg->freeMode);
    if (ImGui::Checkbox(S(131), &m_cfg->chartSnap)) OnSave();
    ImGui::EndDisabled();
    SliderRow(S(175), &m_cfg->chartSnapOffsetX, -200, 200, sv, 170);
    SliderRow(S(176), &m_cfg->chartSnapOffsetY, -200, 200, sv, 170);
}

// ========== 录制页 ==========
void SettingsUI::DrawPageRecording() {
    if (!m_cfg) return;
    ImGui::SeparatorText(S(98));

    // 录制状态指示
    if (m_recording) {
        // 闪烁效果：每500ms切换颜色
        bool blink = (GetTickCount64() / 500) % 2 == 0;
        if (blink) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.9f, 0.3f, 1.0f));
        else       ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 1.0f, 0.8f, 1.0f));
        ImGui::TextUnformatted(S(161));
        ImGui::PopStyleColor();

        // 计时
        uint64_t now = GetTickCount64();
        uint64_t elapsedMs = now - m_recStartTime;
        int sec = (int)(elapsedMs / 1000) % 60;
        int min = (int)(elapsedMs / 60000) % 60;
        int hr  = (int)(elapsedMs / 3600000);
        char timeBuf[32];
        if (hr > 0) snprintf(timeBuf, 32, "⏱ %d:%02d:%02d", hr, min, sec);
        else        snprintf(timeBuf, 32, "⏱ %d:%02d", min, sec);
        ImGui::SameLine(160);
        ImGui::TextUnformatted(timeBuf);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextUnformatted(S(160));
        ImGui::PopStyleColor();
    }

    ImGui::Spacing(); ImGui::SeparatorText(S(99));
    ImGui::TextWrapped(S(148));
    std::string btnLabel = S(149);
    if (m_cfg->recordingHotkeyVK != 0) {
        char kb[64]; GetKeyNameText(m_cfg->recordingHotkeyVK, kb, 64);
        btnLabel = kb;
    }
    if (ImGui::Button(btnLabel.c_str(), ImVec2(220, 30))) {
        KeyboardHook::SetBlocked(true);
        MouseHook::SetBlocked(true);
        g_capturedVK = 0;
        s_captureState = 2;
    }

    // 输出目录
    ImGui::Spacing(); ImGui::SeparatorText(S(162));
    std::wstring& outDir = m_cfg->recordingDir;
    // 如果为空，显示默认路径（exe目录 + record）
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    std::wstring defaultDir = std::wstring(exePath) + L"record";

    const wchar_t* displayDir = outDir.empty() ? defaultDir.c_str() : outDir.c_str();
    char dirUtf8[512];
    WideCharToMultiByte(CP_UTF8, 0, displayDir, -1, dirUtf8, 512, nullptr, nullptr);
    ImGui::TextWrapped("%s", dirUtf8);

    ImGui::SameLine();
    if (ImGui::Button(S(163), ImVec2(100, 0))) {
        // 打开文件夹选择对话框
        wchar_t selPath[MAX_PATH] = {};
        BROWSEINFOW bi = {};
        bi.hwndOwner = m_hwnd;
        bi.lpszTitle = L"选择录制文件输出目录";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        bi.lpfn = nullptr;
        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (pidl) {
            if (SHGetPathFromIDListW(pidl, selPath)) {
                outDir = selPath;
                // 确保末尾有反斜杠
                if (!outDir.empty() && outDir.back() != L'\\')
                    outDir += L'\\';
                OnSave();
            }
            CoTaskMemFree(pidl);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(S(177), ImVec2(80, 0))) {
        outDir.clear();
        OnSave();
    }
}

// ========== 快捷键设置页 ==========
static int* GetHKPtr(AppConfig* cfg, int slot) {
    switch (slot) {
        case 0: return &cfg->hotkeySettingsVK;
        case 1: return &cfg->hotkeyThemeEditorVK;
        case 2: return &cfg->hotkeyToggleDisplayVK;
        case 3: return &cfg->hotkeyNextThemeVK;
        case 4: return &cfg->hotkeyPrevThemeVK;
        case 5: return &cfg->hotkeyToggleTrackVK;
        case 6: return &cfg->hotkeyToggleChartVK;
        default: return nullptr;
    }
}

void SettingsUI::DrawPageHotkeys() {
    if (!m_cfg) return;
    ImGui::SeparatorText(S(151));
    ImGui::TextWrapped(S(185));
    ImGui::Spacing();

    for (int i = 0; i < 7; ++i) {
        int* vkPtr = GetHKPtr(m_cfg, i);
        if (!vkPtr) continue;

        char keyBuf[64] = "未设置";
        if (*vkPtr != 0) GetKeyNameText(*vkPtr, keyBuf, 64);
        char label[128];
        snprintf(label, 128, "Ctrl+Shift+%s", keyBuf);

        const char* hkName = "";
        switch (i) { case 0: hkName = S(178); break; case 1: hkName = S(179); break;
                     case 2: hkName = S(180); break; case 3: hkName = S(181); break;
                     case 4: hkName = S(182); break; case 5: hkName = S(183); break;
                     case 6: hkName = S(184); break; }
        ImGui::Text("%s:", hkName);
        ImGui::SameLine(130);

        if (s_hkCapturing == i) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
            ImGui::Button(S(186), ImVec2(200, 0));
            ImGui::PopStyleColor();

            int capturedVK = g_capturedVK;
            if (capturedVK > 0) {
                g_capturedVK = 0;
                if (capturedVK == VK_ESCAPE) {
                    s_hkCapturing = -1;
                } else {
                    *vkPtr = capturedVK;
                    OnSave();
                    if (GetActiveWindow()) PostMessageW(GetActiveWindow(), WM_APP + 5, 0, 0);
                    s_hkCapturing = -1;
                }
            }
        } else {
            if (ImGui::Button(label, ImVec2(200, 0))) {
                g_capturedVK = 0;
                s_hkCapturing = i;
            }
        }
        ImGui::Spacing();
    }
}

// ========== 保存 ==========
void SettingsUI::OnSave() {
    if (!m_cfg) return;
    if (m_display) m_display->SetClickThrough(m_cfg->clickThrough);
    const wchar_t* sp = m_configPath.empty() ? L"KeyStateSetting.json" : m_configPath.c_str();
    m_cfg->Save(sp);
    if (m_ksm) for (auto& kc : m_cfg->keys) kc.totalPresses = m_ksm->GetTotal(kc.keyCode);
}

void SettingsUI::ApplyTheme(int tid) {
    if (!m_cfg || m_cfg->keys.empty() || tid < 0 || tid > 15) return;
    if ((int)m_cfg->themePresets.size() <= tid) m_cfg->InitDefaultThemes();
    m_cfg->theme = tid;
    auto& tp = m_cfg->themePresets[tid];
    for (auto& kc : m_cfg->keys) {
        if (tp.font.a > 0) kc.colorFont = tp.font;
        if (tp.normal.a > 0) kc.colorNormal = tp.normal;
        if (tp.press.a > 0) kc.colorPress = tp.press;
    }
    if (tp.chartBg.a > 0) m_cfg->chartBgCol = tp.chartBg;
    if (tp.chartLine.a > 0) m_cfg->chartLineCol = tp.chartLine;
    OnSave();
}

void SettingsUI::CycleTheme(int direction) {
    if (!m_cfg || m_cfg->keys.empty()) return;
    int maxTheme = 15, newTheme = m_cfg->theme + direction;
    if (newTheme < 0) newTheme = maxTheme;
    if (newTheme > maxTheme) newTheme = 0;
    ApplyTheme(newTheme);
}

void SettingsUI::UpdateRecordStatus(bool recording, uint64_t startTime) {
    m_recording = recording; m_recStartTime = startTime;
}
