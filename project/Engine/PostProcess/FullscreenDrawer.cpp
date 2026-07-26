#include "FullscreenDrawer.h"


#include <DirectXMath.h>
#include <wrl.h>
#include <d3dx12.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace{
	struct Vertex{
		XMFLOAT3 position;
		XMFLOAT2 texcoord;
	};

	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView;
}

bool FullscreenDrawer::initialized_ = false;

void FullscreenDrawer::Initialize(ID3D12Device* device){
	// 全PostEffectで共有する不変頂点Bufferを一度だけ生成する。
	if (initialized_) return;
	CreateVertexBuffer(device);
	initialized_ = true;
}

void FullscreenDrawer::CreateVertexBuffer(ID3D12Device* device){
	// 画面を覆うOversized Triangleを使い、Quad中央の継ぎ目と余分な頂点を避ける。
	Vertex vertices[] = {
		{{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
		{{-1.0f,  3.0f, 0.0f}, {0.0f, -1.0f}},
		{{ 3.0f, -1.0f, 0.0f}, {2.0f, 1.0f}},
	};

	const UINT sizeInBytes = sizeof(vertices);

	// 初期化後に変更しない小規模データのため、直接Map可能なUpload Heapへ配置する。
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);

	device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer));

	// CPU配列をGPU可視Memoryへ一度転送し、描画時の再Uploadを不要にする。
	void* mapped = nullptr;
	vertexBuffer->Map(0, nullptr, &mapped);
	memcpy(mapped, vertices, sizeInBytes);
	vertexBuffer->Unmap(0, nullptr);

	// CommandListから参照できるVertex Buffer Viewを共有状態として保持する。
	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeInBytes;
	vbView.StrideInBytes = sizeof(Vertex);
}

void FullscreenDrawer::Draw(ID3D12GraphicsCommandList* cmd){
	if (!initialized_) return;

	// PipelineとRenderTargetは呼び出し側が設定し、ここではFullscreen Draw命令だけを記録する。
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->IASetVertexBuffers(0, 1, &vbView);
	cmd->DrawInstanced(3, 1, 0, 0);
}
