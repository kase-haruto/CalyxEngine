#include "DxCommand.h"
#include <Engine/Foundation/Debug/CxAssert.h>

void DxCommand::Initialize(const ComPtr<ID3D12Device>& device){
	HRESULT hr;
	//コマンドキューを生成
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc {};
	hr = device->CreateCommandQueue(&commandQueueDesc,
									IID_PPV_ARGS(&commandQueue_));
	//コマンドキューの生成がうまくいかなかったので起動できない
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	//コマンドアロケータを生成する
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	//コマンドアロケータを生成がうまくいかなかったので起動できない
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	//コマンドリストを生成する
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr,
								   IID_PPV_ARGS(&commandList_));
	//コマンドリストの生成がうまくいかなかったので起動できない
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");
}

void DxCommand::Reset(){
	// コマンドリストが開いていたら閉じる（エラーは無視してよい）

	//次のフレーム用のコマンドリストを準備
	HRESULT hr = commandAllocator_->Reset();
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");
}
