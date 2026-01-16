#include "WinApp.h"

/* lib */
#include <Engine/Foundation/Utility/Converter/ConvertString.h>

/* externals */
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

/* c++ */
#include <cassert>
#include <tchar.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hand, UINT msg, WPARAM wparam, LPARAM lparam);

// コンストラクタ
WinApp::WinApp(const int wWidth, const int wHeight, const std::string windowName) {
	wrc.right	= wWidth;
	wrc.bottom	= wHeight;
	windowName_ = windowName;
	CreateWnd();
	SetBorderlessFullscreen(false);
}

// デストラクタ
WinApp::~WinApp() {
	CloseWindow(hwnd);
}

// ウィンドウ作成処理
void WinApp::CreateWnd() {
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.style		 = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	 = WindowProc;
	wc.hInstance	 = GetModuleHandle(nullptr);
	wc.hCursor		 = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = _T("WinApp");

	RegisterClassEx(&wc);

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	hwnd = CreateWindow(
		wc.lpszClassName,
		// マルチバイト→ワイド文字変換が必要な場合は適宜対応する
		_T("MyGameWindow"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr);

	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	ShowWindow(hwnd, SW_MAXIMIZE);
}

// ウィンドウ破棄
void WinApp::TerminateGameWindow() {
	UnregisterClass(wc.lpszClassName, wc.hInstance);
	CoUninitialize();
}

// メッセージループ
bool WinApp::ProcessMessage() {
	MSG msg{};
	while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (msg.message == WM_QUIT);
}

// ウィンドウプロシージャ
LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	// ImGui用の処理
	if(ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}

	// このウィンドウに対応するWinAppインスタンスを取得
	WinApp* pThis = reinterpret_cast<WinApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if(pThis) {
	}

	switch(msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_SIZE:
		if (pThis && pThis->resizeCallback_ && wparam != SIZE_MINIMIZED) {
			int width = LOWORD(lparam);
			int height = HIWORD(lparam);
			pThis->resizeCallback_(width, height);
		}
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//------------------------------------------------------------------------------
// ボーダーレス フルスクリーンの切り替え
//------------------------------------------------------------------------------
void WinApp::SetBorderlessFullscreen(bool enable) {

	if(enable) {
		// 今のウィンドウの情報を保存
		GetWindowRect(hwnd, &wrc);

		// ウィンドウがあるモニターの領域を取得
		HMONITOR	hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi{};
		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hMon, &mi);

		// スタイルを境界線なしのポップアップへ変更
		SetWindowLong(hwnd, GWL_STYLE, WS_POPUP);
		SetWindowLong(hwnd, GWL_EXSTYLE, 0);

		// ウィンドウをモニター領域いっぱいに拡大
		SetWindowPos(
			hwnd,
			HWND_TOP,
			mi.rcMonitor.left,
			mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
		ShowWindow(hwnd, SW_MAXIMIZE);
	} else {
		// 通常のウィンドウスタイルに戻す

		// 以前の位置・サイズに復元
		SetWindowPos(
			hwnd,
			0,
			wrc.left,
			wrc.top,
			wrc.right - wrc.left,
			wrc.bottom - wrc.top,
			SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
		ShowWindow(hwnd, SW_SHOWNORMAL);
	}
}