#pragma once

#include <vector>
#include <string>
#include <cstdint>

// RGBA 颜色结构
struct RgbaColor {
    uint8_t r = 235, g = 235, b = 245, a = 255;
    RgbaColor() = default;
    RgbaColor(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_)
        : r(r_), g(g_), b(b_), a(a_) {}
};

// 主题预设配色
struct ThemePreset {
    std::wstring name;
    RgbaColor font, normal, press, chartBg, chartLine;
};

// 单个按键映射配置
struct KeyConfig {
    int     keyCode       = 0;        // 虚拟键码
    std::wstring label;               // 显示标签
    RgbaColor colorFont     = {235, 235, 245, 255};
    RgbaColor colorNormal   = {48,  48,  48,  200};
    RgbaColor colorPress    = {255, 95,  95,  255};
    uint64_t totalPresses    = 0;
};

// 应用全局配置
struct AppConfig {
    int     version        = 3;
    bool    showTotal      = true;
    bool    showKPS        = true;
    bool    showBPM        = false;   // 汇总框显示 BPM
    int     bpmNoteDiv     = 16;      // 计算基准分音 (4/8/16/32/64)
    int     bpmMergeMs     = 30;      // 同时按下合并窗口 (0=禁用, 10~100ms)
    int     lang           = 0;       // 语言 0=CN 1=EN 2=JP
    bool    showSummary    = false;   // 末尾汇总框
    bool    showHistory     = false;   // 按键历史轨道
    int     historyTrackH   = 600;     // 轨道高度 (px)
    int     historyGrowSpeed = 300;    // 方块增长速率 (px/s)
    int     historyFloatSpeed= 300;    // 方块上浮速率 (px/s)
    int     historyBlockMax = 100;     // 方块最大高度 (% 轨道)
    int     historyTrackGap = 12;      // 轨道与按键间距 (px)
    int     historyTrackAlpha= 18;     // 轨道背景透明度 (0-100)
    int     historyBlockAlpha=255;     // 方块透明度 (0-255)
    RgbaColor totalBoxBg = {60,65,80,210};
    RgbaColor totalBoxFc = {220,225,240,255};
    RgbaColor kpsBoxBg   = {65,60,80,210};
    RgbaColor kpsBoxFc   = {220,220,250,255};
    RgbaColor bpmBoxBg   = {50,70,60,210};
    RgbaColor bpmBoxFc   = {180,240,200,255};
    int     fps             = 90;      // 渲染帧率 (25/45/60/90/120)
    bool    clickThrough   = false;
    float   overlayOpacity = 0.95f;
    int     keySpacing     = 12;
    int     keySize        = 64;
    int     keyRadius      = 12;
    int     keyBorderW     = 2;       // 边框宽度 (px)
    int     keyFontSize    = 26;      // 按键字体大小
    int     theme          = 0;       // 0=自定义
    int     displayX       = 100;
    int     displayY       = 100;
    int     displayW       = 400;
    int     displayH       = 120;
    int     chartX         = 100;
    int     chartY         = 330;
    int     chartW         = 400;
    int     chartH         = 180;
    int     chartMarginL   = 20;
    int     chartMarginR   = 12;
    int     chartMarginT   = 12;
    int     chartMarginB   = 12;
    bool    showChart      = false;
    int     chartTimeRange = 10000;   // X 轴时间范围 (ms)
    RgbaColor chartBgCol   = {18, 18, 22, 255};
    RgbaColor chartLineCol = {100, 255, 130, 255};
    int     chartRadius    = 8;       // 圆角半径
    std::vector<KeyConfig> keys;

    // 加载 / 保存 JSON 配置
    bool Load(const wchar_t* filepath);
    bool Save(const wchar_t* filepath) const;
    bool LoadThemes(const wchar_t* filepath);
    bool SaveThemes(const wchar_t* filepath) const;
    void InitDefaultThemes();
    std::vector<ThemePreset> themePresets;
};
