#pragma once

////////////////////////////////////////////////////////////
//  include
////////////////////////////////////////////////////////////
/* engine */
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/scene/Base/BaseScene.h>
#include <Engine/Renderer/Sprite/Sprite.h>

/* game */
#include <Game/Runtime/Engagement/EnemyEngagementService.h>

/* c++ */
#include <memory>

class DefeatScene final
	: public BaseScene {
public:
	DefeatScene();
	~DefeatScene() override;

	void Initialize() override;
	void Update(float dt) override;
	void Draw(ID3D12GraphicsCommandList*,class PipelineService*,IRenderTarget* ) override;
	void CleanUp() override;
	void LoadAssets() override;


private:
	/* objects ======================================================*/
	std::unique_ptr<Sprite> defeatSprite_ = nullptr;
	std::unique_ptr<Sprite> buttonSprite = nullptr;
	/* UIs ==========================================================*/
	std::unique_ptr<Sprite> attackSprite_;

	/* runtime services =============================================*/
	bool  blinkState    = true;     // 表示/非表示
	float blinkTimer    = 0.0f;     // 経過時間
	float blinkInterval = 0.5f;    // 反転間隔(秒)
};