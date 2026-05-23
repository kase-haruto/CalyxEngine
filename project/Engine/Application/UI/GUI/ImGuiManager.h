#pragma once



#include<d3d12.h>
#include <wrl.h>

class WinApp;
namespace CalyxEngine {
	class DxCore;
}

/*-----------------------------------------------------------------------------------------
 * ImGuiManager
 * - ImGui管理クラス
 * - ImGuiの初期化、フレームの開始/終了、描画およびスタイルのカスタマイズを管理
 *---------------------------------------------------------------------------------------*/
class ImGuiManager{
public:
	/**
	 * \brief コンストラクタ
	 */
	ImGuiManager() = default;
	/**
	 * \brief デストラクタ
	 */
	~ImGuiManager() = default;

	/**
	 * \brief 初期化
	 * \param winApp ウィンドウアプリケーション
	 * \param dxCore DirectXコア
	 */
	void Initialize(WinApp* winApp, const CalyxEngine::DxCore* dxCore);
	/**
	 * \brief 終了処理
	 */
	void Finalize();
	/**
	 * \brief フレーム開始
	 */
	void Begin();
	/**
	 * \brief フレーム終了
	 */
	void End();
	/**
	 * \brief 描画
	 */
	void Draw();

private:
	void CustomizeImGuiStyle();
private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	const CalyxEngine::DxCore* pDxCore_ = nullptr; //< DirectXコアへのポインタ
};

