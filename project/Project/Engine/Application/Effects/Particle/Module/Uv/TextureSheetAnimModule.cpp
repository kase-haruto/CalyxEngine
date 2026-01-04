#include "TextureSheetAnimModule.h"

#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/Foundation/Math/Vector2.h>

TextureSheetAnimationModule::TextureSheetAnimationModule(const std::string& name)
	: BaseFxModule(name),
	rows_(4), cols_(4), loop_(true), animationSpeed_(10.0f),
	useCustomFrames_(false){
	totalFrames_ = rows_ * cols_;
}

void TextureSheetAnimationModule::ShowGuiContent(){
	GuiCmd::DragInt("Rows", rows_);
	GuiCmd::DragInt("Cols", cols_);
	GuiCmd::CheckBox("Loop", loop_);
	GuiCmd::DragFloat("Animation Speed (fps)", animationSpeed_);

	if (useCustomFrames_){
	}

	// 総フレーム数更新
	totalFrames_ = useCustomFrames_ ? static_cast< int >(customFrameUVs_.size())
		: rows_ * cols_;
}

void TextureSheetAnimationModule::OnUpdate(FxUnit& unit,[[maybe_unused]] float dt){
	if (unit.lifetime <= 0.0f || totalFrames_ == 0)
		return;

	// フレームインデックス計算
	float framePos = unit.age * animationSpeed_;
	int frameIndex = static_cast< int >(framePos);

	if (loop_){
		frameIndex %= totalFrames_;
	} else{
		frameIndex = ( std::min ) (frameIndex, totalFrames_ - 1);
	}

	if (useCustomFrames_ && !customFrameUVs_.empty()){
		frameIndex = std::clamp(frameIndex, 0, static_cast< int >(customFrameUVs_.size()) - 1);
		unit.uvTransform.translate = customFrameUVs_[frameIndex].offset;
		unit.uvTransform.scale = customFrameUVs_[frameIndex].scale;
	} else{
		// グリッド方式
		int col = frameIndex % cols_;
		int row = frameIndex / cols_;

		float uStep = 1.0f / static_cast< float >(cols_);
		float vStep = 1.0f / static_cast< float >(rows_);

		unit.uvTransform.translate = Vector2(uStep * col, vStep * row);
		unit.uvTransform.scale = Vector2(uStep, vStep);
	}
}

void TextureSheetAnimationModule::SetCustomFrameUVs(const std::vector<FrameUV>& uvFrames){
	customFrameUVs_ = uvFrames;
	useCustomFrames_ = true;
	totalFrames_ = static_cast< int >(customFrameUVs_.size());
}

void TextureSheetAnimationModule::UseGridMode(int rows, int cols){
	rows_ = rows;
	cols_ = cols;
	useCustomFrames_ = false;
	totalFrames_ = rows_ * cols_;
}

void TextureSheetAnimationModule::SetLoop(bool enable){
	loop_ = enable;
}

void TextureSheetAnimationModule::SetAnimationSpeed(float speed){
	animationSpeed_ = speed;
}

void TextureSheetAnimationModule::SetUseCustomFrames(bool enable){
	useCustomFrames_ = enable;
	totalFrames_ = enable ? static_cast< int >(customFrameUVs_.size())
		: (rows_ * cols_);
}
