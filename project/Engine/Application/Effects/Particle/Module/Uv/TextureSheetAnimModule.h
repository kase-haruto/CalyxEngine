#pragma once

// engine
#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Engine/Foundation/Math/Vector2.h>

// std
#include <string>
#include <vector>

namespace CalyxEngine {
	/* ========================================================================
	/*		uvアニメーションを行うモジュール
	/* ===================================================================== */
	class TextureSheetAnimationModule
		: public BaseFxModule {
	public:
		//===================================================================*/
		//					structs
		//===================================================================*/
		struct FrameUV {
			CalyxEngine::Vector2 offset;
			CalyxEngine::Vector2 scale;
		};

	public:
		//===================================================================*/
		//					public methods
		//===================================================================*/
		TextureSheetAnimationModule(const std::string& name);

		void ShowGuiContent() override;
		void OnUpdate(struct FxUnit&, float) override;

		//--------- setters -----------------------------------------------------
		void SetLoop(bool enable);
		void SetAnimationSpeed(float speed);
		void UseGridMode(int rows, int cols);
		void SetCustomFrameUVs(const std::vector<FrameUV>& uvFrames);
		void SetUseCustomFrames(bool enable);

		//--------- getters -----------------------------------------------------
		int					 GetRows() const { return rows_; }
		int					 GetCols() const { return cols_; }
		bool				 GetLoop() const { return loop_; }
		float				 GetAnimationSpeed() const { return animationSpeed_; }
		bool				 GetUseCustomFrames() const { return useCustomFrames_; }
		std::vector<FrameUV> GetCustomFrameUVs() const { return customFrameUVs_; }
		virtual const char*	 GetObjectClassName() const override { return "TextureSheetAnimationModule"; }

	private:
		//===================================================================*/
		//					private methods
		//===================================================================*/
		int	  rows_			   = 4;		//< 分割数(行
		int	  cols_			   = 4;		//< 分割数(列
		int	  totalFrames_	   = 16;	//< アニメーション時間
		float animationSpeed_  = 10.0f; //< アニメーション速度
		bool  loop_			   = true;	//< ループフラグ
		bool  useCustomFrames_ = false; //< カスタムするか

		std::vector<FrameUV> customFrameUVs_;
	};
} // namespace CalyxEngine