#include "keyboard.h"

// ========== KeyboardHook ==========

bool KeyboardHook::Install(Callback cb) {
    if (m_hook) return false;
    m_callback  = std::move(cb);
    s_instance  = this;
    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, HookProc,
                                GetModuleHandleW(nullptr), 0);
    return m_hook != nullptr;
}

void KeyboardHook::Uninstall() {
    if (m_hook) { UnhookWindowsHookEx(m_hook); m_hook = nullptr; }
    s_instance = nullptr;
}

LRESULT CALLBACK KeyboardHook::HookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0 && s_instance && s_instance->m_callback && !s_blocked) {
        auto* ks = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        // 过滤 IME 虚拟键 (VK_PROCESSKEY = 229)，输入法处理时不触发
        if (ks->vkCode == 229) return CallNextHookEx(nullptr, code, wParam, lParam);
        bool pressed = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        // 过滤重复的 WM_KEYDOWN（系统自动重复）
        if (wParam == WM_KEYDOWN) {
            if (ks->flags & LLKHF_UP) return CallNextHookEx(nullptr, code, wParam, lParam);
        }
        s_instance->m_callback((int)ks->vkCode, pressed);
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

// ========== KeyStateManager ==========

void KeyStateManager::Reset() { m_keys.clear(); }

void KeyStateManager::ResetTotals() {
    for (auto& kv : m_keys) { kv.second.total = 0; kv.second.pressTimes.clear(); kv.second.pressDurations.clear(); }
}

void KeyStateManager::OnKeyEvent(int keyCode, bool pressed) {
    auto& kr = m_keys[keyCode];
    // 过滤长按自动重复：状态没变则忽略
    if (pressed  && kr.state == KeyState::Down) return;
    if (!pressed && kr.state == KeyState::Up)  return;
    kr.state = pressed ? KeyState::Down : KeyState::Up;
    if (pressed) {
        kr.total++;
        kr.pressTimes.push_back(GetTickCount64());
        kr.pressStartTime = GetTickCount64();
        PruneOldPresses(kr);
    } else {
        // 记录按键时长
        uint64_t now = GetTickCount64();
        uint64_t dur = now - kr.pressStartTime;
        if (dur > 0) {
            kr.pressDurations.push_back({now, dur});
            // 清理超过 2 秒的旧记录
            while (!kr.pressDurations.empty() &&
                   now - kr.pressDurations.front().releaseTime > 2000)
                kr.pressDurations.pop_front();
        }
    }
}

bool KeyStateManager::IsDown(int keyCode) const {
    auto it = m_keys.find(keyCode);
    return it != m_keys.end() && it->second.state == KeyState::Down;
}

uint64_t KeyStateManager::GetTotal(int keyCode) const {
    auto it = m_keys.find(keyCode);
    return it != m_keys.end() ? it->second.total : 0;
}

double KeyStateManager::GetKPS(int keyCode) const {
    auto it = m_keys.find(keyCode);
    if (it == m_keys.end()) return 0.0;
    auto& kr = it->second;
    const_cast<KeyStateManager*>(this)->PruneOldPresses(const_cast<KeyRuntime&>(kr));
    return (double)kr.pressTimes.size(); // 最近 1 秒内的次数
}

void KeyStateManager::SetTotal(int keyCode, uint64_t val) {
    m_keys[keyCode].total = val;
}

const std::deque<uint64_t>& KeyStateManager::GetPressTimestamps(int keyCode) const {
    static std::deque<uint64_t> empty;
    auto it = m_keys.find(keyCode);
    if (it == m_keys.end()) return empty;
    const_cast<KeyStateManager*>(this)->PruneOldPresses(const_cast<KeyRuntime&>(it->second));
    return it->second.pressTimes;
}

const std::deque<PressStamp>& KeyStateManager::GetPressDurations(int keyCode) const {
    static std::deque<PressStamp> empty;
    auto it = m_keys.find(keyCode);
    if (it == m_keys.end()) return empty;
    return it->second.pressDurations;
}

uint64_t KeyStateManager::GetPressStartTime(int keyCode) const {
    auto it = m_keys.find(keyCode);
    return (it != m_keys.end()) ? it->second.pressStartTime : 0;
}

void KeyStateManager::PruneOldFromDeque(std::deque<uint64_t>& dq) const {
    uint64_t now = GetTickCount64();
    while (!dq.empty() && now - dq.front() > 1000) dq.pop_front();
}

void KeyStateManager::PruneOldPresses(KeyRuntime& kr) const {
    uint64_t now = GetTickCount64();
    while (!kr.pressTimes.empty() && now - kr.pressTimes.front() > 1000) {
        kr.pressTimes.pop_front();
    }
}
