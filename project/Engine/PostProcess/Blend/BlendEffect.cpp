#include "BlendEffect.h"

#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <externals/imgui/imgui.h>

#include <algorithm>

void BlendEffect::Initialize(const PipelineSet& psoSet) {
	// PSOとRoot SignatureはPipeline管理層が所有するため、非所有ハンドルだけを保持する。
	psoSet_ = psoSet;

	// EditorとRuntimeで共有する合成パラメータをGPUへ転送する定数バッファを確保する。
	buffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());

	// 新規生成時と再初期化時の表示結果を一致させるため既定値へ戻す。
	ResetParameters();
}

void BlendEffect::Apply(ID3D12GraphicsCommandList* cmd,
						D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
						IRenderTarget* outputRT) {
	// 単一入力経路では同じSRVを両入力へ割り当て、2入力シェーダーの契約を満たす。
	Apply(cmd, inputSRV, inputSRV, outputRT);
}

void BlendEffect::Apply(ID3D12GraphicsCommandList* cmd,
						D3D12_GPU_DESCRIPTOR_HANDLE inputA,
						D3D12_GPU_DESCRIPTOR_HANDLE inputB,
						IRenderTarget* outputRT) {
	// Blend結果を書き込めるよう、出力ResourceをRender Target状態へ遷移する。
	outputRT->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
	outputRT->SetRenderTarget(cmd);

	// GUIまたはAnimationで変更された最新パラメータを描画直前にGPUへ転送する。
	buffer_.TransferData(param_);

	// Blend専用Pipelineと2枚の入力SRV、定数バッファをRoot Signatureへ設定する。
	psoSet_.SetCommand(cmd);
	cmd->SetGraphicsRootDescriptorTable(0, inputA);
	cmd->SetGraphicsRootDescriptorTable(1, inputB);
	buffer_.SetCommand(cmd, 2);

	// Fullscreen Triangleを1回描画して出力Target全体を合成する。
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);
}

void BlendEffect::ShowImGui() {
	if(ImGui::CollapsingHeader("Blend")) {
		// Shaderで定義された値域だけをEditorから設定できるよう制限する。
		ImGui::SliderFloat("Opacity", &param_.opacity, 0.0f, 1.0f);
		int mode = static_cast<int>(param_.mode);
		if(ImGui::Combo("Mode", &mode, "Lerp\0Add\0Multiply\0Max\0")) {
			param_.mode = static_cast<float>(mode);
		}
		if(ImGui::Button("Reset")) ResetParameters();
	}
}

void BlendEffect::ResetParameters() {
	param_.opacity = 0.5f;
	param_.mode = 0.0f;
}

nlohmann::json BlendEffect::SaveParameters() const {
	return nlohmann::json{{"opacity", param_.opacity}, {"mode", static_cast<int>(param_.mode)}};
}

void BlendEffect::LoadParameters(const nlohmann::json& params) {
	// 外部編集されたJSONでもShader想定範囲を越えないよう読込時にClampする。
	if(params.contains("opacity") && params["opacity"].is_number()) {
		param_.opacity = std::clamp(params["opacity"].get<float>(), 0.0f, 1.0f);
	}
	if(params.contains("mode") && params["mode"].is_number_integer()) {
		param_.mode = static_cast<float>(std::clamp(params["mode"].get<int>(), 0, 3));
	}
}

bool BlendEffect::GetFloatParameter(const std::string& name, float& out) const {
	if(name == "opacity") {
		out = param_.opacity;
		return true;
	}
	if(name == "mode") {
		out = param_.mode;
		return true;
	}
	return false;
}

bool BlendEffect::SetFloatParameter(const std::string& name, float value) {
	if(name == "opacity") {
		param_.opacity = std::clamp(value, 0.0f, 1.0f);
		return true;
	}
	if(name == "mode") {
		param_.mode = static_cast<float>(std::clamp(static_cast<int>(value), 0, 3));
		return true;
	}
	return false;
}
