#include "BaseHud.h"

namespace Calyx2D {

	/////////////////////////////////////////////////////////////////////////////////////////
	//		コンストラクタ / デストラクタ
	/////////////////////////////////////////////////////////////////////////////////////////
	BaseHud::BaseHud()	= default;
	BaseHud::~BaseHud() = default;

	//////////////////////////////////////////////////////////////////////////////////////////
	//		初期化
	//////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::Initialize(const HudConfig& config) {
		config_ = config;

		// スプライト生成
		spriteObj_ = std::make_unique<SpriteObject2d>();
		spriteObj_->Initialize(config_.texturePath);

		// モーション初期化（全チャネル使用）
		motion_.Initialize(
			static_cast<uint32_t>(HudMotionChannel::Position) |
			static_cast<uint32_t>(HudMotionChannel::Scale) |
			static_cast<uint32_t>(HudMotionChannel::Alpha) |
			static_cast<uint32_t>(HudMotionChannel::Rotation));

		// 登場開始
		StartEnter();
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//		更新
	//////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::Update(float dt) {
		if(phase_ == HudPhase::End) return;

		// モーション更新
		motion_.Update(dt);

		// Spriteへ反映
		spriteObj_->SetPosition(motion_.GetPosition());
		spriteObj_->SetScale(motion_.GetScale());
		spriteObj_->SetRotation(motion_.GetRotation());
		spriteObj_->SetAlpha(motion_.GetAlpha());

		switch(phase_) {

		//==================================================================================
		//		登場フェーズ
		//==================================================================================
		case HudPhase::Enter:
			if(motion_.IsFinished()) {
				phase_ = HudPhase::Stay;
				OnEnterFinished();
			}
			break;

		//==================================================================================
		//		滞在フェーズ
		//==================================================================================
		case HudPhase::Stay:
			StayUpdate(dt);
			break;

		//==================================================================================
		//		退場フェーズ
		//==================================================================================
		case HudPhase::Exit:
			if(motion_.IsFinished()) {
				phase_ = HudPhase::End;
				OnExitFinished();
			}
			break;

		default:
			break;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//		描画
	//////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::Draw(SpriteRenderer* renderer) const {
		if(phase_ == HudPhase::End) return;
		spriteObj_->Draw(renderer);
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//		登場開始
	//////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::StartEnter() {
		// 登場モーション開始
		motion_.ApplyMotionSet(config_.enterMotion);
		phase_ = HudPhase::Enter;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//		退場開始
	//////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::StartExit() {
		if(phase_ != HudPhase::Stay) return;

		// 退場モーション開始
		motion_.ApplyMotionSet(config_.exitMotion);
		phase_ = HudPhase::Exit;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//		滞在フェーズ更新
	//////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::StayUpdate(float dt) {
		(void)dt;
	}

} // namespace Calyx2D
