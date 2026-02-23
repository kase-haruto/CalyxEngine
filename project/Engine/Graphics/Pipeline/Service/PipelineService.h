#pragma once

// engine
#include <Engine/Graphics/Pipeline/Factory/PsoFactory.h>
#include <Engine/Graphics/Pipeline/Library/PsoLibrary.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/Graphics/Pipeline/Shader/ShaderCompiler.h>


// std
#include <array>

/* ========================================================================
/*		パイプライン配膳サービス
/* ===================================================================== */
class PipelineService {
public:
	//===================================================================*/
	//		structs
	//===================================================================*/
	struct PipelineKey {
		PipelineTag::Object tag;
		BlendMode			blend;

		bool operator==(const PipelineKey& rhs) const {
			return tag == rhs.tag && blend == rhs.blend;
		}

		bool operator<(const PipelineKey& rhs) const {
			// Rank BlendMode so opaque (NONE, NORMAL) is drawn before transparent (ALPHA, ADD, etc.)
			auto getRank = [](BlendMode b) {
				if(b == BlendMode::NONE || b == BlendMode::NORMAL) return 0;
				return 1;
			};

			int rankL = getRank(blend);
			int rankR = getRank(rhs.blend);

			if(rankL != rankR) return rankL < rankR;
			if(tag != rhs.tag) return tag < rhs.tag;
			return blend < rhs.blend;
		}
	};

	struct PipelineKeyHasher {
		std::size_t operator()(const PipelineKey& k) const {
			return std::hash<int>()(static_cast<int>(k.tag)) ^ (std::hash<int>()(static_cast<int>(k.blend)) << 1);
		}
	};

public:
	//===================================================================*/
	//		public functions
	//===================================================================*/
	PipelineService();
	~PipelineService() = default;

	/// <summary>
	/// すべてのパイプライン登録
	/// </summary>
	void RegisterAllPipelines();

	/// <summary>
	/// desc空登録
	/// </summary>
	/// <param name="desc"></param>
	void Register(const GraphicsPipelineDesc& desc);

	/// <summary>
	/// コマンドに積む
	/// </summary>
	/// <param name="desc"></param>
	/// <param name="cmd"></param>
	void SetCommand(const GraphicsPipelineDesc& desc, ID3D12GraphicsCommandList* cmd);
	void SetCommand(const PipelineSet& set, ID3D12GraphicsCommandList* cmd) const;

	//--------- accessor -------------------------------------------------//
	const PipelineSet	 GetPipelineSet(const GraphicsPipelineDesc& desc) const;
	ID3D12PipelineState* GetPipelineState(const GraphicsPipelineDesc& desc) { return library_->GetOrCreate(desc); }
	ID3D12RootSignature* GetRootSig(const GraphicsPipelineDesc& desc) { return library_->GetRoot(desc); }

	PipelineSet		   GetPipelineSet(PipelineTag::Object tag, BlendMode blend = BlendMode::NORMAL) const;
	PipelineSet		   GetPipelineSet(PipelineTag::PostProcess tag) const;
	const PipelineSet& GetComputePipelineSet(PipelineTag::Compute tag) const {
		return csCache_[static_cast<size_t>(tag)];
	}
	ShaderCompiler* GetCompiler() const { return compiler_.get(); }
	PsoFactory*		GetFactory() const { return factory_.get(); }
	PsoLibrary*		GetLibrary() const { return library_.get(); }

	void ResetState() {
		lastPipelineState_ = nullptr;
		lastRootSignature_ = nullptr;
	}

private:
	//===================================================================*/
	//		private variables
	//===================================================================*/
	std::unique_ptr<ShaderCompiler> compiler_;
	std::unique_ptr<PsoFactory>		factory_;
	std::unique_ptr<PsoLibrary>		library_;

	mutable ID3D12PipelineState* lastPipelineState_ = nullptr;
	mutable ID3D12RootSignature* lastRootSignature_ = nullptr;

	std::unordered_map<PipelineKey, PipelineSet, PipelineKeyHasher>					  objCache_;
	std::array<PipelineSet, static_cast<size_t>(PipelineTag::PostProcess::Count)>	  ppCache_{};
	std::array<PipelineSet, static_cast<size_t>(PipelineTag::Compute::kComputeCount)> csCache_{};
};
