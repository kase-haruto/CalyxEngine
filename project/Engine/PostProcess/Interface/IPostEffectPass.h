#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
//engine
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>

//c++
#include <d3d12.h>
#include <wrl.h>
#include <externals/nlohmann/json.hpp>
#include <string>

/**
 * @brief IPostEffectPassの機能を提供するクラスです。
 */
class IPostEffectPass{
public:
	virtual ~IPostEffectPass() = default;

	virtual void Apply(ID3D12GraphicsCommandList* cmd,
					   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
					   IRenderTarget* outputRT) = 0;

	virtual void ShowImGui() {}

	// 既定値へ戻す
	virtual void ResetParameters() {}

	// 更新
	virtual void Tick(float /*dt*/) {}

	virtual const std::string GetName() const = 0;

	virtual nlohmann::json SaveParameters() const { return nlohmann::json::object(); }
	virtual void LoadParameters(const nlohmann::json& /*params*/) {}

	virtual bool GetFloatParameter(const std::string& /*name*/, float& /*out*/) const { return false; }
	virtual bool SetFloatParameter(const std::string& /*name*/, float /*value*/) { return false; }
};
