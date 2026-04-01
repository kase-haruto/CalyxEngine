#include "PipelineService.h"

#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
PipelineService::PipelineService() {
	compiler_ = std::make_unique<ShaderCompiler>();
	compiler_->InitializeDXC();
	factory_ = std::make_unique<PsoFactory>(compiler_.get());
	library_ = std::make_unique<PsoLibrary>(factory_.get());
}

/////////////////////////////////////////////////////////////////////////////////////////
//		すべてのパイプラインを登録
/////////////////////////////////////////////////////////////////////////////////////////
void PipelineService::RegisterAllPipelines() {

	//===================================================================*/
	//					パイプラインの作成、登録
	//===================================================================*/

	auto regObj = [&](PipelineTag::Object tag, BlendMode mode, auto makeFn) {
		GraphicsPipelineDesc desc = makeFn(mode);
		auto pso = library_->GetOrCreate(desc);
		auto root = library_->GetRoot(desc);
		PipelineKey key{ tag, mode };
		objCache_[key] = { pso, root };
	};

	auto regObjNoBlend = [&](PipelineTag::Object tag, auto makeFn) {
		GraphicsPipelineDesc desc = makeFn();
		auto pso = library_->GetOrCreate(desc);
		auto root = library_->GetRoot(desc);
		PipelineKey key{ tag, BlendMode::NONE };
		objCache_[key] = { pso, root };
	};

	auto regPP = [&](PipelineTag::PostProcess tag, auto makeFn) {
		GraphicsPipelineDesc desc = makeFn();
		auto pso = library_->GetOrCreate(desc);
		auto root = library_->GetRoot(desc);
		ppCache_[static_cast<size_t>(tag)] = { pso, root };
	};

	auto regCS = [&](PipelineTag::Compute tag, auto makeFn) {
		GraphicsPipelineDesc desc = makeFn(); 
		auto pso = library_->GetOrCreate(desc);
		auto root = library_->GetRoot(desc);
		csCache_[static_cast<size_t>(tag)] = { pso, root };
	};


	//=================== Object Pipelines ================================


	for (int i = 0; i < static_cast< int >(BlendMode::kBlendModeCount); ++i){
		BlendMode mode = static_cast< BlendMode >(i);
		
		//===================================================================*/
		//						Object3D Pipelines
		//===================================================================*/
		regObj(PipelineTag::Object::Object3d, mode, PipelinePresets::MakeObject3D);
		
		//===================================================================*/
		//						SkinObject3D Pipelines
		//===================================================================*/
		regObj(PipelineTag::Object::SkinningObject3D, mode, PipelinePresets::MakeSkinningObject3D);

		//===================================================================*/
		//						Wireframe Pipelines
		//===================================================================*/
		regObj(PipelineTag::Object::WireframeObject3D, mode, PipelinePresets::MakeWireframeObject3D);
		regObj(PipelineTag::Object::WireframeSkinnedObject3D, mode, PipelinePresets::MakeWireframeSkinnedObject3D);

		//===================================================================*/
		//						Particle Pipelines
		//===================================================================*/
		regObj(PipelineTag::Object::Particle, mode, PipelinePresets::MakeParticle);
		regObj(PipelineTag::Object::GpuParticle, mode, PipelinePresets::MakeGpuParticle);
	}

	//========================= Shadow ===================================
	regObjNoBlend(PipelineTag::Object::ShadowStatic, PipelinePresets::MakeShadowStatic);
	regObjNoBlend(PipelineTag::Object::ShadowSkinned, PipelinePresets::MakeShadowSkinned);

#if defined(_DEBUG) || defined(DEVELOP)
	//========================= Picking ===================================
	regObjNoBlend(PipelineTag::Object::PickingObject3D, PipelinePresets::MakePickingStatic);
	regObjNoBlend(PipelineTag::Object::PickingSkinned, PipelinePresets::MakePickingSkinned);
#endif

	//=================== cs Pipelines ===================================

	//===================================================================*/
	//						gpuParticle
	//===================================================================*/
	regCS(PipelineTag::Compute::ParticleInitializeCompute, PipelinePresets::MakeGpuParticleCS);
	regCS(PipelineTag::Compute::ParticleEmitCompute, PipelinePresets::MakeGpuParticleEmit);
	regCS(PipelineTag::Compute::ParticleUpdateCompute, PipelinePresets::MakeGpuParticleUpdate);

	//=================== PostProcess Pipelines ==========================
	regPP(PipelineTag::PostProcess::GrayScale, PipelinePresets::MakeGrayScale);
	regPP(PipelineTag::PostProcess::RadialBlur, PipelinePresets::MakeRadialBlur);
	regPP(PipelineTag::PostProcess::Vignette, PipelinePresets::MakeVignette);
	regPP(PipelineTag::PostProcess::ChromaticAberration, PipelinePresets::MakeChromaticAberration);
	regPP(PipelineTag::PostProcess::CRT, PipelinePresets::MakeCRT);
	regPP(PipelineTag::PostProcess::CopyImage, PipelinePresets::MakeCopyImage);

}

/////////////////////////////////////////////////////////////////////////////////////////
//		登録
/////////////////////////////////////////////////////////////////////////////////////////
void PipelineService::Register(const GraphicsPipelineDesc& desc) {
	library_->GetOrCreate(desc);
}

void PipelineService::SetCommand(const GraphicsPipelineDesc& desc, ID3D12GraphicsCommandList* cmd) {
	GetPipelineSet(desc).SetCommand(cmd);
}

void PipelineService::SetCommand(const PipelineSet& set, ID3D12GraphicsCommandList* cmd) const {
	if (set.pipelineState != lastPipelineState_){
		cmd->SetPipelineState(set.pipelineState);
		lastPipelineState_ = set.pipelineState;
	}
	if (set.rootSignature != lastRootSignature_){
		cmd->SetGraphicsRootSignature(set.rootSignature);
		lastRootSignature_ = set.rootSignature;
	}
}

const PipelineSet PipelineService::GetPipelineSet(const GraphicsPipelineDesc& desc) const {
	return {library_->GetOrCreate(desc), library_->GetRoot(desc)};
}

/////////////////////////////////////////////////////////////////////////////////////////
//		pso取得(グラフィックパイプライン)
/////////////////////////////////////////////////////////////////////////////////////////
PipelineSet PipelineService::GetPipelineSet(PipelineTag::Object tag, BlendMode blend) const{
	PipelineKey key {tag, blend};
	auto it = objCache_.find(key);
	if (it != objCache_.end()){
		return it->second;
	}
	assert(false && "Pipeline not found!");
	return {};
}


/////////////////////////////////////////////////////////////////////////////////////////
//		pso取得(ポストプロセス
/////////////////////////////////////////////////////////////////////////////////////////
PipelineSet PipelineService::GetPipelineSet(PipelineTag::PostProcess tag) const{
	return ppCache_[static_cast< size_t >(tag)];
}