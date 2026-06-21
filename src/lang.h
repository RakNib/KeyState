#pragma once

// 语言体系：0=中文 1=English 2=日本語
enum class Lang { CN = 0, EN = 1, JP = 2 };

void SetLanguage(int lang);
int  GetLanguage();
const wchar_t* LANG(int key);
