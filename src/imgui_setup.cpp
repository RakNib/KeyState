#include "imgui_setup.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx9.h"
#include <cstdio>
#include <cstring>

// D3D9 设备
static LPDIRECT3D9              g_pD3D       = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static D3DPRESENT_PARAMETERS    g_d3dpp      = {};
static HWND                     g_hwnd       = nullptr;

// ImGui Win32 WndProc 转发
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// UI 主题状态
static int      g_uiTheme    = 0;     // 0=抹茶, 1=紫夜
static D3DCOLOR g_clearColor = D3DCOLOR_RGBA(247, 247, 237, 255);

bool ImGui_Init(HWND hwnd) {
    g_hwnd = hwnd;

    // 1. 创建 D3D9
    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_pD3D) {
        MessageBoxW(hwnd, L"Direct3DCreate9 失败！", L"错误", MB_ICONERROR);
        return false;
    }

    // 2. D3D9 呈现参数
    RECT rc;
    GetClientRect(hwnd, &rc);
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed               = TRUE;
    g_d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat       = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE; // 限制 60 FPS

    // 3. 创建设备
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                             D3DCREATE_HARDWARE_VERTEXPROCESSING,
                             &g_d3dpp, &g_pd3dDevice) < 0) {
        // 尝试软件处理
        if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                 D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                 &g_d3dpp, &g_pd3dDevice) < 0) {
            MessageBoxW(hwnd, L"创建 D3D9 设备失败！", L"错误", MB_ICONERROR);
            return false;
        }
    }

    // 4. 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // 不保存 imgui.ini

    // 4.5 加载中文字体（默认字体不含中文，会显示???）
    {
        ImFontConfig fontCfg;
        fontCfg.OversampleH = 2;
        fontCfg.OversampleV = 1;
        fontCfg.PixelSnapH  = true;
        // 尝试加载系统字体
        static const char* s_fontPaths[] = {
            "C:\\Windows\\Fonts\\msyh.ttc",  // Microsoft YaHei
            "C:\\Windows\\Fonts\\simhei.ttf",
            "C:\\Windows\\Fonts\\simsun.ttc",
            "C:\\Windows\\Fonts\\yahei.ttf",
        };
        ImFont* font = nullptr;
        for (const char* path : s_fontPaths) {
            if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
                continue;
            font = io.Fonts->AddFontFromFileTTF(path, 16.0f,
                &fontCfg, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
            if (font) break;
            // TTC 可能需指定索引
            if (strstr(path, ".ttc")) {
                fontCfg.FontNo = 1;
                font = io.Fonts->AddFontFromFileTTF(path, 16.0f,
                    &fontCfg, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                fontCfg.FontNo = 0;
                if (font) break;
            }
        }
        if (!font) {
            // 尝试全量 CJK 范围
            for (const char* path : s_fontPaths) {
                if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
                    continue;
                font = io.Fonts->AddFontFromFileTTF(path, 16.0f,
                    &fontCfg, io.Fonts->GetGlyphRangesChineseFull());
                if (font) break;
                if (strstr(path, ".ttc")) {
                    fontCfg.FontNo = 1;
                    font = io.Fonts->AddFontFromFileTTF(path, 16.0f,
                        &fontCfg, io.Fonts->GetGlyphRangesChineseFull());
                    fontCfg.FontNo = 0;
                    if (font) break;
                }
            }
        }
        if (!font) {
            io.Fonts->AddFontDefault();  // 最终降级
        }
    }

    // 5. 设置 ImGui 风格
    g_uiTheme = 0;
    ApplyImGuiTheme(0); // 默认抹茶主题

    // 6. 初始化 ImGui backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    return true;
}

void ImGui_Shutdown() {
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D)       { g_pD3D->Release();       g_pD3D = nullptr; }
}

void ImGui_BeginFrame() {
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGui_EndFrame() {
    ImGui::Render();

    // 清空背景
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

    // 清屏颜色跟随当前 UI 主题
    g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, g_clearColor, 1.0f, 0);

    if (g_pd3dDevice->BeginScene() >= 0) {
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        g_pd3dDevice->EndScene();
    }

    HRESULT hr = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
    // 设备丢失时重置
    if (hr == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
        g_pd3dDevice->Reset(&g_d3dpp);
        ImGui_ImplDX9_CreateDeviceObjects();
    }
}

bool ImGui_ProcessMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
}

// ========== UI 主题切换 ==========
void ApplyImGuiTheme(int theme) {
    g_uiTheme = theme;
    auto& style = ImGui::GetStyle();
    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 6.0f;
    style.WindowBorderSize  = 0.0f;
    style.FrameBorderSize   = 0.0f;
    style.ScrollbarSize     = 14.0f;
    style.WindowMinSize     = ImVec2(400, 300);

    ImVec4* colors = ImGui::GetStyle().Colors;

    if (theme == 0) {
        // 🔴 红
        ImGui::StyleColorsLight();
        colors[ImGuiCol_Text]                = ImVec4(0.35f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_WindowBg]            = ImVec4(0.98f, 0.94f, 0.93f, 1.00f);
        colors[ImGuiCol_TitleBg]             = ImVec4(0.75f, 0.30f, 0.25f, 1.00f);
        colors[ImGuiCol_TitleBgActive]       = ImVec4(0.65f, 0.22f, 0.18f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]    = ImVec4(0.85f, 0.55f, 0.50f, 0.60f);
        colors[ImGuiCol_MenuBarBg]           = ImVec4(0.94f, 0.86f, 0.84f, 1.00f);
        colors[ImGuiCol_FrameBg]             = ImVec4(0.95f, 0.90f, 0.88f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.90f, 0.78f, 0.75f, 1.00f);
        colors[ImGuiCol_FrameBgActive]       = ImVec4(0.84f, 0.65f, 0.60f, 1.00f);
        colors[ImGuiCol_Button]              = ImVec4(0.82f, 0.50f, 0.45f, 1.00f);
        colors[ImGuiCol_ButtonHovered]       = ImVec4(0.72f, 0.38f, 0.32f, 1.00f);
        colors[ImGuiCol_ButtonActive]        = ImVec4(0.62f, 0.28f, 0.22f, 1.00f);
        colors[ImGuiCol_Header]              = ImVec4(0.85f, 0.62f, 0.58f, 1.00f);
        colors[ImGuiCol_HeaderHovered]       = ImVec4(0.78f, 0.50f, 0.45f, 1.00f);
        colors[ImGuiCol_HeaderActive]        = ImVec4(0.70f, 0.40f, 0.35f, 1.00f);
        colors[ImGuiCol_Separator]           = ImVec4(0.82f, 0.60f, 0.55f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]    = ImVec4(0.72f, 0.42f, 0.38f, 1.00f);
        colors[ImGuiCol_SeparatorActive]     = ImVec4(0.62f, 0.32f, 0.28f, 1.00f);
        colors[ImGuiCol_SliderGrab]          = ImVec4(0.72f, 0.35f, 0.28f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]    = ImVec4(0.60f, 0.25f, 0.20f, 1.00f);
        colors[ImGuiCol_CheckMark]           = ImVec4(0.70f, 0.30f, 0.25f, 1.00f);
        colors[ImGuiCol_Tab]                 = ImVec4(0.92f, 0.82f, 0.80f, 1.00f);
        colors[ImGuiCol_TabHovered]          = ImVec4(0.82f, 0.58f, 0.52f, 1.00f);
        colors[ImGuiCol_TabActive]           = ImVec4(0.72f, 0.42f, 0.38f, 1.00f);
        colors[ImGuiCol_TabUnfocused]        = ImVec4(0.94f, 0.88f, 0.86f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.86f, 0.72f, 0.68f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]         = ImVec4(0.92f, 0.86f, 0.84f, 0.60f);
        g_clearColor = D3DCOLOR_RGBA(250, 240, 238, 255);
    } else if (theme == 1) {
        // 🟠 橙
        ImGui::StyleColorsLight();
        colors[ImGuiCol_Text]                = ImVec4(0.35f, 0.22f, 0.12f, 1.00f);
        colors[ImGuiCol_WindowBg]            = ImVec4(0.98f, 0.95f, 0.90f, 1.00f);
        colors[ImGuiCol_TitleBg]             = ImVec4(0.80f, 0.50f, 0.18f, 1.00f);
        colors[ImGuiCol_TitleBgActive]       = ImVec4(0.70f, 0.42f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]    = ImVec4(0.88f, 0.65f, 0.40f, 0.60f);
        colors[ImGuiCol_MenuBarBg]           = ImVec4(0.94f, 0.88f, 0.80f, 1.00f);
        colors[ImGuiCol_FrameBg]             = ImVec4(0.95f, 0.91f, 0.85f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.90f, 0.82f, 0.70f, 1.00f);
        colors[ImGuiCol_FrameBgActive]       = ImVec4(0.84f, 0.72f, 0.55f, 1.00f);
        colors[ImGuiCol_Button]              = ImVec4(0.85f, 0.62f, 0.35f, 1.00f);
        colors[ImGuiCol_ButtonHovered]       = ImVec4(0.78f, 0.52f, 0.25f, 1.00f);
        colors[ImGuiCol_ButtonActive]        = ImVec4(0.68f, 0.42f, 0.18f, 1.00f);
        colors[ImGuiCol_Header]              = ImVec4(0.88f, 0.70f, 0.48f, 1.00f);
        colors[ImGuiCol_HeaderHovered]       = ImVec4(0.82f, 0.60f, 0.35f, 1.00f);
        colors[ImGuiCol_HeaderActive]        = ImVec4(0.74f, 0.50f, 0.25f, 1.00f);
        colors[ImGuiCol_Separator]           = ImVec4(0.85f, 0.68f, 0.45f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]    = ImVec4(0.78f, 0.55f, 0.30f, 1.00f);
        colors[ImGuiCol_SeparatorActive]     = ImVec4(0.68f, 0.45f, 0.22f, 1.00f);
        colors[ImGuiCol_SliderGrab]          = ImVec4(0.78f, 0.52f, 0.25f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]    = ImVec4(0.65f, 0.40f, 0.18f, 1.00f);
        colors[ImGuiCol_CheckMark]           = ImVec4(0.75f, 0.48f, 0.22f, 1.00f);
        colors[ImGuiCol_Tab]                 = ImVec4(0.92f, 0.85f, 0.76f, 1.00f);
        colors[ImGuiCol_TabHovered]          = ImVec4(0.84f, 0.68f, 0.48f, 1.00f);
        colors[ImGuiCol_TabActive]           = ImVec4(0.76f, 0.55f, 0.32f, 1.00f);
        colors[ImGuiCol_TabUnfocused]        = ImVec4(0.94f, 0.90f, 0.84f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.88f, 0.78f, 0.65f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]         = ImVec4(0.92f, 0.88f, 0.82f, 0.60f);
        g_clearColor = D3DCOLOR_RGBA(250, 244, 235, 255);
    } else if (theme == 2) {
        // 🟡 黄
        ImGui::StyleColorsLight();
        colors[ImGuiCol_Text]                = ImVec4(0.30f, 0.28f, 0.12f, 1.00f);
        colors[ImGuiCol_WindowBg]            = ImVec4(0.98f, 0.97f, 0.90f, 1.00f);
        colors[ImGuiCol_TitleBg]             = ImVec4(0.80f, 0.72f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBgActive]       = ImVec4(0.72f, 0.64f, 0.14f, 1.00f);
        colors[ImGuiCol_MenuBarBg]           = ImVec4(0.95f, 0.92f, 0.80f, 1.00f);
        colors[ImGuiCol_FrameBg]             = ImVec4(0.96f, 0.94f, 0.85f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.92f, 0.88f, 0.72f, 1.00f);
        colors[ImGuiCol_FrameBgActive]       = ImVec4(0.86f, 0.80f, 0.58f, 1.00f);
        colors[ImGuiCol_Button]              = ImVec4(0.88f, 0.80f, 0.40f, 1.00f);
        colors[ImGuiCol_ButtonHovered]       = ImVec4(0.80f, 0.70f, 0.30f, 1.00f);
        colors[ImGuiCol_ButtonActive]        = ImVec4(0.70f, 0.60f, 0.22f, 1.00f);
        colors[ImGuiCol_Header]              = ImVec4(0.90f, 0.84f, 0.55f, 1.00f);
        colors[ImGuiCol_HeaderHovered]       = ImVec4(0.84f, 0.76f, 0.42f, 1.00f);
        colors[ImGuiCol_HeaderActive]        = ImVec4(0.76f, 0.66f, 0.32f, 1.00f);
        colors[ImGuiCol_Separator]           = ImVec4(0.86f, 0.80f, 0.50f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]    = ImVec4(0.78f, 0.70f, 0.35f, 1.00f);
        colors[ImGuiCol_SeparatorActive]     = ImVec4(0.68f, 0.60f, 0.25f, 1.00f);
        colors[ImGuiCol_SliderGrab]          = ImVec4(0.78f, 0.70f, 0.30f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]    = ImVec4(0.65f, 0.56f, 0.20f, 1.00f);
        colors[ImGuiCol_CheckMark]           = ImVec4(0.75f, 0.65f, 0.25f, 1.00f);
        colors[ImGuiCol_Tab]                 = ImVec4(0.94f, 0.90f, 0.78f, 1.00f);
        colors[ImGuiCol_TabHovered]          = ImVec4(0.86f, 0.78f, 0.52f, 1.00f);
        colors[ImGuiCol_TabActive]           = ImVec4(0.78f, 0.68f, 0.38f, 1.00f);
        colors[ImGuiCol_TabUnfocused]        = ImVec4(0.96f, 0.94f, 0.86f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.90f, 0.84f, 0.68f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]         = ImVec4(0.94f, 0.92f, 0.82f, 0.60f);
        g_clearColor = D3DCOLOR_RGBA(252, 250, 238, 255);
    } else if (theme == 3) {
        // 🟢 绿
        ImGui::StyleColorsLight();
        colors[ImGuiCol_Text]                = ImVec4(0.18f, 0.30f, 0.16f, 1.00f);
        colors[ImGuiCol_WindowBg]            = ImVec4(0.96f, 0.97f, 0.93f, 1.00f);
        colors[ImGuiCol_TitleBg]             = ImVec4(0.35f, 0.62f, 0.32f, 1.00f);
        colors[ImGuiCol_TitleBgActive]       = ImVec4(0.28f, 0.54f, 0.25f, 1.00f);
        colors[ImGuiCol_MenuBarBg]           = ImVec4(0.85f, 0.90f, 0.82f, 1.00f);
        colors[ImGuiCol_FrameBg]             = ImVec4(0.90f, 0.94f, 0.88f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.80f, 0.88f, 0.76f, 1.00f);
        colors[ImGuiCol_FrameBgActive]       = ImVec4(0.70f, 0.80f, 0.64f, 1.00f);
        colors[ImGuiCol_Button]              = ImVec4(0.60f, 0.80f, 0.55f, 1.00f);
        colors[ImGuiCol_ButtonHovered]       = ImVec4(0.48f, 0.70f, 0.42f, 1.00f);
        colors[ImGuiCol_ButtonActive]        = ImVec4(0.38f, 0.60f, 0.32f, 1.00f);
        colors[ImGuiCol_Header]              = ImVec4(0.68f, 0.82f, 0.62f, 1.00f);
        colors[ImGuiCol_HeaderHovered]       = ImVec4(0.55f, 0.74f, 0.48f, 1.00f);
        colors[ImGuiCol_HeaderActive]        = ImVec4(0.45f, 0.65f, 0.38f, 1.00f);
        colors[ImGuiCol_Separator]           = ImVec4(0.65f, 0.78f, 0.58f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]    = ImVec4(0.50f, 0.68f, 0.42f, 1.00f);
        colors[ImGuiCol_SeparatorActive]     = ImVec4(0.40f, 0.58f, 0.32f, 1.00f);
        colors[ImGuiCol_SliderGrab]          = ImVec4(0.45f, 0.68f, 0.38f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]    = ImVec4(0.32f, 0.55f, 0.28f, 1.00f);
        colors[ImGuiCol_CheckMark]           = ImVec4(0.38f, 0.62f, 0.30f, 1.00f);
        colors[ImGuiCol_Tab]                 = ImVec4(0.85f, 0.90f, 0.82f, 1.00f);
        colors[ImGuiCol_TabHovered]          = ImVec4(0.62f, 0.78f, 0.55f, 1.00f);
        colors[ImGuiCol_TabActive]           = ImVec4(0.50f, 0.70f, 0.42f, 1.00f);
        colors[ImGuiCol_TabUnfocused]        = ImVec4(0.90f, 0.94f, 0.88f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.78f, 0.85f, 0.74f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]         = ImVec4(0.88f, 0.92f, 0.85f, 0.60f);
        g_clearColor = D3DCOLOR_RGBA(245, 248, 240, 255);
    } else if (theme == 4) {
        // 🔵 蓝
        ImGui::StyleColorsLight();
        colors[ImGuiCol_Text]                = ImVec4(0.12f, 0.20f, 0.35f, 1.00f);
        colors[ImGuiCol_WindowBg]            = ImVec4(0.94f, 0.95f, 0.98f, 1.00f);
        colors[ImGuiCol_TitleBg]             = ImVec4(0.28f, 0.48f, 0.78f, 1.00f);
        colors[ImGuiCol_TitleBgActive]       = ImVec4(0.22f, 0.40f, 0.68f, 1.00f);
        colors[ImGuiCol_MenuBarBg]           = ImVec4(0.84f, 0.88f, 0.95f, 1.00f);
        colors[ImGuiCol_FrameBg]             = ImVec4(0.90f, 0.92f, 0.96f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.80f, 0.84f, 0.92f, 1.00f);
        colors[ImGuiCol_FrameBgActive]       = ImVec4(0.70f, 0.76f, 0.88f, 1.00f);
        colors[ImGuiCol_Button]              = ImVec4(0.55f, 0.70f, 0.88f, 1.00f);
        colors[ImGuiCol_ButtonHovered]       = ImVec4(0.42f, 0.58f, 0.80f, 1.00f);
        colors[ImGuiCol_ButtonActive]        = ImVec4(0.32f, 0.48f, 0.70f, 1.00f);
        colors[ImGuiCol_Header]              = ImVec4(0.65f, 0.76f, 0.90f, 1.00f);
        colors[ImGuiCol_HeaderHovered]       = ImVec4(0.52f, 0.65f, 0.82f, 1.00f);
        colors[ImGuiCol_HeaderActive]        = ImVec4(0.42f, 0.55f, 0.74f, 1.00f);
        colors[ImGuiCol_Separator]           = ImVec4(0.60f, 0.72f, 0.85f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]    = ImVec4(0.48f, 0.60f, 0.76f, 1.00f);
        colors[ImGuiCol_SeparatorActive]     = ImVec4(0.38f, 0.50f, 0.66f, 1.00f);
        colors[ImGuiCol_SliderGrab]          = ImVec4(0.35f, 0.55f, 0.78f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]    = ImVec4(0.25f, 0.42f, 0.65f, 1.00f);
        colors[ImGuiCol_CheckMark]           = ImVec4(0.25f, 0.48f, 0.72f, 1.00f);
        colors[ImGuiCol_Tab]                 = ImVec4(0.85f, 0.88f, 0.94f, 1.00f);
        colors[ImGuiCol_TabHovered]          = ImVec4(0.60f, 0.72f, 0.86f, 1.00f);
        colors[ImGuiCol_TabActive]           = ImVec4(0.42f, 0.58f, 0.78f, 1.00f);
        colors[ImGuiCol_TabUnfocused]        = ImVec4(0.90f, 0.92f, 0.96f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.76f, 0.80f, 0.90f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]         = ImVec4(0.88f, 0.90f, 0.94f, 0.60f);
        g_clearColor = D3DCOLOR_RGBA(240, 243, 248, 255);
    } else if (theme == 5) {
        // 🟣 靛
        ImGui::StyleColorsDark();
        colors[ImGuiCol_Text]                = ImVec4(0.78f, 0.80f, 0.95f, 1.00f);
        colors[ImGuiCol_WindowBg]            = ImVec4(0.10f, 0.10f, 0.18f, 1.00f);
        colors[ImGuiCol_TitleBg]             = ImVec4(0.20f, 0.20f, 0.45f, 1.00f);
        colors[ImGuiCol_TitleBgActive]       = ImVec4(0.25f, 0.25f, 0.55f, 1.00f);
        colors[ImGuiCol_MenuBarBg]           = ImVec4(0.14f, 0.14f, 0.24f, 1.00f);
        colors[ImGuiCol_FrameBg]             = ImVec4(0.16f, 0.16f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.24f, 0.24f, 0.40f, 1.00f);
        colors[ImGuiCol_FrameBgActive]       = ImVec4(0.30f, 0.30f, 0.50f, 1.00f);
        colors[ImGuiCol_Button]              = ImVec4(0.28f, 0.28f, 0.50f, 1.00f);
        colors[ImGuiCol_ButtonHovered]       = ImVec4(0.36f, 0.36f, 0.62f, 1.00f);
        colors[ImGuiCol_ButtonActive]        = ImVec4(0.44f, 0.44f, 0.72f, 1.00f);
        colors[ImGuiCol_Header]              = ImVec4(0.26f, 0.26f, 0.48f, 1.00f);
        colors[ImGuiCol_HeaderHovered]       = ImVec4(0.34f, 0.34f, 0.58f, 1.00f);
        colors[ImGuiCol_HeaderActive]        = ImVec4(0.40f, 0.40f, 0.66f, 1.00f);
        colors[ImGuiCol_Separator]           = ImVec4(0.26f, 0.26f, 0.46f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]    = ImVec4(0.36f, 0.36f, 0.58f, 1.00f);
        colors[ImGuiCol_SeparatorActive]     = ImVec4(0.44f, 0.44f, 0.68f, 1.00f);
        colors[ImGuiCol_SliderGrab]          = ImVec4(0.40f, 0.40f, 0.68f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]    = ImVec4(0.50f, 0.50f, 0.80f, 1.00f);
        colors[ImGuiCol_CheckMark]           = ImVec4(0.50f, 0.50f, 0.82f, 1.00f);
        colors[ImGuiCol_Tab]                 = ImVec4(0.14f, 0.14f, 0.26f, 1.00f);
        colors[ImGuiCol_TabHovered]          = ImVec4(0.28f, 0.28f, 0.48f, 1.00f);
        colors[ImGuiCol_TabActive]           = ImVec4(0.34f, 0.34f, 0.56f, 1.00f);
        colors[ImGuiCol_TabUnfocused]        = ImVec4(0.12f, 0.12f, 0.22f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.18f, 0.18f, 0.32f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]         = ImVec4(0.08f, 0.08f, 0.16f, 0.60f);
        g_clearColor = D3DCOLOR_RGBA(16, 16, 32, 255);
    } else {
        // 🟤 紫
        ImGui::StyleColorsDark();
        colors[ImGuiCol_Text]                = ImVec4(0.90f, 0.85f, 0.95f, 1.00f);
        colors[ImGuiCol_WindowBg]            = ImVec4(0.14f, 0.10f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBg]             = ImVec4(0.38f, 0.20f, 0.50f, 1.00f);
        colors[ImGuiCol_TitleBgActive]       = ImVec4(0.45f, 0.25f, 0.58f, 1.00f);
        colors[ImGuiCol_MenuBarBg]           = ImVec4(0.18f, 0.14f, 0.26f, 1.00f);
        colors[ImGuiCol_FrameBg]             = ImVec4(0.20f, 0.16f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.30f, 0.22f, 0.40f, 1.00f);
        colors[ImGuiCol_FrameBgActive]       = ImVec4(0.38f, 0.28f, 0.50f, 1.00f);
        colors[ImGuiCol_Button]              = ImVec4(0.35f, 0.24f, 0.48f, 1.00f);
        colors[ImGuiCol_ButtonHovered]       = ImVec4(0.46f, 0.32f, 0.60f, 1.00f);
        colors[ImGuiCol_ButtonActive]        = ImVec4(0.55f, 0.40f, 0.70f, 1.00f);
        colors[ImGuiCol_Header]              = ImVec4(0.34f, 0.24f, 0.46f, 1.00f);
        colors[ImGuiCol_HeaderHovered]       = ImVec4(0.42f, 0.30f, 0.56f, 1.00f);
        colors[ImGuiCol_HeaderActive]        = ImVec4(0.50f, 0.36f, 0.64f, 1.00f);
        colors[ImGuiCol_Separator]           = ImVec4(0.34f, 0.24f, 0.44f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]    = ImVec4(0.44f, 0.32f, 0.56f, 1.00f);
        colors[ImGuiCol_SeparatorActive]     = ImVec4(0.52f, 0.40f, 0.66f, 1.00f);
        colors[ImGuiCol_SliderGrab]          = ImVec4(0.50f, 0.36f, 0.64f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]    = ImVec4(0.62f, 0.46f, 0.78f, 1.00f);
        colors[ImGuiCol_CheckMark]           = ImVec4(0.65f, 0.48f, 0.82f, 1.00f);
        colors[ImGuiCol_Tab]                 = ImVec4(0.18f, 0.14f, 0.26f, 1.00f);
        colors[ImGuiCol_TabHovered]          = ImVec4(0.32f, 0.22f, 0.44f, 1.00f);
        colors[ImGuiCol_TabActive]           = ImVec4(0.40f, 0.28f, 0.54f, 1.00f);
        colors[ImGuiCol_TabUnfocused]        = ImVec4(0.16f, 0.12f, 0.22f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.22f, 0.18f, 0.32f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]         = ImVec4(0.12f, 0.08f, 0.18f, 0.60f);
        g_clearColor = D3DCOLOR_RGBA(22, 16, 34, 255);
    }
}
