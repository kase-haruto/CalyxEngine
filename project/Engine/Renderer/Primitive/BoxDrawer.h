#pragma once
#include <vector>
#include <Engine/Renderer/Mesh/VertexData.h>
#include <Engine/Graphics/Buffer/DxVertexBuffer.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>
struct CalyxEngine::Vector3;
struct CalyxEngine::Vector4;
struct Matrix4x4;

/*-----------------------------------------------------------------------------------------
 * BoxDrawer
 * - デバッグ用ボックス形状を一括描画するプリミティブ描画クラス
 * - CPU頂点の蓄積、GPU頂点バッファ更新、描画命令登録を担当
 * - DirectX12デバイスとコマンドリストの所有権は管理しない
 *---------------------------------------------------------------------------------------*/
/**
 * @brief BoxDrawerの機能を提供するクラスです。
 */
class BoxDrawer {
public:
	/** \brief ボックス描画用GPUバッファを初期化する */
	void Initialize();
	/** \brief 1個のボックス頂点を描画キューへ追加する \param center 中心座標 \param rotate 回転 \param size 各軸の全長 \param color 頂点色 */
	void DrawBox(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color);
	/** \brief 蓄積したボックスの描画命令を現在のコマンドリストへ登録する */
	void Render();
	/** \brief フレーム内に蓄積したCPU頂点を破棄する */
	void Clear();
private:
	std::vector<VertexPosColor> vertices_; //< 現在フレームに描画するボックス頂点
	DxVertexBuffer<VertexPosColor> vertexBuffer_; //< 最大ボックス数分の頂点を保持するGPUバッファ
	DxConstantBuffer<TransformationMatrix> transformBuffer_; //< ボックス描画用変換行列のGPU定数バッファ
	static constexpr size_t kMaxBoxes = 256; //< 1フレームに登録可能な最大ボックス数
};
