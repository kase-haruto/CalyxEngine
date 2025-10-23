#pragma once

#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Engine/Foundation/Math/Vector2.h>

#include <string>
#include <vector>
#include <algorithm>

class TextureSheetAnimationModule
	: public BaseFxModule{
public:
	struct FrameUV{
		Vector2 offset;
		Vector2 scale;
	};

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
	int GetRows() const{ return rows_; }
	int GetCols() const{ return cols_; }
	bool GetLoop() const{ return loop_; }
	float GetAnimationSpeed() const{ return animationSpeed_; }
	bool GetUseCustomFrames() const{ return useCustomFrames_; }
	std::vector<FrameUV> GetCustomFrameUVs() const{ return customFrameUVs_; }

private:
	int rows_ = 4;
	int cols_ = 4;
	int totalFrames_ = 16;
	bool loop_ = true;
	float animationSpeed_ = 10.0f;

	bool useCustomFrames_ = false;
	std::vector<FrameUV> customFrameUVs_;
};