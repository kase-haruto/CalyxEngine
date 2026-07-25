#pragma once
#include "Engine/Graphics/Device/DxCore.h"

#include <Engine/Graphics/GpuResource/DxGpuResource.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>
#include <Engine/PostProcess/Slot/PostEffectSlot.h>
#include <externals/nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace CalyxEngine {
	class CalyxEngine::DxCore;
}
class PostProcessCollection;
class IRenderTarget;

/*-----------------------------------------------------------------------------------------
 * PostEffectGraph
 * - ポストエフェクトグラフクラス
 * - 複数のポストエフェクトパスを連結して実行するパイプラインを管理
 *---------------------------------------------------------------------------------------*/
class PostEffectGraph {
public:
	/**
	 * \brief ポストエフェクトの登録先を参照して実行グラフを構築する
	 * \param postProcessCollection 所有権を持たないポストエフェクト管理機能
	 */
	PostEffectGraph(PostProcessCollection* postProcessCollection)
		: postProcessCollection_(postProcessCollection){}

	/**
	 * \brief 有効なパス一覧から従来互換の線形実行順を設定する
	 * \param slots ポストエフェクトの有効状態と実体を保持する一覧
	 */
	void SetPassesFromList(const std::vector<PostEffectSlot>& slots);
	/**
	 * \brief JSONのノードとリンクから実行グラフを復元する
	 * \param root グラフ設定を含むJSON
	 * \param slots ノード型と実体を関連付けるポストエフェクト一覧
	 */
	void SetGraphFromJson(const nlohmann::json& root, const std::vector<PostEffectSlot>& slots);

	/**
	 * \brief ノードグラフまたは線形パスを実行して最終RenderTargetへ出力する
	 * \param cmd 描画命令とResourceBarrierを記録するコマンドリスト
	 * \param input ポストエフェクトへ入力するScene描画リソース
	 * \param finalTarget 最終結果を書き込むRenderTarget
	 * \param dxCore 一時RenderTargetを取得するDirectX12基盤
	 * \pre 全ポインタは呼出し中有効であること
	 */
	void Execute(ID3D12GraphicsCommandList* cmd,
				 DxGpuResource* input,
				 IRenderTarget* finalTarget,
				 CalyxEngine::DxCore* dxCore);

private:
	/*-----------------------------------------------------------------------------------------
	 * GraphNode
	 * - JSONから復元した1個のポストエフェクトノードを保持する内部データ構造
	 * - ピン接続と非所有のEffect Pass参照を管理し、GPUリソースは所有しない
	 *---------------------------------------------------------------------------------------*/
	struct GraphNode {
		int32_t id = 0;                    //< JSON内でノードを識別するID。0は無効値
		std::string type;                  //< Effect Passとの関連付けに使用するノード型名
		std::vector<int32_t> inputPins;    //< 入力リンクを受けるピンID一覧
		std::vector<int32_t> outputPins;   //< 後続ノードへ接続するピンID一覧
		IPostEffectPass* pass = nullptr;   //< 所有権を持たない実行対象Effect Pass
	};

	/**
	 * \brief 依存ノードを再帰評価して指定ノードの出力SRVを取得する
	 * \param cmd 描画命令を記録するコマンドリスト
	 * \param nodeId 評価するノードID
	 * \param sceneSRV グラフ入力となるSceneのSRV
	 * \param dxCore 一時RenderTargetを取得するDirectX12基盤
	 * \param cache 評価済みノードの出力SRVキャッシュ
	 * \param visiting 循環リンクを検出する再帰訪問状態
	 * \param tempIndex 次に使用する一時RenderTarget番号
	 * \return 指定ノードの出力SRV。評価不能時はsceneSRV
	 */
	D3D12_GPU_DESCRIPTOR_HANDLE ExecuteGraphNode(ID3D12GraphicsCommandList* cmd,
												 int32_t nodeId,
												 D3D12_GPU_DESCRIPTOR_HANDLE sceneSRV,
												 CalyxEngine::DxCore* dxCore,
												 std::unordered_map<int32_t, D3D12_GPU_DESCRIPTOR_HANDLE>& cache,
												 std::unordered_map<int32_t, bool>& visiting,
												 int& tempIndex);
	/**
	 * \brief ノード出力に使用する一時RenderTargetを取得する
	 * \param dxCore RenderTarget Collectionを所有するDirectX12基盤
	 * \param tempIndex 次に使用する番号。取得試行後に加算される
	 * \return 所有権を持たない一時RenderTarget。取得失敗時はnullptr
	 */
	IRenderTarget* AcquireTempTarget(CalyxEngine::DxCore* dxCore, int& tempIndex) const;
	/** \brief 入力ピンへ接続された出力元ノードを検索する \param inputPinId 入力ピンID \return 接続元ノードID。未接続時は0 */
	int32_t FindInputSourceNode(int32_t inputPinId) const;
	/** \brief Outputノードへ接続された最終処理ノードを検索する \return 最終処理ノードID。未接続時は0 */
	int32_t FindOutputSourceNode() const;

	std::vector<IPostEffectPass*> passes_;                         //< 所有権を持たない線形実行用Effect Pass一覧
	PostProcessCollection* postProcessCollection_ = nullptr;       //< 所有権を持たないEffect Pass登録先
	bool useNodeGraph_ = false;                                    //< ノードグラフ方式で実行するか
	std::unordered_map<int32_t, GraphNode> graphNodes_;            //< ノードIDから復元済みノードへの対応表
	std::unordered_map<int32_t, int32_t> pinOwner_;                //< ピンIDから所有ノードIDへの対応表
	std::unordered_map<int32_t, int32_t> inputPinToSourceNode_;    //< 入力ピンIDから接続元ノードIDへの対応表
	int32_t outputNodeId_ = 0;                                     //< OutputノードID。未設定時は0
};
