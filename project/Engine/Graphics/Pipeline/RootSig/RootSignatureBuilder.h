#pragma once
/* ======================================================================== */
/* 	include space                                                           */
/* ===================================================================== */

#include <d3d12.h>
#include <vector>
#include <wrl.h>

/* ======================================================================== */
/* 	class RootSignatureBuilder                                              */
/* ===================================================================== */
class RootSignatureBuilder {
public:
	//===================================================================*/
	//		parmSet
	//===================================================================*/
	RootSignatureBuilder& Constants(UINT reg, UINT num32BitValues, D3D12_SHADER_VISIBILITY vis);
	RootSignatureBuilder& CBV(UINT reg, D3D12_SHADER_VISIBILITY vis);
	RootSignatureBuilder& SRV(UINT reg, D3D12_SHADER_VISIBILITY vis);
	RootSignatureBuilder& UAV(UINT reg, D3D12_SHADER_VISIBILITY vis);
	RootSignatureBuilder& AllowDXR();

	RootSignatureBuilder& UAVTable(UINT shaderRegister, UINT count,
								   D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

	//===================================================================*/
	//		samplerSet
	//===================================================================*/
	RootSignatureBuilder& StaticSampler(const D3D12_STATIC_SAMPLER_DESC& s);
	RootSignatureBuilder& SampleClampLinear(UINT reg, D3D12_SHADER_VISIBILITY vis = D3D12_SHADER_VISIBILITY_PIXEL);
	RootSignatureBuilder& SamplerWrapLinear(UINT reg, D3D12_SHADER_VISIBILITY vis = D3D12_SHADER_VISIBILITY_PIXEL);
	RootSignatureBuilder& SamplerWrapAniso(UINT reg, UINT maxAniso = 16, D3D12_SHADER_VISIBILITY vis = D3D12_SHADER_VISIBILITY_PIXEL);

	//===================================================================*/
	//		flags
	//===================================================================*/
	RootSignatureBuilder& AllowIA();
	RootSignatureBuilder& DenyVS();
	RootSignatureBuilder& DenyPS();

	//===================================================================*/
	//		DescriptorTable
	//===================================================================*/
	RootSignatureBuilder& SRVTable(UINT shaderRegister, UINT count,
								   D3D12_DESCRIPTOR_RANGE_TYPE type,
								   D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

	//===================================================================*/
	//		build
	//===================================================================*/
	D3D12_ROOT_SIGNATURE_DESC Desc() const;

	//===================================================================*/
	//		hash
	//===================================================================*/
	size_t Hash() const;

private:
	//===================================================================*/
	//		inner util
	//===================================================================*/
	static D3D12_ROOT_PARAMETER RootParm(D3D12_ROOT_PARAMETER_TYPE type, D3D12_SHADER_VISIBILITY vis, UINT reg);
	static D3D12_STATIC_SAMPLER_DESC MakeSampler(UINT reg, D3D12_SHADER_VISIBILITY vis, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrMode);
	void AddParm(D3D12_ROOT_PARAMETER param);

private:
	//===================================================================*/
	//		desc table
	//===================================================================*/
	struct DescriptorTableEntry {
		std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
		D3D12_ROOT_PARAMETER param;
	};
	std::vector<DescriptorTableEntry> descriptorTables_; //< 各テーブルの生存管理

	//===================================================================*/
	//		root signature
	//===================================================================*/
	std::vector<D3D12_ROOT_PARAMETER> params_;
	std::vector<D3D12_STATIC_SAMPLER_DESC> samplers_;
	D3D12_ROOT_SIGNATURE_FLAGS flags_{};
};