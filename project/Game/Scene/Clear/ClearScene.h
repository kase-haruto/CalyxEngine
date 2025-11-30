#pragma once

////////////////////////////////////////////////////////////
//  include
////////////////////////////////////////////////////////////
/* engine */
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/scene/Base/BaseScene.h>

/* game */
#include <Game/Runtime/Engagement/EnemyEngagementService.h>

/* c++ */
#include <memory>

class NumbersSprite;

class ClearScene final
	: public BaseScene {
public:
	ClearScene();
	~ClearScene() override;

	void Initialize() override;
	void Update(float dt) override;
	void Draw(ID3D12GraphicsCommandList*,class PipelineService*,RenderTargetType) override;
	void CleanUp() override;
	void LoadAssets() override;

	void SetPayload(const SceneTransitionPayload& payload);

private:
	/* objects ======================================================*/
	std::unique_ptr<Sprite> buttonSprite_ = nullptr;
	std::unique_ptr<NumbersSprite> scoreSprite_ = nullptr;
	int32_t finalScore_;

	/* runtime services =============================================*/
	bool  blinkState    = true;     // 表示/非表示
	float blinkTimer    = 0.0f;     // 経過時間
	float blinkInterval = 0.5f;    // 反転間隔(秒)
};