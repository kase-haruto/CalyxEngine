#include "TransitionControl.h"
#include <Engine/Application/System/Environment.h>
#include <Engine/Renderer/Sprite/SpriteRenderer.h>
#include <algorithm>

TransitionControl::TransitionControl()	= default;
TransitionControl::~TransitionControl() = default;

void TransitionControl::Initialize(const std::string& texPath1, const std::string& texPath2, const std::string& texPath3) {
	plate1_ = std::make_unique<CalyxEngine::SpriteObject2d>();
	plate1_->Initialize(texPath1);
	plate1_->SetPosition({0.0f, 0.0f});
	plate1_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
	plate1_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate1_->Update(0.0f);

	if(!texPath2.empty()) {
		plate2_ = std::make_unique<CalyxEngine::SpriteObject2d>();
		plate2_->Initialize(texPath2);
		plate2_->SetPosition({0.0f, 0.0f});
		plate2_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
		plate2_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		plate2_->Update(0.0f);
	}

	if(!texPath3.empty()) {
		plate3_ = std::make_unique<CalyxEngine::SpriteObject2d>();
		plate3_->Initialize(texPath3);
		plate3_->SetPosition({0.0f, 0.0f});
		plate3_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
		plate3_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		plate3_->Update(0.0f);
	}

	state_ = TransitionState::Idle;
}

void TransitionControl::Update(float dt) {
	if(state_ == TransitionState::Idle || state_ == TransitionState::Full) return;

	timer_ += dt;
	float progress = (std::min)(timer_ / duration_, 1.0f);

	if(updateFunc_) {
		updateFunc_(progress, plate1_.get(), plate2_.get(), plate3_.get());
	}


	if(plateAnim1_) plateAnim1_->Update(dt);
	if(plateAnim2_) plateAnim2_->Update(dt);
	if(plateAnim3_) plateAnim3_->Update(dt);
	if(plate1_) plate1_->Update(dt);
	if(plate2_) plate2_->Update(dt);
	if(plate3_) plate3_->Update(dt);

	if(progress >= 1.0f) {
		auto cb				= onFinishedCallback_;
		onFinishedCallback_ = nullptr;

		if(state_ == TransitionState::Closing) {
			state_ = TransitionState::Full;
		} else {
			state_ = TransitionState::Idle;
		}

		if(cb) cb();
		if(plate1_) {
			plate1_->SetPosition({1280.0f, 0.0f});
			plate1_->SetAlpha(1.0f);
			plate1_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
			plate1_->Update(dt);
		}
		if(plate2_) {
			plate2_->SetPosition({1280.0f, 0.0f});
			plate2_->SetAlpha(1.0f);
			plate2_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
			plate2_->Update(dt);
		}
		if(plate3_) {
			plate3_->SetPosition({1280.0f, 0.0f});
			plate3_->SetAlpha(1.0f);
			plate3_->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
			plate3_->Update(dt);
		}
	}
}

void TransitionControl::Draw(SpriteRenderer* renderer) {
	if(state_ == TransitionState::Idle) return;

	if(plate3_ && isDrawPlate3_) plate3_->Draw(renderer);
	if(plate2_ && isDrawPlate2_) plate2_->Draw(renderer);
	if(plate1_) plate1_->Draw(renderer);
}

void TransitionControl::StartClosing(float duration, std::function<void()> onCovered) {
	state_				= TransitionState::Closing;
	timer_				= 0.0f;
	duration_			= duration;
	onFinishedCallback_ = onCovered;
}

void TransitionControl::StartOpening(float duration, std::function<void()> onOpened) {
	state_				= TransitionState::Opening;
	timer_				= 0.0f;
	duration_			= duration;
	onFinishedCallback_ = onOpened;
}

void TransitionControl::SetPresetSlide(bool toRight) {
	plate1_->Update(0.0f);
	SetUpdateFunc([toRight, this](float progress, CalyxEngine::SpriteObject2d* p1, CalyxEngine::SpriteObject2d* p2, CalyxEngine::SpriteObject2d* p3) {
		p2;
		p3;
		float w = static_cast<float>(kWindowWidth);
		float x = 0.0f;

		if(state_ == TransitionState::Closing) {
			// 画面外から画面内へ
			x = toRight ? -w + w * progress : w - w * progress;
		} else {
			// 画面内から画面外へ
			x = toRight ? w * progress : -w * progress;
		}
		p1->SetPosition({x, 0});
	});
}

void TransitionControl::SetPresetFade() {
	plate1_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});
	plate1_->Update(0.0f);
	SetUpdateFunc([this](float progress, CalyxEngine::SpriteObject2d* p1, CalyxEngine::SpriteObject2d* p2, CalyxEngine::SpriteObject2d* p3) {
		p2;
		p3;
		p1->SetPosition({0, 0});
		p1->SetScale({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});

		if(state_ == TransitionState::Closing) {
			p1->SetAlpha(progress);
		} else {
			p1->SetAlpha(1.0f - progress);
		}
	});
}

void TransitionControl::SetPresetSplit() {
	plate1_->Update(0.0f);
	plate2_->Update(0.0f);
	SetUpdateFunc([this](float progress, CalyxEngine::SpriteObject2d* p1, CalyxEngine::SpriteObject2d* p2, CalyxEngine::SpriteObject2d* p3) {
		if(!p2) return;
		p3;
		float w		= static_cast<float>(kWindowWidth);
		float halfW = w * 0.5f;
		float h		= static_cast<float>(kWindowHeight);

		p1->SetScale({halfW, h});
		p2->SetScale({halfW, h});

		float offset = 0.0f;
		if(state_ == TransitionState::Closing) {
			// 左右から中央へ
			offset = halfW * (1.0f - progress);
			p1->SetPosition({-offset, 0});
			p2->SetPosition({halfW + offset, 0});
		} else {
			// 中央から左右へ
			offset = halfW * progress;
			p1->SetPosition({-offset, 0});
			p2->SetPosition({halfW + offset, 0});
		}
	});
}

void TransitionControl::SetPresetUpDownSlide() {
	plate1_->Update(0.0f);
	plate2_->Update(0.0f);
	plate3_->Update(0.0f);
	if(plateAnim1_) plateAnim1_->Play("Wave");
	if(plateAnim2_) plateAnim2_->Play("Wave");
	if(plateAnim3_) plateAnim3_->Play("Wave");

	SetUpdateFunc([this](float progress, CalyxEngine::SpriteObject2d* p1, CalyxEngine::SpriteObject2d* p2, CalyxEngine::SpriteObject2d* p3) {
		if(!p1 || !p2 || !p3) return;
		float w	 = static_cast<float>(kWindowWidth);
		float h = static_cast<float>(kWindowHeight);

		p1->SetScale({w, h * 1.5f});
		p2->SetScale({w, h * 1.5f});
		p3->SetScale({w, h * 1.5f});

		if(state_ == TransitionState::Closing) {
			// 1: 0.0-0.5, 2: 0.25-0.75, 3: 0.5-1.0
			float p1_local = std::clamp((progress - 0.00f) / 0.5f, 0.0f, 1.0f);
			float p2_local = std::clamp((progress - 0.25f) / 0.5f, 0.0f, 1.0f);
			float p3_local = std::clamp((progress - 0.50f) / 0.5f, 0.0f, 1.0f);

			p1->SetPosition({0.0f, h + (-360.0f - h) * p1_local});
			p2->SetPosition({0.0f, h + (-540.0f - h) * p2_local});
			p3->SetPosition({0.0f, h + (-720.0f - h) * p3_local});
		} else {
			// 3: 0.0-0.5, 2: 0.25-0.75, 1: 0.5-1.0
			float p3_local = std::clamp((progress - 0.00f) / 0.5f, 0.0f, 1.0f);
			float p2_local = std::clamp((progress - 0.25f) / 0.5f, 0.0f, 1.0f);
			float p1_local = std::clamp((progress - 0.50f) / 0.5f, 0.0f, 1.0f);

			p3->SetPosition({0.0f, -720.0f + (h - (-720.0f)) * p3_local});
			p2->SetPosition({0.0f, -540.0f + (h - (-540.0f)) * p2_local});
			p1->SetPosition({0.0f, -360.0f + (h - (- 360.0f)) * p1_local});
		}
	});
}

void TransitionControl::SetAutoPreset(SceneType now, SceneType next) {
	isDrawPlate2_ = false;
	isDrawPlate3_ = false;
	plate1_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate2_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate3_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate1_->SetUvScale({1.0f, 1.0f});
	plate2_->SetUvScale({1.0f, 1.0f});
	plate3_->SetUvScale({1.0f, 1.0f});
	plate1_->SetUvOffset({0.0f, 0.0f});
	plate2_->SetUvOffset({0.0f, 0.0f});
	plate3_->SetUvOffset({0.0f, 0.0f});
	// タイトルからセレクトへはスライド
	if(now == SceneType::TITLE && next == SceneType::SELECT) {
		isDrawPlate2_ = true;
		isDrawPlate3_ = true;
		InitAnim();
		SetPresetUpDownSlide();
	}
	// テスト（ゲーム)からセレクトへは観音開き
	else if(now == SceneType::TEST && next == SceneType::SELECT) {
		isDrawPlate2_ = true;
		SetPresetSplit();
	}
	// テスト（ゲーム)からタイトルへは観音開き
	else if(now == SceneType::TEST && next == SceneType::TITLE) {
		isDrawPlate2_ = true;
		SetPresetSplit();
	}
	// ゲームオーバーやクリアへの遷移はフェード
	else if(next == SceneType::GAMEOVER || next == SceneType::CLEAR) {
		SetPresetFade();
	}
	// その他はデフォルトでフェード
	else {
		SetPresetFade();
	}
}

void TransitionControl::SetAutoPresetFromPrevious(SceneType prev, SceneType now) {
	isDrawPlate2_ = false;
	isDrawPlate3_ = false;

	// 初期位置や初期サイズを設定しておかないと一瞬Init状態で描画される
	plate1_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate2_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate3_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	plate1_->SetUvScale({1.0f, 1.0f});
	plate2_->SetUvScale({1.0f, 1.0f});
	plate3_->SetUvScale({1.0f, 1.0f});
	plate1_->SetUvOffset({0.0f, 0.0f});
	plate2_->SetUvOffset({0.0f, 0.0f});
	plate3_->SetUvOffset({0.0f, 0.0f});
	float w		= static_cast<float>(kWindowWidth);
	float halfW = w * 0.5f;
	float h		= static_cast<float>(kWindowHeight);
	//float halfH = h * 0.5f;
	// タイトルからセレクトに来た時はスライドで開ける
	if(prev == SceneType::TITLE && now == SceneType::SELECT) {
		isDrawPlate2_ = true;
		isDrawPlate3_ = true;
		InitAnim();
		plate1_->SetScale({w, h * 1.5f});
		plate2_->SetScale({w, h * 1.5f});
		plate3_->SetScale({w, h * 1.5f});
		plate1_->SetPosition({0.0f, -360.0f});
		plate2_->SetPosition({0.0f, -540.0f});
		plate3_->SetPosition({0.0f, -720.0f});
		SetPresetUpDownSlide();
	}
	// ゲームからセレクトに来た時は観音開きで開ける
	else if(prev == SceneType::TEST && now == SceneType::SELECT) {
		isDrawPlate2_ = true;
		plate1_->SetScale({halfW, h});
		plate2_->SetScale({halfW, h});

		float offset = 0.0f;
		offset = halfW;
		plate1_->SetPosition({0.0f, 0});
		plate2_->SetPosition({halfW, 0});
		SetPresetSplit();
	}
	// ゲームからタイトルに来た時は観音開きで開ける
	else if(prev == SceneType::TEST && now == SceneType::TITLE) {
		isDrawPlate2_ = true;
		plate1_->SetScale({halfW, h});
		plate2_->SetScale({halfW, h});

		float offset = 0.0f;
		offset		 = halfW;
		plate1_->SetPosition({0.0f, 0});
		plate2_->SetPosition({halfW, 0});
		SetPresetSplit();
	}
	// その他、フェードで開ける
	else {
		SetPresetFade();
	}
}

void TransitionControl::SetTexturePlate1(const std::string& texPath1) {
	plate1_->SetTexture(texPath1);
}

void TransitionControl::SetTexturePlate2(const std::string& texPath2) {
	plate2_->SetTexture(texPath2);
}

void TransitionControl::SetTexturePlate3(const std::string& texPath3) {
	plate3_->SetTexture(texPath3);
}

void TransitionControl::InitAnim() {
	plateAnim1_ = std::make_unique<CalyxEngine::SpriteAnimator2d>();
	plateAnim1_->Bind(plate1_.get());
	plateAnim2_ = std::make_unique<CalyxEngine::SpriteAnimator2d>();
	plateAnim2_->Bind(plate2_.get());
	plateAnim3_ = std::make_unique<CalyxEngine::SpriteAnimator2d>();
	plateAnim3_->Bind(plate3_.get());


	auto asset		   = std::make_shared<CalyxEngine::SpriteAnimationAsset>();
	asset->division	   = {5, 2}; // 例: 4x4分割
	asset->texturePath = "Textures/Transition/wave.png";

	CalyxEngine::SpriteAnimationClip clip;
	clip.name		   = "Wave";
	clip.startFrame	   = 0;
	clip.frameCount	   = 10;
	clip.frameDuration = 0.1f;
	clip.loop		   = true;
	asset->clips.push_back(clip);

	plateAnim1_->SetAnimationAsset(asset);
	plateAnim2_->SetAnimationAsset(asset);
	plateAnim3_->SetAnimationAsset(asset);
}
