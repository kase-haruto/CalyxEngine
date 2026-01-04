#include "DxCommand.h"

void DxCommand::Initialize(const ComPtr<ID3D12Device>& device){
	HRESULT hr;
	//コマンドキューを生成
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc {};
	hr = device->CreateCommandQueue(&commandQueueDesc,
									IID_PPV_ARGS(&commandQueue_));
	//コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドアロケータを生成する
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	//コマンドアロケータを生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドリストを生成する
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr,
								   IID_PPV_ARGS(&commandList_));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));
}

void DxCommand::Reset(){
	//次のフレーム用のコマンドリストを準備
	HRESULT hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));
}
