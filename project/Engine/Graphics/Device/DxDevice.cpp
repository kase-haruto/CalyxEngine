#include "DxDevice.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Foundation/Utility/Converter/ConvertString.h>

// lib
#include <Engine/Foundation/Utility/Func/MyFunc.h>

/* c++ */
#include <dxgidebug.h>
#include <format>

namespace CalyxGraphics {
	DxDevice::~DxDevice() =default;

	/////////////////////////////////////////////////////////////////////////////////
	//  初期化処理
	/////////////////////////////////////////////////////////////////////////////////
	void DxDevice::Initialize() {
		SetupDebugLayer();
		CreateDXGIDevice();
		QueryCapabilities();
	}

	///////////////////////////////////////////////////////////////////////////////
	//  インラインレイトレーシング対応確認
	///////////////////////////////////////////////////////////////////////////////
	bool DxDevice::IsInlineRaytracingSupported() const {
		return (caps_.raytracingTier >= D3D12_RAYTRACING_TIER_1_1) && caps_.shaderModel6_5;
	}
	
	/////////////////////////////////////////////////////////////////////////////////
	//  デバッグレイヤーのセットアップ
	/////////////////////////////////////////////////////////////////////////////////
	void DxDevice::SetupDebugLayer() {
#if defined(_DEBUG)
		if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_)))) {
			debugController_->EnableDebugLayer();
			debugController_->SetEnableGPUBasedValidation(TRUE);
		}
#endif
	}

	//////////////////////////////////////////////////////////////////////////////
	//  DXGIデバイスの生成
	//////////////////////////////////////////////////////////////////////////////
	void DxDevice::CreateDXGIDevice() {
		HRESULT hr;
		// dxgiファクトリーの生成
		hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
		assert(SUCCEEDED(hr));
		// よい順にアダプタをたのむ
		for(UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
																 DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter_)) !=
						DXGI_ERROR_NOT_FOUND;
			++i) {
			// アダプターの情報を取得する
			DXGI_ADAPTER_DESC3 adapterDesc{};
			hr = adapter_->GetDesc3(&adapterDesc);
			assert(SUCCEEDED(hr));
			// ソフトウェアアダプタでなければ採用
			if(!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
				// 採用したアダプタの情報をログに出力。wstringのほうなので注意
				Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
				break;
			}
			// ソフトウェアアダプタの場合は見なかったことにする
			adapter_ = nullptr;
		}

		// 機能レベルとログ出力用の文字列
		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0};
		const char* featureLevelStrings[] = {"12.2", "12.1", "12.0"};
		// 高い順に生成できるか試していく
		for(size_t i = 0; i < _countof(featureLevels); ++i) {
			// 採用したアダプタでデバイスを生成
			hr = D3D12CreateDevice(adapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
			// 指定した機能レベルでデバイスが生成できたかを確認
			if(SUCCEEDED(hr)) {
				// 生成できたのでログ出力を行ってループを抜ける
				Log(std::format("FeatureLevel:{}\n", featureLevelStrings[i]));
				break;
			}
		}
		// デバイスの生成がうまくいかなかったので起動できない
		assert(device_ != nullptr);
		Log("Complete create D3D12Device!!!\n"); // 初期化完了のログを出す

#if defined(_DEBUG)

		ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
		if(SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
			// やばいエラー時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			// エラー時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			// 警告時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

			// 抑制するメッセージのid
			D3D12_MESSAGE_ID denyIds[] = {
				// windows11のdxgiデバッグレイヤーとdx12デバッグレイヤーの相互作用バグによるエラーメッセージ
				D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};

			// 抑制するレベル
			D3D12_MESSAGE_SEVERITY	severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
			D3D12_INFO_QUEUE_FILTER filter{};
			filter.DenyList.NumIDs		  = _countof(denyIds);
			filter.DenyList.pIDList		  = denyIds;
			filter.DenyList.NumSeverities = _countof(severities);
			filter.DenyList.pSeverityList = severities;
			// 指定s多メッセージの表示を抑制する
			infoQueue->PushStorageFilter(&filter);
		}
#endif // _DEBUG
	}

	///////////////////////////////////////////////////////////////////////////////
	//  レイトレーシング機能確認
	///////////////////////////////////////////////////////////////////////////////
	void DxDevice::QueryCapabilities() {
		assert(device_ && "Device must be created before QueryCapabilities().");

		// ---- Device5 取得（DXRで必須）----
		HRESULT hr = device_->QueryInterface(IID_PPV_ARGS(&device5_));
		assert(SUCCEEDED(hr) && device5_ && "Failed to get ID3D12Device5 (DXR requires it).");

		// ---- DXR Tier（Options5）----
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 opt5{};
		hr = device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &opt5, sizeof(opt5));
		assert(SUCCEEDED(hr) && "CheckFeatureSupport OPTIONS5 failed.");
		caps_.raytracingTier = opt5.RaytracingTier;

		// ---- Shader Model 6.5（RayQueryに必要）----
		D3D12_FEATURE_DATA_SHADER_MODEL sm{};
		sm.HighestShaderModel = D3D_SHADER_MODEL_6_5;
		hr					  = device_->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm));
		caps_.shaderModel6_5  = SUCCEEDED(hr) && (sm.HighestShaderModel >= D3D_SHADER_MODEL_6_5);

		// ログ
		Log(std::format("RaytracingTier:{}\n", static_cast<int>(caps_.raytracingTier)));
		Log(std::format("ShaderModel6_5:{}\n", caps_.shaderModel6_5 ? "true" : "false"));
	}
} // namespace CalyxGraphics