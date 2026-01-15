#pragma once

#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>

#include <vector>
#include <d3d12.h>

class Sprite;

/*-----------------------------------------------------------------------------------------
 * SpriteRenderer
 * - スプライト描画管理クラス
 * - 登録されたスプライトの一括描画制御を担当
 *---------------------------------------------------------------------------------------*/
class SpriteRenderer {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief スプライトを登録
	 * \param sprite 登録するスプライト
	 */
	void Register(Sprite* sprite);

	/**
	 * \brief 描画処理
	 * \param cmdList コマンドリスト
	 * \param psoService パイプラインサービス
	 * \param renderTargetType 描画ターゲットタイプ
	 */
	void Draw(ID3D12GraphicsCommandList* cmdList,
			  class PipelineService* psoService,
			  RenderTargetType renderTargetType);

	/**
	 * \brief 登録解除（クリア）
	 */
	void Clear();

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	std::vector<Sprite*> sprites_; //< 描画対象のスプライトリスト
};