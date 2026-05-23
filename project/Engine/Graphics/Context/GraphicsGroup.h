#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Graphics/Pipeline/Manager/PipelineStateManager.h>

class GraphicsGroup {
private:
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	static GraphicsGroup* GetInstance();
	~GraphicsGroup() = default;

	void Initialize(const CalyxEngine::DxCore* dxCore, PipelineStateManager* psManager);

	///=========================================
	/// pipelineの取得
	///=========================================
	PipelineStateManager*			   GetPipelineState() const;
	const ComPtr<ID3D12RootSignature>& GetRootSignature(const PipelineType& type, const BlendMode& blendMode = BlendMode::NORMAL) const;
	const ComPtr<ID3D12PipelineState>& GetPipelineState(const PipelineType& type, const BlendMode& blendMode = BlendMode::NORMAL) const;

	///=========================================
	/// device/commandListの取得
	///=========================================
	ComPtr<ID3D12Device>			  GetDevice() const;
	ID3D12Device5*					  GetDevice5() const;
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const;
	uint32_t						  GetClientWidth() const;
	uint32_t						  GetClientHeight() const;

	void SetCommand(ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType psoType, BlendMode blendMode);

private:
	GraphicsGroup() :pDxCore_(nullptr), pipelineManager_(nullptr) {}
	GraphicsGroup(const GraphicsGroup&)			   = delete;
	GraphicsGroup& operator=(const GraphicsGroup*) = delete;

private:
	const CalyxEngine::DxCore*		 pDxCore_;
	PipelineStateManager* pipelineManager_;
};