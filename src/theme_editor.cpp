#include "theme_editor.h"
#include "lang.h"
#include "imgui.h"
#include "imgui_setup.h"

static const char* ToUtf8(const std::wstring& ws) {
    static char buf[256];
    if (ws.empty()) return "";
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, buf, 256, nullptr, nullptr);
    return buf;
}

// 多语言快捷获取 UTF-8 字符串
static const char* S(int key) {
    static char buf[8][256];
    static int idx = 0;
    idx = (idx + 1) % 8;
    buf[idx][0] = '\0';
    const wchar_t* ws = LANG(key);
    if (!ws) return "";
    WideCharToMultiByte(CP_UTF8, 0, ws, -1, buf[idx], 256, nullptr, nullptr);
    return buf[idx];
}

static ImVec4 ToImCol(const RgbaColor& c) {
    return ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}
static RgbaColor ToRgba(const ImVec4& v) {
    return {(uint8_t)(v.x * 255), (uint8_t)(v.y * 255), (uint8_t)(v.z * 255), (uint8_t)(v.w * 255)};
}

ThemeEditor::ThemeEditor()  = default;
ThemeEditor::~ThemeEditor() = default;

bool ThemeEditor::Create(HINSTANCE hInst, AppConfig* cfg) {
    m_hInst = hInst;
    m_cfg   = cfg;
    return true;
}

void ThemeEditor::Show(bool visible) {
    m_visible = visible;
}

void ThemeEditor::RenderContent() {
    if (!m_visible || !m_cfg) return;

    ImGui::Begin(S(82), &m_visible, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

    if (m_cfg->themePresets.size() < 16) m_cfg->InitDefaultThemes();

    // 主题列表
    ImGui::BeginChild("ThemeList", ImVec2(200, 300), true);
    for (int i = 0; i < 16; ++i) {
        const char* name = ToUtf8(m_cfg->themePresets[i].name);
        if (name[0] == 0) {
            char fb[32]; snprintf(fb, 32, "预设%d", i);
            name = fb;
        }
        if (ImGui::Selectable(name, m_selTheme == i)) {
            m_selTheme = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // 编辑选中主题
    auto& tp = m_cfg->themePresets[m_selTheme];
    ImGui::BeginGroup();
    ImGui::Text("%s%s", S(187), ToUtf8(tp.name));

    char nameBuf[64];
    strncpy(nameBuf, ToUtf8(tp.name), 63); nameBuf[63] = 0;
    if (ImGui::InputText(S(137), nameBuf, 64)) {
        std::wstring wname;
        int len = MultiByteToWideChar(CP_UTF8, 0, nameBuf, -1, nullptr, 0);
        wname.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, nameBuf, -1, &wname[0], len);
        while (!wname.empty() && wname.back() == L'\0') wname.pop_back();
        tp.name = wname;
        OnSave();
    }

    ImVec4 colFont   = ToImCol(tp.font);
    ImVec4 colNormal = ToImCol(tp.normal);
    ImVec4 colPress  = ToImCol(tp.press);
    ImVec4 colChartBg = ToImCol(tp.chartBg);
    ImVec4 colChartLn = ToImCol(tp.chartLine);

    if (ImGui::ColorEdit4(S(19), (float*)&colFont, ImGuiColorEditFlags_NoInputs)) {
        tp.font = ToRgba(colFont); OnSave();
    }
    if (ImGui::ColorEdit4(S(20), (float*)&colNormal, ImGuiColorEditFlags_NoInputs)) {
        tp.normal = ToRgba(colNormal); OnSave();
    }
    if (ImGui::ColorEdit4(S(21), (float*)&colPress, ImGuiColorEditFlags_NoInputs)) {
        tp.press = ToRgba(colPress); OnSave();
    }
    if (ImGui::ColorEdit4(S(188), (float*)&colChartBg, ImGuiColorEditFlags_NoInputs)) {
        tp.chartBg = ToRgba(colChartBg); OnSave();
    }
    if (ImGui::ColorEdit4(S(189), (float*)&colChartLn, ImGuiColorEditFlags_NoInputs)) {
        tp.chartLine = ToRgba(colChartLn); OnSave();
    }

    ImGui::EndGroup();

    ImGui::End();
}

void ThemeEditor::OnSave() {
    const wchar_t* sp = m_configPath.empty() ? L"KeyStateThemes.json" : m_configPath.c_str();
    m_cfg->SaveThemes(sp);
}
