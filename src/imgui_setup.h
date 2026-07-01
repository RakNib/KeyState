#pragma once
#include <windows.h>
#include <d3d9.h>
#include <string>

// ImGui 设置窗口的初始化、渲染、销毁
bool ImGui_Init(HWND hwnd);
void ImGui_Shutdown();
void ImGui_BeginFrame();
void ImGui_EndFrame();
bool ImGui_ProcessMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
// UI 主题切换（调用后下一帧生效）
void ApplyImGuiTheme(int theme);
