#include "RootSignatureBuilder.h"
/* ========================================================================
/* 	include space
/* ===================================================================== */

// c++
#include <functional>
#include <cstddef>

/////////////////////////////////////////////////////////////////////////////////////////
//		各viewのparamterを作成
/////////////////////////////////////////////////////////////////////////////////////////
RootSignatureBuilder& RootSignatureBuilder::Constants(UINT reg, UINT num32BitValues, D3D12_SHADER_VISIBILITY vis) {
	D3D12_ROOT_PARAMETER p{};
	p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	p.ShaderVisibility = vis;
	p.Constants.ShaderRegister = reg;
	p.Constants.RegisterSpace = 0;
	p.Constants.Num32BitValues = num32BitValues;
	params_.push_back(p);
	return *this;
}
RootSignatureBuilder& RootSignatureBuilder::CBV(UINT reg, D3D12_SHADER_VISIBILITY vis) {
	AddParm(RootParm(D3D12_ROOT_PARAMETER_TYPE_CBV, vis, reg));
	return *this;
}

RootSignatureBuilder& RootSignatureBuilder::SRV(UINT reg, D3D12_SHADER_VISIBILITY vis) {
	AddParm(RootParm(D3D12_ROOT_PARAMETER_TYPE_SRV, vis, reg));
	return *this;
}

RootSignatureBuilder& RootSignatureBuilder::UAV(UINT reg, D3D12_SHADER_VISIBILITY vis) {
	AddParm(RootParm(D3D12_ROOT_PARAMETER_TYPE_UAV, vis, reg));
	return *this;
}



RootSignatureBuilder& RootSignatureBuilder::UAVTable(UINT shaderRegister, UINT count, D3D12_SHADER_VISIBILITY visibility) {
	descriptorTables_.emplace_back();
	auto& entry = descriptorTables_.back();

	entry.ranges.push_back({ D3D12_DESCRIPTOR_RANGE_TYPE_UAV, count, shaderRegister, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND });
	entry.param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	entry.param.ShaderVisibility = visibility;
	entry.param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(entry.ranges.size());
	entry.param.DescriptorTable.pDescriptorRanges = entry.ranges.data();

	params_.push_back(entry.param);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		samplerを作成
/////////////////////////////////////////////////////////////////////////////////////////
RootSignatureBuilder& RootSignatureBuilder::StaticSampler(const D3D12_STATIC_SAMPLER_DESC& s){
	samplers_.push_back(s); return *this;
}

RootSignatureBuilder& RootSignatureBuilder::SampleClampLinear(UINT reg, D3D12_SHADER_VISIBILITY vis) {
	samplers_.push_back(MakeSampler(reg, vis, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
									D3D12_TEXTURE_ADDRESS_MODE_CLAMP));
	return *this;
}

RootSignatureBuilder& RootSignatureBuilder::SamplerWrapLinear(UINT reg, D3D12_SHADER_VISIBILITY vis){
	samplers_.push_back(MakeSampler(reg, vis, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
						D3D12_TEXTURE_ADDRESS_MODE_WRAP));
	return *this;
}

RootSignatureBuilder& RootSignatureBuilder::SamplerWrapAniso(UINT reg, UINT maxAniso, D3D12_SHADER_VISIBILITY vis){
	auto s = MakeSampler(reg, vis, D3D12_FILTER_ANISOTROPIC,
						 D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	s.MaxAnisotropy = maxAniso;
	samplers_.push_back(s);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		フラグを設定
/////////////////////////////////////////////////////////////////////////////////////////
RootSignatureBuilder& RootSignatureBuilder::AllowIA(){ flags_ |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; return *this; }

RootSignatureBuilder& RootSignatureBuilder::AllowDXR() {
	flags_ |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
	flags_ |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
	return *this;
}

RootSignatureBuilder& RootSignatureBuilder::DenyVS(){ flags_ |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;    return *this; }

RootSignatureBuilder& RootSignatureBuilder::DenyPS(){ flags_ |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;     return *this; }

/////////////////////////////////////////////////////////////////////////////////////////
//		descriptor table
/////////////////////////////////////////////////////////////////////////////////////////
RootSignatureBuilder& RootSignatureBuilder::SRVTable(UINT shaderRegister, UINT count, D3D12_DESCRIPTOR_RANGE_TYPE type, D3D12_SHADER_VISIBILITY visibility) {
	descriptorTables_.emplace_back();
	auto& entry = descriptorTables_.back();

	entry.ranges.push_back({ type, count, shaderRegister, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND });
	entry.param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	entry.param.ShaderVisibility = visibility;
	entry.param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(entry.ranges.size());
	entry.param.DescriptorTable.pDescriptorRanges = entry.ranges.data();

	params_.push_back(entry.param);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		rootSignatureの構築
/////////////////////////////////////////////////////////////////////////////////////////
D3D12_ROOT_SIGNATURE_DESC RootSignatureBuilder::Desc() const{
	D3D12_ROOT_SIGNATURE_DESC d {};
	d.Flags = flags_;
	d.NumParameters = static_cast< UINT >(params_.size());
	d.pParameters = params_.data();
	d.NumStaticSamplers = static_cast< UINT >(samplers_.size());
	d.pStaticSamplers = samplers_.data();
	return d;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ハッシュ値を取得
/////////////////////////////////////////////////////////////////////////////////////////
size_t RootSignatureBuilder::Hash() const{
	size_t h = std::hash<size_t> {}(params_.size()) ^ (samplers_.size() << 1) ^ flags_;
	for (auto const& p : params_) h ^= (p.ParameterType + (p.Descriptor.ShaderRegister << 4));
	return h;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		rootSignatureのparamterを作成
/////////////////////////////////////////////////////////////////////////////////////////
D3D12_ROOT_PARAMETER RootSignatureBuilder::RootParm(D3D12_ROOT_PARAMETER_TYPE type, D3D12_SHADER_VISIBILITY vis, UINT reg) {
	D3D12_ROOT_PARAMETER rootParam{};
	rootParam.ParameterType = type;
	rootParam.ShaderVisibility = vis;
	rootParam.Descriptor.ShaderRegister = reg;
	return rootParam;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		sampler作成
/////////////////////////////////////////////////////////////////////////////////////////
D3D12_STATIC_SAMPLER_DESC RootSignatureBuilder::MakeSampler(UINT reg, D3D12_SHADER_VISIBILITY vis, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrMode) {
	D3D12_STATIC_SAMPLER_DESC s{};
	s.Filter = filter;
	s.AddressU = s.AddressV = s.AddressW = addrMode;
	s.MipLODBias = 0.0f;
	s.MaxAnisotropy = (filter == D3D12_FILTER_ANISOTROPIC) ? 16 : 0;
	s.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	s.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	s.MinLOD = 0.0f;
	s.MaxLOD = D3D12_FLOAT32_MAX;
	s.ShaderRegister = reg;
	s.RegisterSpace = 0;
	s.ShaderVisibility = vis;
	return s;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		rootParamterを追加
/////////////////////////////////////////////////////////////////////////////////////////
void RootSignatureBuilder::AddParm(D3D12_ROOT_PARAMETER param){
	params_.push_back(param);
}