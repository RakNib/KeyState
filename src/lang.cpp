#include "lang.h"
#include "config.h"

// 全局语言指针（由 main 设置）
static int g_lang = 0;

void SetLanguage(int lang) { g_lang = lang; }
int  GetLanguage()         { return g_lang; }

// 字符串表: key → { CN, EN, JP }
static const wchar_t* g_strings[][3] = {
    // 0
    {L"KeyState 设置", L"KeyState Settings", L"KeyState 設定"},
    // 1
    {L"基本设置", L"Basic Settings", L"基本設定"},
    // 2
    {L"显示总数", L"Show Total", L"総数表示"},
    // 3
    {L"显示KPS", L"Show KPS", L"KPS表示"},
    // 4
    {L"显示汇总", L"Show Summary", L"集計表示"},
    // 5
    {L"显示轨道", L"Show History", L"軌道表示"},
    // 6
    {L"显示BPM", L"Show BPM", L"BPM表示"},
    // 7
    {L"固定", L"Click Through", L"クリックスルー"},
    // 8
    {L"按键大小 (px)", L"Key Size (px)", L"キーサイズ (px)"},
    // 9
    {L"按键间距 (px)", L"Key Spacing (px)", L"キー間隔 (px)"},
    // 10
    {L"轨道高度 (px)", L"Track Height (px)", L"軌道高さ (px)"},
    // 11
    {L"增长速率 (px/s)", L"Grow Speed (px/s)", L"成長速度 (px/s)"},
    // 12
    {L"上浮速率 (px/s)", L"Float Speed (px/s)", L"浮上速度 (px/s)"},
    // 13
    {L"方块上限 (%轨道)", L"Block Max (%track)", L"ブロック上限 (%軌道)"},
    // 14
    {L"渲染帧率", L"Render FPS", L"描画FPS"},
    // 15
    {L"按键映射", L"Key Mappings", L"キーマッピング"},
    // 16
    {L"添加按键", L"Add Key", L"キー追加"},
    // 17
    {L"删除按键", L"Delete Key", L"キー削除"},
    // 18
    {L"外观设置 (请先在列表中选中一个按键)", L"Appearance (select a key from list)", L"外観設定 (リストから選択)"},
    // 19
    {L"字体颜色", L"Font Color", L"フォント色"},
    // 20
    {L"正常底色", L"Normal BG", L"通常背景"},
    // 21
    {L"按下底色", L"Press BG", L"押下背景"},
    // 22
    {L"保存并应用", L"Save & Apply", L"保存＆適用"},
    // 23
    {L"重置计数", L"Reset Totals", L"カウントリセット"},
    // 24
    {L"语言", L"Language", L"言語"},
    // 25
    {L"中文", L"中文", L"中文"},
    // 26
    {L"English", L"English", L"English"},
    // 27
    {L"日本語", L"日本語", L"日本語"},
    // 28
    {L"按任意键添加… (ESC 取消)", L"Press any key to add… (ESC to cancel)", L"キーを押して追加… (ESC取消)"},
    // 29
    {L"该按键已在列表中。", L"This key is already in the list.", L"このキーは既に存在します。"},
    // 30
    {L"重复按键", L"Duplicate Key", L"重複キー"},
    // 31
    {L"请先选中要删除的按键。", L"Please select a key to delete.", L"削除するキーを選択してください。"},
    // 32
    {L"未选择", L"No Selection", L"未選択"},
    // 33
    {L"至少保留一个按键映射。", L"At least one key must remain.", L"最低1つのキーが必要です。"},
    // 34
    {L"已达上限", L"Limit", L"上限"},
    // 35
    {L"所有按键计数已清零。", L"All totals have been reset to 0.", L"カウントをリセットしました。"},
    // 36
    {L"重置完成", L"Reset", L"リセット"},
    // 37
    {L"8分",    L"8th",    L"8分"},
    // 38
    {L"16分",   L"16th",   L"16分"},
    // 39
    {L"32分",   L"32nd",   L"32分"},
    // 40
    {L"64分",   L"64th",   L"64分"},
    // 41
    {L"轨道间距 (px)",     L"Track Gap (px)",     L"トラック間隔 (px)"},
    // 42
    {L"轨道透明度",        L"Track Alpha",        L"トラック透明度"},
    // 43
    {L"方块透明度",        L"Block Alpha",        L"ブロック透明度"},
    // 44
    {L"BPM 合并 (ms)",    L"BPM Merge (ms)",     L"BPM統合 (ms)"},
    // 45
    {L"\U0001F3A8 主题",  L"\U0001F3A8 Theme",   L"\U0001F3A8 テーマ"},
    // 46
    {L"自定义",           L"Custom",             L"カスタム"},
    // 47
    {L"预设1",            L"Preset1",            L"プリセット1"},
    // 48
    {L"预设2",            L"Preset2",            L"プリセット2"},
    // 49
    {L"预设3",            L"Preset3",            L"プリセット3"},
    // 50
    {L"盒子颜色",         L"Box Colors",         L"ボックス色"},
    // 51
    {L"BG",               L"BG",                 L"BG"},
    // 52
    {L"字体",             L"Font",               L"フォント"},
    // 53
    {L"边框宽度 (px)",    L"Border W (px)",      L"境界幅 (px)"},
    // 54
    {L"透明度 (%)",       L"Opacity (%)",        L"不透明度 (%)"},
    // 55
    {L"重置默认设置",     L"Reset Defaults",     L"デフォルトに戻す"},
    // 56
    {L"确认将所有设置重置为默认值？\n此操作无法撤销。", L"Reset ALL settings to defaults?\nThis cannot be undone.", L"全ての設定をデフォルトに戻しますか？\nこの操作は取り消せません。"},
    // 57
    {L"确认重置",         L"Confirm Reset",      L"リセット確認"},
    // 58
    {L"KPS 图表",         L"KPS Chart",          L"KPSチャート"},
    // 59
    {L"显示图表",         L"Show Chart",         L"チャート表示"},
    // 60
    {L"时间范围 (s)",     L"Time Range (s)",     L"時間範囲 (s)"},
    // 61
    {L"背景色",           L"Chart BG",           L"背景色"},
    // 62
    {L"折线色",           L"Chart Line",         L"ライン色"},
    // 63
    {L"圆角",             L"Radius",             L"角丸"},
    // 64
    {L"图表宽度",    L"Chart W",       L"チャート幅"},
    // 65
    {L"图表高度",    L"Chart H",       L"チャート高さ"},
    // 66
    {L"左边距",           L"Margin L",           L"左余白"},
    // 67
    {L"右边距",           L"Margin R",           L"右余白"},
    // 68
    {L"上边距",           L"Margin T",           L"上余白"},
    // 69
    {L"下边距",           L"Margin B",           L"下余白"},
    // 70
    {L"预设4",            L"Preset4",            L"プリセット4"},
    // 71
    {L"预设5",            L"Preset5",            L"プリセット5"},
    // 72
    {L"预设6",            L"Preset6",            L"プリセット6"},
    // 73
    {L"预设7",            L"Preset7",            L"プリセット7"},
    // 74
    {L"预设8",            L"Preset8",            L"プリセット8"},
    // 75
    {L"预设9",            L"Preset9",            L"プリセット9"},
    // 76
    {L"预设10",           L"Preset10",           L"プリセット10"},
    // 77
    {L"预设11",           L"Preset11",           L"プリセット11"},
    // 78
    {L"预设12",           L"Preset12",           L"プリセット12"},
    // 79
    {L"预设13",           L"Preset13",           L"プリセット13"},
    // 80
    {L"预设14",           L"Preset14",           L"プリセット14"},
    // 81
    {L"预设15",           L"Preset15",           L"プリセット15"},
    // 82
    {L"配色编辑器",       L"Theme Editor",       L"テーマエディタ"},
    // 83
    {L"显示",             L"Display",            L"表示"},
    // 84
    {L"轨道",             L"Track",             L"トラック"},
    // 85
    {L"主题",             L"Theme",              L"テーマ"},
    // 86
    {L"图表",             L"Chart",              L"チャート"},
    // 87
    {L"按键",             L"Keys",               L"キー"},
    // 88
    {L"置顶显示",         L"Always On Top",      L"最前面に表示"},
    // 89
    {L"显示轨道线",       L"Show Track Lines",   L"トラック線表示"},
    // 90
    {L"显示网格线",       L"Show Grid Lines",    L"グリッド線表示"},
    // 91
    {L"折线图", L"Line", L"折れ線"},
    // 92
    {L"散点图", L"Scatter", L"散布図"},
    // 93
    {L"柱状图", L"Bar", L"棒グラフ"},
    // 94
    {L"渐变填充", L"Gradient Fill", L"グラデーション"},
    // 95
    {L"布局模式", L"Layout Mode", L"レイアウトモード"},
    // 96
    {L"常规模式", L"Normal Mode", L"通常モード"},
    // 97
    {L"自由模式", L"Free Mode", L"フリーモード"},
    // 98
    {L"录制功能", L"Recording", L"録画機能"},
    // 99
    {L"录制快捷键", L"Record Hotkey", L"録画ショートカット"},
    // 100
    {L"点击设置录制快捷键…", L"Click to set record hotkey…", L"クリックして録画キー設定…"},
    // 101
    {L"按任意键设置录制快捷键… (ESC 取消)", L"Press any key for record hotkey… (ESC cancel)", L"録画キーを押してください… (ESC取消)"},
    // 102
    {L"录制中… 再次按下快捷键停止", L"Recording… Press hotkey again to stop", L"録画中… もう一度押して停止"},
    // 103
    {L"录制已保存", L"Recording saved", L"録画保存完了"},
    // 104
    {L"录制数据已保存到", L"Recording data saved to", L"録画データを保存しました"},
    // 105
    {L"未录制", L"Not Recording", L"未録画"},
    // 106
    {L"自定义宽度 (0=默认)", L"Custom Width (0=default)", L"カスタム幅 (0=デフォルト)"},
    // 107
    {L"自定义高度 (0=默认)", L"Custom Height (0=default)", L"カスタム高さ (0=デフォルト)"},
    // 108
    {L"网格吸附", L"Grid Snap", L"グリッドスナップ"},
    // 109
    {L"网格大小 (px)", L"Grid Size (px)", L"グリッドサイズ (px)"},
    // 110
    {L"宽:", L"W:", L"幅:"},
    // 111
    {L"高:", L"H:", L"高:"},
    // 112
    {L"宽 (0=默认)", L"W (0=default)", L"幅 (0=標準)"},
    // 113
    {L"高 (0=默认)", L"H (0=default)", L"高さ (0=標準)"},
    // 114
    {L"上升式", L"Rising", L"上昇式"},
    // 115
    {L"下落式", L"Falling", L"落下式"},
    // 116
    {L"轨道尺寸", L"Track Size", L"軌道サイズ"},
    // 117
    {L"方块动画", L"Block Animation", L"ブロックアニメーション"},
    // 118
    {L"轨道背景透明度", L"Track BG Alpha", L"トラック背景透明度"},
    // 119
    {L"显示区域边界", L"Show Boundary", L"境界表示"},
    // 120
    {L"主题预设", L"Theme Presets", L"テーマプリセット"},
    // 121
    {L"背景色", L"BG Color", L"背景色"},
    // 122
    {L"字体色", L"Font Color", L"フォント色"},
    // 123
    {L"线条色", L"Line Color", L"ライン色"},
    // 124
    {L"宽", L"W", L"幅"},
    // 125
    {L"高", L"H", L"高さ"},
    // 126
    {L"图表类型", L"Chart Type", L"チャートタイプ"},
    // 127
    {L"边距", L"Margins", L"余白"},
    // 128
    {L"颜色", L"Colors", L"色"},
    // 129
    {L"外观", L"Appearance", L"外観"},
    // 130
    {L"吸附", L"Snap", L"スナップ"},
    // 131
    {L"吸附到按键映射下方", L"Snap below keys", L"キー下にスナップ"},
    // 132
    {L"确定", L"OK", L"OK"},
    // 133
    {L"取消", L"Cancel", L"キャンセル"},
    // 134
    {L"重置确认", L"Reset Confirm", L"リセット確認"},
    // 135
    {L"捕获按键", L"Capture Key", L"キーキャプチャ"},
    // 136
    {L"捕获录制快捷键", L"Capture Record Key", L"録画キーキャプチャ"},
    // 137
    {L"名称", L"Name", L"名前"},
    // 138
    {L"音符划分", L"Note Division", L"音符分割"},
    // 139
    {L"当前字体: %s", L"Current Font: %s", L"現在のフォント: %s"},
    // 140
    {L"选择字体...", L"Choose Font...", L"フォント選択..."},
    // 141
    {L"轨道方向", L"Track Direction", L"軌道方向"},
    // 142
    {L"自由模式", L"Free Mode", L"フリーモード"},
    // 143
    {L"布局", L"Layout", L"レイアウト"},
    // 144
    {L"尺寸", L"Size", L"サイズ"},
    // 145
    {L"窗口", L"Window", L"ウィンドウ"},
    // 146
    {L"字体", L"Font", L"フォント"},
    // 147
    {L"透明度", L"Alpha", L"透明度"},
    // 148
    {L"按下设置的快捷键开始/停止录制。", L"Press hotkey to start/stop recording.", L"ショートカットキーで録画開始/停止。"},
    // 149
    {L"点击设置录制快捷键…", L"Click to set record hotkey…", L"クリックして録画キー設定…"},
    // 150
    {L"边框颜色", L"Border Color", L"枠線色"},
    // 151
    {L"快捷键设置", L"Hotkey Settings", L"ショートカット設定"},
    // 152
    {L"UI 主题", L"UI Theme", L"UIテーマ"},
    // 153
    {L"红", L"Red", L"赤"},
    // 154
    {L"橙", L"Orange", L"橙"},
    // 155
    {L"黄", L"Yellow", L"黄"},
    // 156
    {L"绿", L"Green", L"緑"},
    // 157
    {L"蓝", L"Blue", L"青"},
    // 158
    {L"靛", L"Indigo", L"藍"},
    // 159
    {L"紫", L"Purple", L"紫"},
    // 160
    {L"● 未录制", L"● Idle", L"● 未録画"},
    // 161
    {L"● 录制中", L"● Recording", L"● 録画中"},
    // 162
    {L"输出目录", L"Output Dir", L"出力先"},
    // 163
    {L"选择目录...", L"Browse...", L"選択..."},
    // 164
    {L"轨道:", L"Track:", L"軌道:"},
    // 165
    {L"上浮/下沉速率 (px/s)", L"Rise/Sink Speed (px/s)", L"上昇/下降速度 (px/s)"},
    // 166
    {L"区域宽度 (px)", L"Area Width (px)", L"領域幅 (px)"},
    // 167
    {L"区域高度 (px)", L"Area Height (px)", L"領域高さ (px)"},
    // 168
    {L"宽度 (px)", L"Width (px)", L"幅 (px)"},
    // 169
    {L"高度 (px)", L"Height (px)", L"高さ (px)"},
    // 170
    {L"左边距 (px)", L"Margin L (px)", L"左余白 (px)"},
    // 171
    {L"右边距 (px)", L"Margin R (px)", L"右余白 (px)"},
    // 172
    {L"上边距 (px)", L"Margin T (px)", L"上余白 (px)"},
    // 173
    {L"下边距 (px)", L"Margin B (px)", L"下余白 (px)"},
    // 174
    {L"圆角 (px)", L"Radius (px)", L"角丸 (px)"},
    // 175
    {L"X 偏移 (px)", L"X Offset (px)", L"X オフセット (px)"},
    // 176
    {L"Y 偏移 (px)", L"Y Offset (px)", L"Y オフセット (px)"},
    // 177
    {L"重置默认", L"Reset Default", L"デフォルトに戻す"},
    // 178
    {L"设置面板", L"Settings Panel", L"設定パネル"},
    // 179
    {L"主题编辑器", L"Theme Editor", L"テーマエディタ"},
    // 180
    {L"切换按键映射", L"Toggle Key Display", L"キー表示切替"},
    // 181
    {L"下一个主题", L"Next Theme", L"次のテーマ"},
    // 182
    {L"上一个主题", L"Prev Theme", L"前のテーマ"},
    // 183
    {L"切换轨道", L"Toggle Track", L"トラック切替"},
    // 184
    {L"切换图表", L"Toggle Chart", L"チャート切替"},
    // 185
    {L"所有快捷键均使用 Ctrl+Shift+[按键] 组合。点击按钮后按下新按键即可修改。", L"All hotkeys use Ctrl+Shift+[key]. Click button then press a new key.", L"全てのショートカットは Ctrl+Shift+[キー]。ボタン→新キー押下で変更。"},
    // 186
    {L"按任意键… (ESC取消)", L"Press any key… (ESC cancel)", L"キーを押してください… (ESC取消)"},
    // 187
    {L"编辑: ", L"Edit: ", L"編集: "},
    // 188
    {L"图表背景", L"Chart BG", L"チャート背景"},
    // 189
    {L"图表线条", L"Chart Line", L"チャート線"},
};

const wchar_t* LANG(int key) {
    if (key < 0 || key >= (int)(sizeof(g_strings) / sizeof(g_strings[0]))) return L"";
    int idx = g_lang;
    if (idx < 0 || idx > 2) idx = 0;
    return g_strings[key][idx];
}
