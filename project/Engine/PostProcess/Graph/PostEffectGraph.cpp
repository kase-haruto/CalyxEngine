#include "PostEffectGraph.h"
#include <Engine/PostProcess/Collection/PostProcessCollection.h>
#include <Engine/PostProcess/Blend/BlendEffect.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Foundation/Debug/CxAssert.h>
#include <algorithm>
#include <vector>

void PostEffectGraph::SetPassesFromList(const std::vector<PostEffectSlot>& slots){
	passes_.clear();
	useNodeGraph_ = false;
	graphNodes_.clear();
	pinOwner_.clear();
	inputPinToSourceNode_.clear();
	outputNodeId_ = 0;
	int enabledCount = 0;

	for (const auto& slot : slots){
		if (slot.enabled && slot.pass){
			++enabledCount;
			passes_.push_back(slot.pass);
		}
	}

	/*if (enabledCount > 1){
		CX_CHECK(enabledCount <= 2 && "PostEffectGraph currently supports only up to 2 enabled passes for ping-pong rendering.", "Assertion failed");
	}*/
}

void PostEffectGraph::SetGraphFromJson(const nlohmann::json& root, const std::vector<PostEffectSlot>& slots){
	SetPassesFromList(slots);
	if(!root.contains("graph") || !root["graph"].is_object()) return;

	const auto& graph = root["graph"];
	if(!graph.contains("nodes") || !graph["nodes"].is_array()) return;

	graphNodes_.clear();
	pinOwner_.clear();
	inputPinToSourceNode_.clear();
	outputNodeId_ = 0;

	auto findPass = [&](const std::string& type) -> IPostEffectPass* {
		for(const auto& slot : slots){
			// Disabled/Triggered nodes must remain in the graph. Their enabled state is
			// evaluated at execution time so a trigger can activate them later.
			if(slot.name == type && slot.pass) return slot.pass;
		}
		return nullptr;
	};

	for(const auto& nodeJson : graph["nodes"]){
		GraphNode node;
		node.id = nodeJson.value("id", 0);
		node.type = nodeJson.value("type", "");
		if(node.id == 0 || node.type.empty()) continue;
		node.pass = findPass(node.type);
		if(node.type == "Input" || node.type == "Output" || node.pass){
			if(node.type == "Output") outputNodeId_ = node.id;
			if(nodeJson.contains("inputs") && nodeJson["inputs"].is_array()){
				for(const auto& pin : nodeJson["inputs"]){
					const int32_t pinId = pin.value("id", 0);
					if(pinId == 0) continue;
					node.inputPins.push_back(pinId);
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
			if(toPin == 0 || fromOwner == pinOwner_.end()) continue;
			inputPinToSourceNode_[toPin] = fromOwner->second;
		}
	}

	useNodeGraph_ = outputNodeId_ != 0;
}

void PostEffectGraph::Execute(ID3D12GraphicsCommandList* cmd,
							  DxGpuResource* input,
							  IRenderTarget* finalTarget,
							  CalyxEngine::DxCore* dxCore){




	input->Transition(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	if(useNodeGraph_){
		std::unordered_map<int32_t, D3D12_GPU_DESCRIPTOR_HANDLE> cache;
		std::unordered_map<int32_t, bool> visiting;
		int tempIndex = 0;
		D3D12_GPU_DESCRIPTOR_HANDLE currentSRV = input->GetSRVGpuHandle();
		const int32_t sourceNode = FindOutputSourceNode();
		if(sourceNode != 0){
			currentSRV = ExecuteGraphNode(cmd, sourceNode, currentSRV, dxCore, cache, visiting, tempIndex);
		}
		finalTarget->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
		postProcessCollection_->GetEffectByName("CopyImage")->Apply(cmd, currentSRV, finalTarget);
		return;
	}

	// Ping-Pongバッファ
	auto rt1 = dxCore->GetRenderTargetCollection().Get("PostEffectBuffer1");
	auto rt2 = dxCore->GetRenderTargetCollection().Get("PostEffectBuffer2");

	IRenderTarget* currentOutput = rt1;
	bool useFirstRT = true;
	D3D12_GPU_DESCRIPTOR_HANDLE currentSRV = input->GetSRVGpuHandle();

	for (size_t i = 0; i < passes_.size(); ++i){
		currentOutput->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
		passes_[i]->Apply(cmd, currentSRV, currentOutput);
		currentOutput->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		currentSRV = currentOutput->GetSRV();

		// Ping-Pong切り替え
		useFirstRT = !useFirstRT;
		currentOutput = useFirstRT ? rt1 : rt2;
	}

	// 最終出力にPostProcessCollectionのCopyImageを使用してコピー
	finalTarget->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
	postProcessCollection_->GetEffectByName("CopyImage")->Apply(cmd, currentSRV, finalTarget);
}

D3D12_GPU_DESCRIPTOR_HANDLE PostEffectGraph::ExecuteGraphNode(ID3D12GraphicsCommandList* cmd,
															 int32_t nodeId,
															 D3D12_GPU_DESCRIPTOR_HANDLE sceneSRV,
															 CalyxEngine::DxCore* dxCore,
															 std::unordered_map<int32_t, D3D12_GPU_DESCRIPTOR_HANDLE>& cache,
															 std::unordered_map<int32_t, bool>& visiting,
															 int& tempIndex){
	if(nodeId == 0) return sceneSRV;
	if(auto cached = cache.find(nodeId); cached != cache.end()) return cached->second;
	if(visiting[nodeId]) return sceneSRV;
	visiting[nodeId] = true;

	auto nodeIt = graphNodes_.find(nodeId);
	if(nodeIt == graphNodes_.end()) {
		visiting[nodeId] = false;
		return sceneSRV;
	}

	const GraphNode& node = nodeIt->second;
	if(node.type == "Input"){
		cache[nodeId] = sceneSRV;
		visiting[nodeId] = false;
		return sceneSRV;
	}
	if(!node.pass){
		visiting[nodeId] = false;
		return sceneSRV;
	}

	const auto& slots = postProcessCollection_->GetSlots();
	const auto slotIt = std::find_if(slots.begin(), slots.end(), [&](const PostEffectSlot& slot) {
		return slot.name == node.type;
	});
	if(slotIt == slots.end() || !slotIt->enabled) {
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
