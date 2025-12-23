#pragma once

#include <vector>

#include <Engine/Graphics/Buffer/DxVertexBuffer.h>
#include <Engine/Graphics/Buffer/DxIndexBuffer.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>

#include <Engine/Renderer/Mesh/VertexData.h>
#include <Engine/Graphics/Material.h>

#include "Engine/Objects/2D/Sprite/Component/SpriteComponent.h"
#include "Engine/Objects/Transform/Transform.h"
#include "SpriteDrawData.h"

class PipelineService;

namespace Calyx2D {
	class GameObject2D;

	class Renderer2D {
    public:
        void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device);

        void Draw(ID3D12GraphicsCommandList* cmdList,
                  PipelineService* psoService,
                  RenderTargetType renderTarget);

        void Clear();
    	void Collect(const std::vector<GameObject2D*>& objects);

	private:
        void CreateQuadMesh_();
    private:
        // 共通 Quad（全 Sprite で共有）
        DxVertexBuffer<VertexData> quadVB_;
        DxIndexBuffer<uint32_t>    quadIB_;

        // CB（Drawごとに更新）
        DxConstantBuffer<Material2D>           materialCB_;
        DxConstantBuffer<TransformationMatrix> transformCB_;

        std::vector<SpriteDrawData> drawQueue_;

        Microsoft::WRL::ComPtr<ID3D12Device> device_;
        bool initialized_ = false;
    };

} // namespace Calyx2D
