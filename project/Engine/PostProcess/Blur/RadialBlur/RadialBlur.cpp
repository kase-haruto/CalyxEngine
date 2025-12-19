#include "RadialBlur.h"
#include <Engine/PostProcess/FullscreenDrawer.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <algorithm>

void RadialBlurEffect::Initialize(const PipelineSet& psoSet) {
    psoSet_ = psoSet;
    blurBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());
    ResetParameters();
}

void RadialBlurEffect::Apply(ID3D12GraphicsCommandList* cmd,
                             D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
                             IRenderTarget* outputRT) {
    // 出力RTへ
    outputRT->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
    outputRT->SetRenderTarget(cmd);

    // 定数バッファ更新
    blurBuffer_.TransferData(blurParam_);

    // PSO設定
    psoSet_.SetCommand(cmd);
    // RootParameter 0: SRV (input)
    cmd->SetGraphicsRootDescriptorTable(0, inputSRV);
    // RootParameter 1: CB (blurParam)
    blurBuffer_.SetCommand(cmd, 1);

    // フルスクリーン三角形描画
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->DrawInstanced(3, 1, 0, 0);
}

// ---------------- Runtime API ----------------
void RadialBlurEffect::SetWidth(float width) {
    // 負値ガード + 上限クランプ
    blurParam_.width = std::clamp(width, kMinWidth, kMaxWidth);
}

void RadialBlurEffect::SetCenter(const CalyxMath::Vector2& uv) {
    // 画面正規座標 [0,1] にクランプ
    blurParam_.center.x = std::clamp(uv.x, 0.0f, 1.0f);
    blurParam_.center.y = std::clamp(uv.y, 0.0f, 1.0f);
}

// ---------------- Editor UI / Reset ----------------
void RadialBlurEffect::ShowImGui() {
    if (ImGui::CollapsingHeader("RadialBlur")) {
        ImGui::TextUnformatted("Center (UV)");
        ImGui::SliderFloat("U", &blurParam_.center.x, 0.0f, 1.0f);
        ImGui::SliderFloat("V", &blurParam_.center.y, 0.0f, 1.0f);

        ImGui::TextUnformatted("Width (Strength)");
        ImGui::SliderFloat("Width", &blurParam_.width, kMinWidth, kMaxWidth);

        if (ImGui::Button("Reset")) {
            ResetParameters();
        }
    }
}

void RadialBlurEffect::ResetParameters() {
    blurParam_.center = { 0.5f, 0.5f };
    blurParam_.width  = 0.0f;
}