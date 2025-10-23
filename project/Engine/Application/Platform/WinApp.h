#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

/// <summary>
/// ウィンドウクラス
/// </summary>
class WinApp{
public:
	WinApp(const int wWidth, const int wHeight, const std::string windowName);
	~WinApp();

	/// <summary>
	/// ウィンドウプロシージャ
	/// </summary>
	/// <param name="hwnd"></param>
	/// <param name="msg"></param>
	/// <param name="wparam"></param>
	/// <param name="lparam"></param>
	/// <returns></returns>
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	/// <summary>
	/// ウィンドウ作成
	/// </summary>
	void CreateWnd();

	/// <summary>
	/// ウィンドウ破棄
	/// </summary>
	void TerminateGameWindow();
	
	/// <summary>
	/// message
	/// </summary>
	/// <returns></returns>
	bool ProcessMessage();

	/// <summary>
	/// フルスクリーン切り替え
	/// </summary>
	/// <param name="enable"></param>
	void SetBorderlessFullscreen(bool enable);

	// accessor
	HWND GetHWND() const { return hwnd; }

private:
	// ウィンドウの情報
	WNDCLASSEX wc {};
	// ウィンドウのサイズ
	RECT wrc = {};
	// ウィンドウのハンドル
	HWND hwnd {};
	// ウィンドウタイトル
	std::string windowName_;
	// フルスクリーン状態
	bool isFullScreen = true;
	// ウィンドウの元の位置とサイズ
	WINDOWPLACEMENT windowPlacement = {sizeof(WINDOWPLACEMENT)};
};
