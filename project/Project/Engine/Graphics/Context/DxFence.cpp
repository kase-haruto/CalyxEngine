#include <Engine/Graphics/Context/DxFence.h>

DxFence::~DxFence(){
	if (fenceEvent_ != nullptr){
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}
}

void DxFence::Initialize(ComPtr<ID3D12Device> device){
	HRESULT hr;
	//初期値0でFenceを作る
	fenceValue_ = 0;
	hr = device->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	//Fenceのsignalを待つためのイベントを作成する
	fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent_ != nullptr);
}

void DxFence::Signal(ComPtr<ID3D12CommandQueue> commandQueue){
	//Fenceの値を更新
	fenceValue_++;
	//GPUがここまでたどり着いたときに,Fenceの値を指定した値に代入するようにSignalを送る
	commandQueue->Signal(fence_.Get(), fenceValue_);
}

void DxFence::Wait(){
	//fenceの値が指定したsignalの値にたどり着いてるか確認する
	//GetCompletedValueの初期値はfence作成時に渡した初期値
	if (fence_->GetCompletedValue() < fenceValue_){
		//指定したsignalにたどり着いていないので、たどり着くまで待つようにイベントを指定する
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		//イベントを待つ
		WaitForSingleObject(fenceEvent_, INFINITE);
	}
}
