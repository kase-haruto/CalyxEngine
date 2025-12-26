#pragma once
#include<string>
void Log(const std::string& message);
std::wstring ConvertString(const std::string& str);
std::string ConvertString(const std::wstring& str);
std::wstring ConvertString(const std::strong_ordering& str);