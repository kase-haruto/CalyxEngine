#include "FogEffect.h"

// engine
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>

// external
#ifdef _DEBUG
#include<externals/imgui/imgui.h>
#endif // _DEBUG


FogEffect::~FogEffect(){ 
}

void FogEffect::ShowImGuiInterface(){
#ifdef _DEBUG
	ImGui::Begin("fogEffect");
	ImGui::SliderFloat("fogStart", &parameters->fogStart, 100.0f, 50.0f);
	ImGui::SliderFloat("fogEnd", &parameters->fogEnd, 500.0f, 1000.0f);
	ImGui::End();
#endif // _DEBUG
}

FogEffect::FogEffect(const DxCore* dxCore):pDxCore_(dxCore){
	//定数バッファの生成
	CreateConstantBuffer();

	//定数バッファをマップ
	constantBuffer->Map(0, nullptr, reinterpret_cast< void** >(&parameters));

	///================================
	///	霧のパラメータを設定
	///================================
	//霧のスタート地点
	parameters->fogStart = 5.0f;
	//霧の終点
	parameters->fogEnd = 90.0f;
	//霧の色
	parameters->fogColor = XMFLOAT4(0.02f, 0.02f, 0.02f, 1.0f); // 白色の設定

	constantBuffer->Unmap(0, nullptr);
	
}

void FogEffect::CreateConstantBuffer(){
	//定数バッファの作成
	bufferSize = sizeof(FogParameters);

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = bufferSize; // 定数バッファのサイズを設定
	desc.Height = 1; // バッファの高さは1
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN; // 定数バッファなのでフォーマットは不明
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // 定数バッファなので行主要レイアウト
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	ComPtr<ID3D12Device> device = pDxCore_->GetDevice();

	// リソースの作成
	device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // 一般的な読み取り状態で初期化
		nullptr,
		IID_PPV_ARGS(&constantBuffer));
}

void FogEffect::Update(){
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>commandList = pDxCore_->GetCommandList();
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = GraphicsGroup::GetInstance()->GetRootSignature(Object3D);
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	//フォグ用のCBufferの設定
	commandList->SetGraphicsRootConstantBufferView(2, constantBuffer->GetGPUVirtualAddress());
}

