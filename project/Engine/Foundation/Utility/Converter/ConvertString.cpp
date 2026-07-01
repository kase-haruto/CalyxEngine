#include"ConvertString.h"
#include <Engine/Foundation/Log/EngineLogger.h>
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <string>

void Log(const std::string& message){
	ConvertString(message);
	OutputDebugStringA(message.c_str());

	// 既存のDirectX・シェーダーデバッグ出力もエディタのログ画面へ集約する。
	std::string displayMessage = message;
	while(!displayMessage.empty() && (displayMessage.back() == '\n' || displayMessage.back() == '\r')) {
		displayMessage.pop_back();
	}
	if(displayMessage.empty()) return;

	std::string lowerMessage = displayMessage;
	std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});

	CalyxEngine::LogLevel level = CalyxEngine::LogLevel::Trace;
	if(lowerMessage.find("error") != std::string::npos ||
	   lowerMessage.find("failed") != std::string::npos ||
	   lowerMessage.find("failure") != std::string::npos) {
		level = CalyxEngine::LogLevel::Error;
	} else if(lowerMessage.find("warning") != std::string::npos) {
		level = CalyxEngine::LogLevel::Warning;
	}

	CalyxEngine::EngineLogger::GetInstance().Add(
		level,
		CalyxEngine::LogCategory::Rendering,
		displayMessage,
		"GraphicsDebugOutput");
}

std::wstring ConvertString(const std::string& str){
	if (str.empty()){
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast< const char* >(&str[0]), static_cast< int >(str.size()), NULL, 0);
	if (sizeNeeded == 0){
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast< const char* >(&str[0]), static_cast< int >(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str){
	if (str.empty()){
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast< int >(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0){
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast< int >(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

std::wstring ConvertString(const std::strong_ordering& str){
	if (str == std::strong_ordering::equal){
		return L"equal";
	} else if (str == std::strong_ordering::greater){
		return L"greater";
	} else{
		return L"less";
	}
}
