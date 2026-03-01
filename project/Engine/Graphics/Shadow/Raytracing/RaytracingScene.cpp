#include "RaytracingScene.h"

#include "Engine/Foundation/Math/Matrix3x4.h"

namespace CalyxGraphics {

	/////////////////////////////////////////////////////////////////////////////////
	//  インスタンスのクリア
	/////////////////////////////////////////////////////////////////////////////////
	void RaytracingScene::Clear() {
		instances_.clear();
	}

	///////////////////////////////////////////////////////////////////////////////
	//  インスタンスの追加
	///////////////////////////////////////////////////////////////////////////////
	void RaytracingScene::AddInstance(const CalyxMath::Matrix3x4&	  transform,
									  D3D12_GPU_VIRTUAL_ADDRESS		  blasAddress,
									  uint32_t						  instanceID,
									  uint8_t						  mask,
									  D3D12_RAYTRACING_INSTANCE_FLAGS flags) {
		assert(blasAddress != 0);

		// インスタンス記述子の作成
		D3D12_RAYTRACING_INSTANCE_DESC desc{};
		std::memcpy(desc.Transform, transform.m, sizeof(float) * 12);

		// インスタンス情報の設定
		desc.InstanceID							 = instanceID;
		desc.InstanceMask						 = mask;
		desc.InstanceContributionToHitGroupIndex = 0;
		desc.Flags								 = flags;
		desc.AccelerationStructure				 = blasAddress;

		// インスタンスを追加
		instances_.push_back(desc);
	}

	/////////////////////////////////////////////////////////////////////////////////
	//  バッファの確保
	/////////////////////////////////////////////////////////////////////////////////
	void RaytracingScene::EnsureBuffer(ID3D12Device* device) {
		assert(device);

		// インスタンス数を取得
		const UINT count = static_cast<UINT>(instances_.size());
		if(count == 0) return;

		// バッファが足りないならリサイズ
		if(!instanceBuffer_.IsValid() ||
		   instanceBuffer_.GetElementCount() < count) {
			UINT newCount = instanceBuffer_.GetElementCount() ? instanceBuffer_.GetElementCount() * 2 : 128u;
			newCount	  = (std::max)(newCount, count);
			instanceBuffer_.Resize(device, newCount);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	//  バッファのアップロード
	/////////////////////////////////////////////////////////////////////////////////
	void RaytracingScene::Upload() {
		if(instances_.empty()) return;
		// バッファが有効であることを確認
		assert(instanceBuffer_.IsValid());
		// インスタンスデータをコピー
		std::memcpy(
			instanceBuffer_.Data(),
			instances_.data(),
			sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instances_.size());
	}

	///////////////////////////////////////////////////////////////////////////////
	//  インスタンスの追加
	///////////////////////////////////////////////////////////////////////////////
	uint32_t RaytracingScene::GetInstanceCount() const {
		// インスタンス数を返す
		return static_cast<uint32_t>(instances_.size());
	}

	/////////////////////////////////////////////////////////////////////////////////
	//  インスタンス記述子バッファのGPU仮想アドレス取得
	/////////////////////////////////////////////////////////////////////////////////
	D3D12_GPU_VIRTUAL_ADDRESS RaytracingScene::GetInstanceDescGPUVA() const {
		// バッファが有効ならGPU仮想アドレスを返す
		return instanceBuffer_.IsValid()
				   ? instanceBuffer_.GetResource()->GetGPUVirtualAddress()
				   : 0;
	}

} // namespace CalyxGraphics
