#include "config.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <locale>

// ========== 简易 JSON 解析器 ==========
namespace json {

enum class Type { Object, Array, String, Number, Boolean, Null };

struct Value;
using Object = std::vector<std::pair<std::wstring, Value>>;
using Array  = std::vector<Value>;

struct Value {
    Type type = Type::Null;
    std::wstring str;
    double num = 0;
    bool   bl  = false;
    Object obj;
    Array  arr;

    const Value* Find(const std::wstring& key) const {
        if (type != Type::Object) return nullptr;
        for (auto& p : obj) if (p.first == key) return &p.second;
        return nullptr;
    }
};

// 跳过空白
static void SkipWS(const std::wstring& s, size_t& i) {
    while (i < s.size() && (s[i] == L' ' || s[i] == L'\t' || s[i] == L'\n' || s[i] == L'\r')) ++i;
}

static Value ParseValue(const std::wstring& s, size_t& i);

static std::wstring ParseString(const std::wstring& s, size_t& i) {
    if (s[i] != L'"') return {};
    ++i;
    std::wstring out;
    while (i < s.size() && s[i] != L'"') {
        if (s[i] == L'\\') { ++i; if (i < s.size()) out += s[i]; }
        else out += s[i];
        ++i;
    }
    if (i < s.size()) ++i; // skip closing "
    return out;
}

static Value ParseObject(const std::wstring& s, size_t& i) {
    Value v; v.type = Type::Object;
    ++i; SkipWS(s, i);
    if (s[i] == L'}') { ++i; return v; }
    while (true) {
        SkipWS(s, i);
        std::wstring key = ParseString(s, i);
        SkipWS(s, i);
        if (s[i] == L':') ++i;
        SkipWS(s, i);
        Value val = ParseValue(s, i);
        v.obj.push_back({key, val});
        SkipWS(s, i);
        if (s[i] == L'}') { ++i; break; }
        if (s[i] == L',') ++i;
    }
    return v;
}

static Value ParseArray(const std::wstring& s, size_t& i) {
    Value v; v.type = Type::Array;
    ++i; SkipWS(s, i);
    if (s[i] == L']') { ++i; return v; }
    while (true) {
        SkipWS(s, i);
        v.arr.push_back(ParseValue(s, i));
        SkipWS(s, i);
        if (s[i] == L']') { ++i; break; }
        if (s[i] == L',') ++i;
    }
    return v;
}

static Value ParseValue(const std::wstring& s, size_t& i) {
    SkipWS(s, i);
    if (s[i] == L'{') return ParseObject(s, i);
    if (s[i] == L'[') return ParseArray(s, i);
    if (s[i] == L'"') { Value v; v.type = Type::String; v.str = ParseString(s, i); return v; }
    if (s[i] == L't' || s[i] == L'f') {
        Value v; v.type = Type::Boolean;
        v.bl = (s[i] == L't');
        while (i < s.size() && iswalpha(s[i])) ++i;
        return v;
    }
    if (s[i] == L'n') { Value v; v.type = Type::Null; i += 4; return v; }
    // number
    size_t start = i;
    if (s[i] == L'-') ++i;
    while (i < s.size() && iswdigit(s[i])) ++i;
    if (i < s.size() && s[i] == L'.') { ++i; while (i < s.size() && iswdigit(s[i])) ++i; }
    Value v; v.type = Type::Number;
    v.num = std::wcstod(s.substr(start, i - start).c_str(), nullptr);
    return v;
}

static Value Parse(const std::wstring& s) {
    size_t i = 0;
    SkipWS(s, i);
    return ParseValue(s, i);
}
} // namespace json

// ========== 加载配置 ==========
bool AppConfig::Load(const wchar_t* filepath) {
    // 读取文件为 wstring
    HANDLE h = CreateFileW(filepath, GENERIC_READ, FILE_SHARE_READ,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD sz = GetFileSize(h, nullptr);
    if (sz == INVALID_FILE_SIZE || sz == 0) { CloseHandle(h); return false; }
    // 读取为 UTF-8
    std::string utf8(sz, '\0');
    DWORD read = 0;
    ReadFile(h, &utf8[0], sz, &read, nullptr);
    CloseHandle(h);
    utf8.resize(read);

    // UTF-8 -> wstring
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wcontent(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wcontent[0], wlen);
    while (!wcontent.empty() && wcontent.back() == L'\0') wcontent.pop_back();

    auto root = json::Parse(wcontent);
    if (root.type != json::Type::Object) return false;

    // 解析字段
    if (auto* v = root.Find(L"version"))        version   = (int)v->num;
    if (auto* v = root.Find(L"showTotal"))      showTotal = v->bl;
    if (auto* v = root.Find(L"showKPS"))        showKPS   = v->bl;
    if (auto* v = root.Find(L"showBPM"))        showBPM   = v->bl;
    if (auto* v = root.Find(L"bpmNoteDiv"))     bpmNoteDiv = (int)v->num;
    if (auto* v = root.Find(L"bpmMergeMs"))     bpmMergeMs = (int)v->num;
    if (auto* v = root.Find(L"lang"))           lang = (int)v->num;
    auto rdBoxCol = [&](const wchar_t* k, RgbaColor& d) { auto* co=root.Find(k); if(co&&co->type==json::Type::Object){ if(auto*x=co->Find(L"r"))d.r=(uint8_t)x->num; if(auto*x=co->Find(L"g"))d.g=(uint8_t)x->num; if(auto*x=co->Find(L"b"))d.b=(uint8_t)x->num; if(auto*x=co->Find(L"a"))d.a=(uint8_t)x->num; } };
    rdBoxCol(L"totalBoxBg",totalBoxBg); rdBoxCol(L"totalBoxFc",totalBoxFc);
    rdBoxCol(L"kpsBoxBg",kpsBoxBg);   rdBoxCol(L"kpsBoxFc",kpsBoxFc);
    rdBoxCol(L"bpmBoxBg",bpmBoxBg);   rdBoxCol(L"bpmBoxFc",bpmBoxFc);
    if (auto* v = root.Find(L"showSummary"))    showSummary = v->bl;
    if (auto* v = root.Find(L"showHistory"))      showHistory = v->bl;
    if (auto* v = root.Find(L"historyTrackH"))    historyTrackH = (int)v->num;
    if (auto* v = root.Find(L"historyGrowSpeed")) historyGrowSpeed = (int)v->num;
    if (auto* v = root.Find(L"historyFloatSpeed"))historyFloatSpeed = (int)v->num;
    if (auto* v = root.Find(L"historyBlockMax"))  historyBlockMax = (int)v->num;
    if (auto* v = root.Find(L"historyTrackGap"))  historyTrackGap = (int)v->num;
    if (auto* v = root.Find(L"historyTrackAlpha"))historyTrackAlpha=(int)v->num;
    if (auto* v = root.Find(L"historyBlockAlpha"))historyBlockAlpha=(int)v->num;
    if (auto* v = root.Find(L"historyShowLines")) historyShowLines = v->bl;
    if (auto* v = root.Find(L"fps"))              fps = (int)v->num;
    if (auto* v = root.Find(L"clickThrough"))     clickThrough = v->bl;
    if (auto* v = root.Find(L"alwaysOnTop"))      alwaysOnTop  = v->bl;
    if (auto* v = root.Find(L"overlayOpacity")) overlayOpacity = (float)v->num;
    if (auto* v = root.Find(L"keySpacing"))     keySpacing = (int)v->num;
    if (auto* v = root.Find(L"keySize"))        keySize    = (int)v->num;
    if (auto* v = root.Find(L"keyRadius"))      keyRadius  = (int)v->num;
    if (auto* v = root.Find(L"keyBorderW"))     keyBorderW = (int)v->num;
    if (auto* v = root.Find(L"keyFontSize"))    keyFontSize= (int)v->num;
    if (auto* v = root.Find(L"keyFontName"))    keyFontName= v->str;
    if (auto* v = root.Find(L"theme"))          theme      = (int)v->num;
    if (auto* v = root.Find(L"displayX"))       displayX   = (int)v->num;
    if (auto* v = root.Find(L"displayY"))       displayY   = (int)v->num;
    if (auto* v = root.Find(L"displayW"))       displayW   = (int)v->num;
    if (auto* v = root.Find(L"displayH"))       displayH   = (int)v->num;
    if (auto* v = root.Find(L"chartX"))         chartX     = (int)v->num;
    if (auto* v = root.Find(L"chartY"))         chartY     = (int)v->num;
    if (auto* v = root.Find(L"chartW"))         chartW     = (int)v->num;
    if (auto* v = root.Find(L"chartH"))         chartH     = (int)v->num;
    if (auto* v = root.Find(L"chartMarginL"))   chartMarginL = (int)v->num;
    if (auto* v = root.Find(L"chartMarginR"))   chartMarginR = (int)v->num;
    if (auto* v = root.Find(L"chartMarginT"))   chartMarginT = (int)v->num;
    if (auto* v = root.Find(L"chartMarginB"))   chartMarginB = (int)v->num;
    if (auto* v = root.Find(L"showChart"))      showChart  = v->bl;
    if (auto* v = root.Find(L"chartTimeRange")) chartTimeRange = (int)v->num;
    rdBoxCol(L"chartBgCol",   chartBgCol);
    rdBoxCol(L"chartLineCol", chartLineCol);
    if (auto* v = root.Find(L"chartRadius"))    chartRadius = (int)v->num;
    if (auto* v = root.Find(L"chartShowGrid"))  chartShowGrid = v->bl;
    if (auto* v = root.Find(L"chartSnap"))      chartSnap   = v->bl;
    if (auto* v = root.Find(L"chartSnapOffsetX")) chartSnapOffsetX = (int)v->num;
    if (auto* v = root.Find(L"chartSnapOffsetY")) chartSnapOffsetY = (int)v->num;
    if (auto* v = root.Find(L"chartType"))      chartType   = (int)v->num;
    if (auto* v = root.Find(L"chartGradientFill")) chartGradientFill = v->bl;
    if (auto* v = root.Find(L"freeMode"))       freeMode    = v->bl;
    if (auto* v = root.Find(L"freeAreaW"))      freeAreaW   = (int)v->num;
    if (auto* v = root.Find(L"freeAreaH"))      freeAreaH   = (int)v->num;
    if (auto* v = root.Find(L"freeShowBoundary")) freeShowBoundary = v->bl;
    if (auto* v = root.Find(L"freeTotalX"))     freeTotalX  = (int)v->num;
    if (auto* v = root.Find(L"freeTotalY"))     freeTotalY  = (int)v->num;
    if (auto* v = root.Find(L"freeKPSX"))       freeKPSX    = (int)v->num;
    if (auto* v = root.Find(L"freeKPSY"))       freeKPSY    = (int)v->num;
    if (auto* v = root.Find(L"freeBPMX"))       freeBPMX    = (int)v->num;
    if (auto* v = root.Find(L"freeBPMY"))       freeBPMY    = (int)v->num;
    if (auto* v = root.Find(L"recordingHotkeyVK")) recordingHotkeyVK = (int)v->num;

    // 解析 keys 数组
    if (auto* jkeys = root.Find(L"keys")) {
        if (jkeys->type == json::Type::Array) {
            keys.clear();
            for (auto& jk : jkeys->arr) {
                if (jk.type != json::Type::Object) continue;
                KeyConfig kc;  // 保留内置默认色
                if (auto* v = jk.Find(L"keyCode")) kc.keyCode = (int)v->num;
                if (auto* v = jk.Find(L"label"))   kc.label   = v->str;
                if (auto* v = jk.Find(L"totalPresses")) kc.totalPresses = (uint64_t)v->num;
                if (auto* v = jk.Find(L"freeX"))   kc.freeX   = (int)v->num;
                if (auto* v = jk.Find(L"freeY"))   kc.freeY   = (int)v->num;

                // 仅当 JSON 中有颜色对象时才更新（否则保留 KeyConfig 默认色）
                auto applyColor = [&](const wchar_t* name, RgbaColor& dst) {
                    auto* co = jk.Find(name);
                    if (!co || co->type != json::Type::Object) return; // 不覆盖
                    if (auto* x = co->Find(L"r")) dst.r = (uint8_t)x->num;
                    if (auto* x = co->Find(L"g")) dst.g = (uint8_t)x->num;
                    if (auto* x = co->Find(L"b")) dst.b = (uint8_t)x->num;
                    if (auto* x = co->Find(L"a")) dst.a = (uint8_t)x->num;
                };
                applyColor(L"colorFont",   kc.colorFont);
                applyColor(L"colorNormal", kc.colorNormal);
                applyColor(L"colorPress",  kc.colorPress);

                keys.push_back(kc);
            }
        }
    }
    return true;
}

// ========== 保存配置 ==========
static void WriteIndent(std::wstring& out, int indent) {
    for (int i = 0; i < indent; ++i) out += L"  ";
}
static void WriteColor(std::wstring& out, int indent, const wchar_t* name, const RgbaColor& c) {
    WriteIndent(out, indent);
    wchar_t buf[128];
    swprintf(buf, 128, L"\"%ls\": {\"r\":%d,\"g\":%d,\"b\":%d,\"a\":%d}", name, c.r, c.g, c.b, c.a);
    out += buf;
}

bool AppConfig::Save(const wchar_t* filepath) const {
    std::wstring out;
    out += L"{\n";

    auto wInt = [&](const wchar_t* key, int v) {
        WriteIndent(out, 1);
        out += L"\""; out += key; out += L"\": "; out += std::to_wstring(v); out += L",\n";
    };
    auto wBool = [&](const wchar_t* key, bool v) {
        WriteIndent(out, 1);
        out += L"\""; out += key; out += L"\": "; out += (v ? L"true" : L"false"); out += L",\n";
    };

    wInt(L"version", version);
    wBool(L"showTotal", showTotal);
    wBool(L"showKPS", showKPS);
    wBool(L"showBPM", showBPM);
    wInt(L"bpmNoteDiv", bpmNoteDiv);
    wInt(L"bpmMergeMs", bpmMergeMs);
    wInt(L"lang", lang);
    wBool(L"showSummary", showSummary);
    wBool(L"showHistory", showHistory);
    wInt(L"historyTrackH", historyTrackH);
    wInt(L"historyGrowSpeed", historyGrowSpeed);
    wInt(L"historyFloatSpeed", historyFloatSpeed);
    wInt(L"historyBlockMax", historyBlockMax);
    wInt(L"historyTrackGap", historyTrackGap);
    wInt(L"historyTrackAlpha", historyTrackAlpha);
    wInt(L"historyBlockAlpha", historyBlockAlpha);
    wBool(L"historyShowLines", historyShowLines);
    auto wBoxCol = [&](const wchar_t* k, const RgbaColor& c) { WriteColor(out,1,k,c); out+=L",\n"; };
    wBoxCol(L"totalBoxBg",totalBoxBg); wBoxCol(L"totalBoxFc",totalBoxFc);
    wBoxCol(L"kpsBoxBg",kpsBoxBg);   wBoxCol(L"kpsBoxFc",kpsBoxFc);
    wBoxCol(L"bpmBoxBg",bpmBoxBg);   wBoxCol(L"bpmBoxFc",bpmBoxFc);
    wInt(L"fps", fps);
    wBool(L"clickThrough", clickThrough);
    wBool(L"alwaysOnTop", alwaysOnTop);
    WriteIndent(out, 1); out += L"\"overlayOpacity\": " + std::to_wstring(overlayOpacity) + L",\n";
    wInt(L"keySpacing", keySpacing);
    wInt(L"keySize", keySize);
    wInt(L"keyRadius", keyRadius);
    wInt(L"keyBorderW", keyBorderW);
    wInt(L"keyFontSize", keyFontSize);
    WriteIndent(out, 1); out += L"\"keyFontName\": \"" + keyFontName + L"\",\n";
    wInt(L"theme", theme);
    wInt(L"displayX", displayX);
    wInt(L"displayY", displayY);
    wInt(L"displayW", displayW);
    wInt(L"displayH", displayH);
    wInt(L"chartX", chartX);
    wInt(L"chartY", chartY);
    wInt(L"chartW", chartW);
    wInt(L"chartH", chartH);
    wInt(L"chartMarginL", chartMarginL);
    wInt(L"chartMarginR", chartMarginR);
    wInt(L"chartMarginT", chartMarginT);
    wInt(L"chartMarginB", chartMarginB);
    wBool(L"showChart", showChart);
    wInt(L"chartTimeRange", chartTimeRange);
    auto wChartCol = [&](const wchar_t* k, const RgbaColor& c) { WriteColor(out,1,k,c); out+=L",\n"; };
    wChartCol(L"chartBgCol",   chartBgCol);
    wChartCol(L"chartLineCol", chartLineCol);
    wInt(L"chartRadius", chartRadius);
    wBool(L"chartShowGrid", chartShowGrid);
    wBool(L"chartSnap", chartSnap);
    wInt(L"chartSnapOffsetX", chartSnapOffsetX);
    wInt(L"chartSnapOffsetY", chartSnapOffsetY);
    wInt(L"chartType", chartType);
    wBool(L"chartGradientFill", chartGradientFill);
    wBool(L"freeMode", freeMode);
    wInt(L"freeAreaW", freeAreaW);
    wInt(L"freeAreaH", freeAreaH);
    wBool(L"freeShowBoundary", freeShowBoundary);
    wInt(L"freeTotalX", freeTotalX);
    wInt(L"freeTotalY", freeTotalY);
    wInt(L"freeKPSX", freeKPSX);
    wInt(L"freeKPSY", freeKPSY);
    wInt(L"freeBPMX", freeBPMX);
    wInt(L"freeBPMY", freeBPMY);
    wInt(L"recordingHotkeyVK", recordingHotkeyVK);

    // keys 数组
    WriteIndent(out, 1); out += L"\"keys\": [\n";
    for (size_t ki = 0; ki < keys.size(); ++ki) {
        auto& kc = keys[ki];
        WriteIndent(out, 2); out += L"{\n";
        WriteIndent(out, 3); out += L"\"keyCode\": " + std::to_wstring(kc.keyCode) + L",\n";
        WriteIndent(out, 3); out += L"\"label\": \"" + kc.label + L"\",\n";
        WriteColor(out, 3, L"colorFont",   kc.colorFont);   out += L",\n";
        WriteColor(out, 3, L"colorNormal", kc.colorNormal); out += L",\n";
        WriteColor(out, 3, L"colorPress",  kc.colorPress);  out += L",\n";
        WriteIndent(out, 3); out += L"\"totalPresses\": " + std::to_wstring(kc.totalPresses) + L",\n";
        WriteIndent(out, 3); out += L"\"freeX\": " + std::to_wstring(kc.freeX) + L",\n";
        WriteIndent(out, 3); out += L"\"freeY\": " + std::to_wstring(kc.freeY);
        out += L"\n";
        WriteIndent(out, 2); out += L"}";
        if (ki + 1 < keys.size()) out += L",";
        out += L"\n";
    }
    WriteIndent(out, 1); out += L"]\n";
    out += L"}\n";

    // wstring -> UTF-8
    int ulen = WideCharToMultiByte(CP_UTF8, 0, out.c_str(), (int)out.size(), nullptr, 0, nullptr, nullptr);
    std::string utf8(ulen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, out.c_str(), (int)out.size(), &utf8[0], ulen, nullptr, nullptr);

    HANDLE h = CreateFileW(filepath, GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    WriteFile(h, utf8.c_str(), (DWORD)utf8.size(), &written, nullptr);
    CloseHandle(h);
    return true;
}

// ========== ThemePreset 默认值 ==========
void AppConfig::InitDefaultThemes() {
    themePresets.resize(16);
    // 0 空
    auto set = [&](int i, const wchar_t* n, RgbaColor f, RgbaColor nm, RgbaColor p, RgbaColor cb, RgbaColor cl) {
        themePresets[i].name=n; themePresets[i].font=f; themePresets[i].normal=nm;
        themePresets[i].press=p; themePresets[i].chartBg=cb; themePresets[i].chartLine=cl;
    };
    set(1, L"\u9884\u8BBE1",  {255,255,255,255},{255,107,107,255},{200,50,50,255},{255,248,240,255},{255,107,107,255});
    set(2, L"\u9884\u8BBE2",  {255,255,255,255},{0,206,201,255},{0,150,150,255},{240,255,250,255},{0,206,201,255});
    set(3, L"\u9884\u8BBE3",  {255,255,255,255},{108,92,231,255},{70,50,180,255},{245,240,255,255},{108,92,231,255});
    set(4, L"\u9884\u8BBE4",  {255,255,255,255},{255,159,159,255},{220,100,100,255},{255,245,242,255},{255,159,159,255});
    set(5, L"\u9884\u8BBE5",  {255,255,255,255},{72,150,120,255},{40,110,80,255},{240,250,245,255},{72,150,120,255});
    set(6, L"\u9884\u8BBE6",  {255,255,255,255},{54,102,204,255},{30,70,160,255},{240,245,255,255},{54,102,204,255});
    set(7, L"\u9884\u8BBE7",  {255,255,255,255},{255,165,50,255},{220,120,20,255},{255,248,235,255},{255,165,50,255});
    set(8, L"\u9884\u8BBE8",  {255,255,255,255},{160,130,210,255},{120,90,180,255},{248,242,255,255},{160,130,210,255});
    set(9, L"\u9884\u8BBE9",  {255,255,255,255},{255,182,193,255},{220,130,145,255},{255,248,248,255},{255,182,193,255});
    set(10, L"\u9884\u8BBE10", {255,255,255,255},{30,80,70,255},{20,55,45,255},{240,248,245,255},{30,80,70,255});
    set(11, L"\u9884\u8BBE11", {255,255,255,255},{255,215,0,255},{210,170,0,255},{255,252,240,255},{255,215,0,255});
    set(12, L"\u9884\u8BBE12", {255,255,255,255},{120,144,156,255},{80,100,110,255},{245,248,250,255},{120,144,156,255});
    set(13, L"\u9884\u8BBE13", {255,255,255,255},{220,50,60,255},{170,30,40,255},{255,240,240,255},{220,50,60,255});
    set(14, L"\u9884\u8BBE14", {255,255,255,255},{0,200,180,255},{0,150,135,255},{240,255,252,255},{0,200,180,255});
    set(15, L"\u9884\u8BBE15", {255,255,255,255},{90,50,140,255},{60,30,100,255},{248,240,255,255},{90,50,140,255});
}

bool AppConfig::LoadThemes(const wchar_t* filepath) {
    themePresets.clear();
    InitDefaultThemes();  // 先填默认值

    HANDLE h = CreateFileW(filepath, GENERIC_READ, FILE_SHARE_READ,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD sz = GetFileSize(h, nullptr);
    if (sz == INVALID_FILE_SIZE || sz == 0) { CloseHandle(h); return false; }
    std::string utf8(sz, '\0'); DWORD read = 0;
    ReadFile(h, &utf8[0], sz, &read, nullptr); CloseHandle(h);
    utf8.resize(read);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wcontent(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wcontent[0], wlen);
    while (!wcontent.empty() && wcontent.back() == L'\0') wcontent.pop_back();

    auto root = json::Parse(wcontent);
    if (root.type != json::Type::Object) return false;
    auto* jarr = root.Find(L"themes");
    if (!jarr || jarr->type != json::Type::Array) return false;

    auto readCol = [](const json::Value& v, RgbaColor& c) {
        if (auto* x = v.Find(L"r")) c.r = (uint8_t)x->num;
        if (auto* x = v.Find(L"g")) c.g = (uint8_t)x->num;
        if (auto* x = v.Find(L"b")) c.b = (uint8_t)x->num;
        if (auto* x = v.Find(L"a")) c.a = (uint8_t)x->num;
    };
    for (size_t ti = 0; ti < jarr->arr.size() && ti < 16; ++ti) {
        auto& tp = themePresets[ti];
        auto& obj = jarr->arr[ti];
        if (obj.type != json::Type::Object) continue;
        if (auto* v = obj.Find(L"name"))      tp.name = v->str;
        if (auto* v = obj.Find(L"font"))      readCol(*v, tp.font);
        if (auto* v = obj.Find(L"normal"))    readCol(*v, tp.normal);
        if (auto* v = obj.Find(L"press"))     readCol(*v, tp.press);
        if (auto* v = obj.Find(L"chartBg"))   readCol(*v, tp.chartBg);
        if (auto* v = obj.Find(L"chartLine")) readCol(*v, tp.chartLine);
    }
    return true;
}

bool AppConfig::SaveThemes(const wchar_t* filepath) const {
    std::wstring out;
    out += L"{\n  \"themes\": [\n";
    for (size_t ti = 0; ti < themePresets.size(); ++ti) {
        auto& tp = themePresets[ti];
        auto wCol = [&](const wchar_t* name, const RgbaColor& c) {
            wchar_t b[128];
            swprintf(b, 128, L"\"%ls\":{\"r\":%d,\"g\":%d,\"b\":%d,\"a\":%d}", name, c.r, c.g, c.b, c.a);
            out += b;
        };
        out += L"    {";
        out += L"\"name\":\""; out += tp.name; out += L"\",";
        wCol(L"font", tp.font);      out += L",";
        wCol(L"normal", tp.normal);  out += L",";
        wCol(L"press", tp.press);    out += L",";
        wCol(L"chartBg", tp.chartBg);out += L",";
        wCol(L"chartLine", tp.chartLine);
        out += L"}";
        if (ti + 1 < themePresets.size()) out += L",";
        out += L"\n";
    }
    out += L"  ]\n}\n";

    int ulen = WideCharToMultiByte(CP_UTF8, 0, out.c_str(), (int)out.size(), nullptr, 0, nullptr, nullptr);
    std::string utf8(ulen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, out.c_str(), (int)out.size(), &utf8[0], ulen, nullptr, nullptr);
    HANDLE h = CreateFileW(filepath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    WriteFile(h, utf8.c_str(), (DWORD)utf8.size(), &written, nullptr);
    CloseHandle(h);
    return true;
}
