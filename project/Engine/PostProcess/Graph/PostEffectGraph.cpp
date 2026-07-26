#include "PostEffectGraph.h"
#include <Engine/PostProcess/Collection/PostProcessCollection.h>
#include <Engine/PostProcess/Blend/BlendEffect.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Foundation/Debug/CxAssert.h>
#include <algorithm>
#include <vector>

/*-----------------------------------------------------------------------------------------
 * LinearPostEffectExecutionState
 * - 従来互換の線形ポストエフェクト実行を担当するState
 * - 一時RenderTargetを交互に使用し、GPUリソース自体は所有しない
 *---------------------------------------------------------------------------------------*/
class LinearPostEffectExecutionState final : public IPostEffectExecutionState {
public:
	/** \brief 登録順にPassを実行して最終Targetへコピーする */
	void Execute(PostEffectGraph& graph, ID3D12GraphicsCommandList* cmd, DxGpuResource* input,
				 IRenderTarget* finalTarget, CalyxEngine::DxCore* dxCore) const override;
};

/*-----------------------------------------------------------------------------------------
 * NodePostEffectExecutionState
 * - ノード依存関係を評価するポストエフェクト実行State
 * - 評価キャッシュと循環検出を実行単位で管理し、GPUリソース自体は所有しない
 *---------------------------------------------------------------------------------------*/
class NodePostEffectExecutionState final : public IPostEffectExecutionState {
public:
	/** \brief Outputノードから依存関係を評価して最終Targetへコピーする */
	void Execute(PostEffectGraph& graph, ID3D12GraphicsCommandList* cmd, DxGpuResource* input,
				 IRenderTarget* finalTarget, CalyxEngine::DxCore* dxCore) const override;
};

namespace {
	const LinearPostEffectExecutionState kLinearExecutionState;
	const NodePostEffectExecutionState kNodeExecutionState;
}

PostEffectGraph::PostEffectGraph(PostProcessCollection* postProcessCollection)
	: postProcessCollection_(postProcessCollection),
	  executionState_(&kLinearExecutionState) {}

void PostEffectGraph::SetPassesFromList(const std::vector<PostEffectSlot>& slots){
	// 旧形式の線形実行順を再構築するため、前回のPassとノード接続を全て破棄する。
	passes_.clear();
	executionState_ = &kLinearExecutionState;
	graphNodes_.clear();
	pinOwner_.clear();
	inputPinToSourceNode_.clear();
	outputNodeId_ = 0;
	// Slotの所有権はCollectionに残し、有効なPassへの非所有参照だけを実行順に保持する。
	for (const auto& slot : slots){
		if (slot.enabled && slot.pass){
			passes_.push_back(slot.pass);
		}
	}
}

void PostEffectGraph::SetGraphFromJson(const nlohmann::json& root, const std::vector<PostEffectSlot>& slots){
	// ノード情報がない旧データでも描画できるよう、先に線形互換経路を構築する。
	SetPassesFromList(slots);

	// graphまたはnodesがない場合は、線形実行を維持して読込を正常終了する。
	if(!root.contains("graph") || !root["graph"].is_object()) return;

	const auto& graph = root["graph"];
	if(!graph.contains("nodes") || !graph["nodes"].is_array()) return;

	// JSONから新しいグラフを復元する前に、線形初期化で空になった対応表を明示的に初期化する。
	graphNodes_.clear();
	pinOwner_.clear();
	inputPinToSourceNode_.clear();
	outputNodeId_ = 0;

	auto findPass = [&](const std::string& type) -> IPostEffectPass* {
		for(const auto& slot : slots){
			// 無効・Trigger待ちのノードも構造へ残し、実行時の有効状態でバイパスを判断する。
			if(slot.name == type && slot.pass) return slot.pass;
		}
		return nullptr;
	};

	for(const auto& nodeJson : graph["nodes"]){
		// 永続IDと型名が欠けたノードはリンク解決できないため個別に読み飛ばす。
		GraphNode node;
		node.id = nodeJson.value("id", 0);
		node.type = nodeJson.value("type", "");
		if(node.id == 0 || node.type.empty()) continue;
		node.pass = findPass(node.type);
		if(node.type == "Input" || node.type == "Output" || node.pass){
			// 特殊入出力ノードまたは実装Passを解決できたノードだけを登録する。
			if(node.type == "Output") outputNodeId_ = node.id;
			if(nodeJson.contains("inputs") && nodeJson["inputs"].is_array()){
				for(const auto& pin : nodeJson["inputs"]){
					const int32_t pinId = pin.value("id", 0);
					if(pinId == 0) continue;
					node.inputPins.push_back(pinId);
					// Link読込時に入力ピンから所有ノードを逆引きする。
					pinOwner_[pinId] = node.id;
				}
			}
			if(nodeJson.contains("outputs") && nodeJson["outputs"].is_array()){
				for(const auto& pin : nodeJson["outputs"]){
					const int32_t pinId = pin.value("id", 0);
					if(pinId == 0) continue;
					node.outputPins.push_back(pinId);
					pinOwner_[pinId] = node.id;
				}
			}
			graphNodes_[node.id] = std::move(node);
		}
	}

	if(graph.contains("links") && graph["links"].is_array()){
		for(const auto& link : graph["links"]){
			const int32_t fromPin = link.value("fromPinId", 0);
			const int32_t toPin = link.value("toPinId", 0);
			auto fromOwner = pinOwner_.find(fromPin);
			// 壊れたリンクはグラフ全体を無効にせず、そのリンクだけを無視する。
			if(toPin == 0 || fromOwner == pinOwner_.end()) continue;
			inputPinToSourceNode_[toPin] = fromOwner->second;
		}
	}

	// Outputノードまで復元できた場合だけ実行Stateをノード方式へ切り替える。
	if(outputNodeId_ != 0) executionState_ = &kNodeExecutionState;
}

void PostEffectGraph::Execute(ID3D12GraphicsCommandList* cmd,
							  DxGpuResource* input,
							  IRenderTarget* finalTarget,
							  CalyxEngine::DxCore* dxCore){
	// 現在Stateへ実行手順を委譲し、Graph本体は実行方式による条件分岐を持たない。
	executionState_->Execute(*this, cmd, input, finalTarget, dxCore);
}

void NodePostEffectExecutionState::Execute(
	PostEffectGraph& graph,
	ID3D12GraphicsCommandList* cmd,
	DxGpuResource* input,
	IRenderTarget* finalTarget,
	CalyxEngine::DxCore* dxCore) const {
	// 全PassがScene入力をSRVとして参照できる状態へ遷移する。
	input->Transition(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// キャッシュと訪問状態は1回の実行内だけ保持し、フレーム間でDescriptorを使い回さない。
	std::unordered_map<int32_t, D3D12_GPU_DESCRIPTOR_HANDLE> cache;
	std::unordered_map<int32_t, bool> visiting;
	int tempIndex = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE currentSRV = input->GetSRVGpuHandle();
	const int32_t sourceNode = graph.FindOutputSourceNode();

	// Output未接続時はScene入力をそのまま最終コピーへ渡す。
	if(sourceNode != 0){
		currentSRV = graph.ExecuteGraphNode(cmd, sourceNode, currentSRV, dxCore, cache, visiting, tempIndex);
	}

	// CopyImageの書込先として使用する直前に最終TargetをRender Target状態へ遷移する。
	finalTarget->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
	graph.postProcessCollection_->GetEffectByName("CopyImage")->Apply(cmd, currentSRV, finalTarget);
}

void LinearPostEffectExecutionState::Execute(
	PostEffectGraph& graph,
	ID3D12GraphicsCommandList* cmd,
	DxGpuResource* input,
	IRenderTarget* finalTarget,
	CalyxEngine::DxCore* dxCore) const {
	// 全PassがScene入力をSRVとして参照できる状態へ遷移する。
	input->Transition(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// 線形Passでは入出力Resourceの競合を避けるため、2枚の一時Targetを交互に使用する。
	auto rt1 = dxCore->GetRenderTargetCollection().Get("PostEffectBuffer1");
	auto rt2 = dxCore->GetRenderTargetCollection().Get("PostEffectBuffer2");

	IRenderTarget* currentOutput = rt1;
	bool useFirstRT = true;
	D3D12_GPU_DESCRIPTOR_HANDLE currentSRV = input->GetSRVGpuHandle();

	for (size_t i = 0; i < graph.passes_.size(); ++i){
		// 現在の一時TargetをPassの書込先として使用可能にする。
		currentOutput->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
		graph.passes_[i]->Apply(cmd, currentSRV, currentOutput);

		// 次のPassが直前の結果をSRVとして読める状態へ戻す。
		currentOutput->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		currentSRV = currentOutput->GetSRV();

		// 次のPassが入力中のTargetへ上書きしないよう出力先を反転する。
		useFirstRT = !useFirstRT;
		currentOutput = useFirstRT ? rt1 : rt2;
	}

	// 最後に生成されたSRVをViewportまたはSwapChain側の最終Targetへコピーする。
	finalTarget->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
	graph.postProcessCollection_->GetEffectByName("CopyImage")->Apply(cmd, currentSRV, finalTarget);
}

D3D12_GPU_DESCRIPTOR_HANDLE PostEffectGraph::ExecuteGraphNode(ID3D12GraphicsCommandList* cmd,
															 int32_t nodeId,
															 D3D12_GPU_DESCRIPTOR_HANDLE sceneSRV,
															 CalyxEngine::DxCore* dxCore,
															 std::unordered_map<int32_t, D3D12_GPU_DESCRIPTOR_HANDLE>& cache,
															 std::unordered_map<int32_t, bool>& visiting,
															 int& tempIndex){
	if(nodeId == 0) return sceneSRV;
	// 分岐先から同じノードが参照されてもGPU Passを重複実行しない。
	if(auto cached = cache.find(nodeId); cached != cache.end()) return cached->second;
	// 循環リンクを検出した場合は再帰を打ち切りScene入力へフォールバックする。
	if(visiting[nodeId]) return sceneSRV;
	visiting[nodeId] = true;

	auto nodeIt = graphNodes_.find(nodeId);
	if(nodeIt == graphNodes_.end()) {
		visiting[nodeId] = false;
		return sceneSRV;
	}

	const GraphNode& node = nodeIt->second;
	if(node.type == "Input"){
		// Inputノードは描画せず、Scene SRVをグラフへ導入する。
		cache[nodeId] = sceneSRV;
		visiting[nodeId] = false;
		return sceneSRV;
	}
	if(!node.pass){
		// 実装を解決できないノードではGPU命令を記録しない。
		visiting[nodeId] = false;
		return sceneSRV;
	}

	const auto& slots = postProcessCollection_->GetSlots();
	const auto slotIt = std::find_if(slots.begin(), slots.end(), [&](const PostEffectSlot& slot) {
		return slot.name == node.type;
	});
	if(slotIt == slots.end() || !slotIt->enabled) {
		// 無効ノードは先頭入力を後続へ渡し、接続構造を維持したまま処理だけを省略する。
		D3D12_GPU_DESCRIPTOR_HANDLE bypass = sceneSRV;
		if(!node.inputPins.empty()) {
			bypass = ExecuteGraphNode(cmd, FindInputSourceNode(node.inputPins[0]), sceneSRV,
								  dxCore, cache, visiting, tempIndex);
		}
		cache[nodeId] = bypass;
		visiting[nodeId] = false;
		return bypass;
	}

	IRenderTarget* output = AcquireTempTarget(dxCore, tempIndex);
	// 一時Target不足時は不正な書込を行わずScene入力を返す。
	if(!output){
		visiting[nodeId] = false;
		return sceneSRV;
	}

	if(node.type == "Blend"){
		if(node.inputPins.empty()){
			cache[nodeId] = sceneSRV;
			visiting[nodeId] = false;
			return sceneSRV;
		}

		std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> inputs;
		inputs.reserve(node.inputPins.size());
		for(const int32_t inputPin : node.inputPins){
			const int32_t sourceNode = FindInputSourceNode(inputPin);
			if(sourceNode == 0) continue;
			inputs.push_back(ExecuteGraphNode(cmd, sourceNode, sceneSRV, dxCore, cache, visiting, tempIndex));
		}

		if(inputs.empty()){
			cache[nodeId] = sceneSRV;
			visiting[nodeId] = false;
			return sceneSRV;
		}

		if(auto* blend = dynamic_cast<BlendEffect*>(node.pass)){
			if(inputs.size() == 1){
				output->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
				node.pass->Apply(cmd, inputs.front(), output);
			}else{
				D3D12_GPU_DESCRIPTOR_HANDLE current = inputs.front();
				for(size_t i = 1; i < inputs.size(); ++i){
					IRenderTarget* blendOutput = (i + 1 == inputs.size()) ? output : AcquireTempTarget(dxCore, tempIndex);
					if(!blendOutput) break;
					blendOutput->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
					blend->Apply(cmd, current, inputs[i], blendOutput);
					blendOutput->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
					current = blendOutput->GetSRV();
				}
			}
		}else{
			output->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
			node.pass->Apply(cmd, inputs.front(), output);
		}
	}else{
		D3D12_GPU_DESCRIPTOR_HANDLE inputSRV = sceneSRV;
		if(!node.inputPins.empty()){
			inputSRV = ExecuteGraphNode(cmd, FindInputSourceNode(node.inputPins[0]), sceneSRV, dxCore, cache, visiting, tempIndex);
		}
		output->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
		node.pass->Apply(cmd, inputSRV, output);
	}

	output->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	const auto result = output->GetSRV();
	cache[nodeId] = result;
	visiting[nodeId] = false;
	return result;
}

IRenderTarget* PostEffectGraph::AcquireTempTarget(CalyxEngine::DxCore* dxCore, int& tempIndex) const{
	static constexpr int kMaxNodeBuffers = 8;
	const int index = (std::min)(tempIndex, kMaxNodeBuffers - 1);
	++tempIndex;
	return dxCore->GetRenderTargetCollection().Get("PostEffectNodeBuffer" + std::to_string(index));
}

int32_t PostEffectGraph::FindInputSourceNode(int32_t inputPinId) const{
	auto it = inputPinToSourceNode_.find(inputPinId);
	return it != inputPinToSourceNode_.end() ? it->second : 0;
}

int32_t PostEffectGraph::FindOutputSourceNode() const{
	auto outIt = graphNodes_.find(outputNodeId_);
	if(outIt == graphNodes_.end() || outIt->second.inputPins.empty()) return 0;
	return FindInputSourceNode(outIt->second.inputPins.front());
}
