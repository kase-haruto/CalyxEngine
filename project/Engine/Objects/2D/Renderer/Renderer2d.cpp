#include "Renderer2D.h"

#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Objects/2D/Object2d/GameObject2d.h>

#include <cassert>

namespace Calyx2D {

    void Renderer2D::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device) {
        device_ = device;
        assert(device_ && "Renderer2D::Initialize device is null");

        CreateQuadMesh_();

        materialCB_.Initialize(device_);
        transformCB_.Initialize(device_);

        initialized_ = true;
    }

    void Renderer2D::CreateQuadMesh_() {
        // Quad: 4 vertices, 6 indices
        quadVB_.Initialize(device_, 4);
        quadIB_.Initialize(device_, 6);

        // VertexData の定義はあなたの既存のものを使う
        // position は「アンカー計算」を Renderer 側でできるように 0..1 の基準にしておく
        auto* v = quadVB_.Data();
        v[0].position = {0.0f, 1.0f, 0.0f, 1.0f}; v[0].texcoord = {0.0f, 1.0f}; // 左下
        v[1].position = {0.0f, 0.0f, 0.0f, 1.0f}; v[1].texcoord = {0.0f, 0.0f}; // 左上
        v[2].position = {1.0f, 1.0f, 0.0f, 1.0f}; v[2].texcoord = {1.0f, 1.0f}; // 右下
        v[3].position = {1.0f, 0.0f, 0.0f, 1.0f}; v[3].texcoord = {1.0f, 0.0f}; // 右上

        auto* i = quadIB_.Data();
        i[0] = 0; i[1] = 1; i[2] = 2;
        i[3] = 1; i[4] = 3; i[5] = 2;
    }

    void Renderer2D::Draw(ID3D12GraphicsCommandList* cmdList,
                          PipelineService* psoService,
                          RenderTargetType renderTarget) {
        if (!initialized_) return;
        if (drawQueue_.empty()) return;

        // RenderTarget でフィルタ
        // （必要なら事前にキューを RenderTarget ごとに分けてもOK）
        bool any = false;
        for (auto& d : drawQueue_) {
            if (d.targetRT == renderTarget) { any = true; break; }
        }
        if (!any) { Clear(); return; }

        // PSO/RootSig 設定（あなたの旧 SpriteRenderer と同様）
        auto desc = PipelinePresets::MakeObject2D();
        psoService->SetCommand(desc, cmdList);

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 共通メッシュを IA にセット
        quadVB_.SetCommand(cmdList);
        quadIB_.SetCommand(cmdList);

    	for (const auto& d : drawQueue_) {
    		if (d.targetRT != renderTarget) continue;

    		cmdList->SetGraphicsRootDescriptorTable(2, d.texture);

    		materialCB_.TransferData(d.material);
    		materialCB_.SetCommand(cmdList, 0);

    		TransformationMatrix tm{};
    		tm.world = d.wvp;
    		transformCB_.TransferData(tm);
    		transformCB_.SetCommand(cmdList, 1);

    		cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
    	}

        Clear();
    }

    void Renderer2D::Clear() {
        drawQueue_.clear();
    }

	void Renderer2D::Collect(const std::vector<GameObject2D*>& objects) {
    	drawQueue_.clear();
    	drawQueue_.reserve(objects.size());

    	for (auto* obj : objects) {
    		if (!obj) continue;

    		auto* sp = obj->GetComponent<SpriteComponent>();
    		if (!sp || !sp->visible || !sp->asset) continue;

    		SpriteDrawData d{};
    		d.texture = sp->asset->GetHandle();
    		d.wvp     = obj->transform.GetMatrix();
    		d.material.color      = sp->color;
    		d.material.fillAmount = sp->fillAmount;
    		d.targetRT = sp->targetRT;

    		drawQueue_.push_back(d);
    	}
    }

} // namespace Calyx2D
