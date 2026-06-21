#pragma once

#include <windows.h>
#include <functional>
#include <vector>
#include <deque>
#include <unordered_map>
#include <atomic>
#include <chrono>

// 按键状态：按下 / 松开
enum class KeyState { Up, Down };

// 每次按键的持续时间（用于历史轨道）
struct PressStamp {
    uint64_t releaseTime = 0;  // 松开时刻 (ms)
    uint64_t durationMs  = 0;  // 按住时长 (ms)
};

// 每个按键的运行时数据
struct KeyRuntime {
    KeyState state      = KeyState::Up;
    uint64_t total      = 0;        // 总按键次数
    std::deque<uint64_t> pressTimes; // 最近按键时间戳 (ms)，用于算 KPS
    uint64_t pressStartTime = 0;     // 当前按下的起始时刻
    std::deque<PressStamp> pressDurations; // 最近按键时长
};

// 键盘监听器 — 底层全局钩子
class KeyboardHook {
public:
    using Callback = std::function<void(int keyCode, bool pressed)>;

    bool Install(Callback cb);
    void Uninstall();
    bool IsInstalled() const { return m_hook != nullptr; }

    // 暂停/恢复钩子回调（AddKey 捕获期间暂停）
    static void SetBlocked(bool blocked) { s_blocked = blocked; }
    static bool IsBlocked()              { return s_blocked; }

private:
    static LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);
    HHOOK         m_hook   = nullptr;
    Callback      m_callback;
    static inline KeyboardHook*       s_instance = nullptr;
    static inline std::atomic<bool>   s_blocked  = false;
};

// 按键状态管理器
class KeyStateManager {
public:
    void Reset();
    void ResetTotals();
    void OnKeyEvent(int keyCode, bool pressed);

    // 查询
    bool     IsDown(int keyCode) const;
    uint64_t GetTotal(int keyCode) const;
    double   GetKPS(int keyCode) const;

    // 获取最近按键时间戳（用于历史轨道渲染）
    const std::deque<uint64_t>& GetPressTimestamps(int keyCode) const;
    const std::deque<PressStamp>& GetPressDurations(int keyCode) const;
    uint64_t GetPressStartTime(int keyCode) const;
    void PruneOldFromDeque(std::deque<uint64_t>& dq) const;

    // 直接修改计数
    void SetTotal(int keyCode, uint64_t val);

private:
    std::unordered_map<int, KeyRuntime> m_keys;
    void PruneOldPresses(KeyRuntime& kr) const;
};
