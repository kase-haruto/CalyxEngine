#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <functional>

/*-----------------------------------------------------------------------------------------
 * WinApp
 * - Windowsアプリケーション管理クラス
 * - ウィンドウの生成、メッセージループの管理、フルスクリーン切り替えなどを担当
 *---------------------------------------------------------------------------------------*/
class WinApp{
public:
	/**
	 * \brief ウィンドウプロシージャ
	 * \param hwnd ウィンドウハンドル
	 * \param msg メッセージ
	 * \param wparam パラメータ
	 * \param lparam パラメータ
	 * \return 処理結果
	 */
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	/**
	 * \brief コンストラクタ
	 * \param wWidth ウィンドウ幅
	 * \param wHeight ウィンドウ高さ
	 * \param windowName ウィンドウタイトル
	 */
	WinApp(const int wWidth, const int wHeight, const std::string windowName);

	/**
	 * \brief デストラクタ
	 */
	~WinApp();

	/**
	 * \brief ウィンドウを作成
	 */
	void CreateWnd();

	/**
	 * \brief ウィンドウハンドルを取得
	 * \return ウィンドウハンドル
	 */
	HWND GetHWND() const{ return hwnd; }

	/**
	 * \brief ゲームウィンドウの破棄
	 */
	void TerminateGameWindow();

	/**
	 * \brief メッセージ処理
	 * \return アプリケーションを終了するか
	 */
	bool ProcessMessage();

	/**
	 * \brief フルスクリーン切り替え
	 * \param enable 有効にするか
	 */
	void SetBorderlessFullscreen(bool enable);

	/**
	 * \brief リサイズコールバックを登録
	 * \param callback コールバック関数
	 */
	void SetResizeCallback(std::function<void(int, int)> callback) { resizeCallback_ = callback; }

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	WNDCLASSEX wc {}; //< ウィンドウクラス情報
	RECT wrc = {}; //< ウィンドウサイズ情報
	HWND hwnd {}; //< ウィンドウハンドル
	std::string windowName_; //< ウィンドウタイトル
	bool isFullScreen = true; //< フルスクリーンフラグ
	WINDOWPLACEMENT windowPlacement = {sizeof(WINDOWPLACEMENT)}; //< ウィンドウの元の位置とサイズ
	
	std::function<void(int, int)> resizeCallback_; //< リサイズコールバック
};
