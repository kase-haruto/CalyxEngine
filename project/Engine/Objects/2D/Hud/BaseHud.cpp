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
		// コンフィグから設定構築
		config_	   = config;
		spriteObj_ = std::make_unique<SpriteObject2d>();
		spriteObj_->Initialize(config_.texturePath);

		// 座標スタート
		moveAnim_.SetStart(config.startPos_);
		moveAnim_.SetEnd(config.endPos_);

		// アニメーション設定
		moveAnim_.SetLoopType(CalyxUtil::AnimationLoop::AnimationLoopType::PingPong);
		moveAnim_.Start();

		// フェーズ初期化
		phase_ = HudPhase::Enter;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//		更新
	//////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::Update(float dt) {
		if(phase_ == HudPhase::End) return;

		CalyxMath::Vector2 pos;
		moveAnim_.LerpValue(pos, dt);
		spriteObj_->SetPosition(pos);

		switch(phase_) {

		//==================================================================================
		//		入場フェーズ
		//==================================================================================
		case HudPhase::Enter:
			if(moveAnim_.IsFinished()) {
				phase_ = HudPhase::Stay;
				// 入場完了コールバック
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
			if(moveAnim_.IsFinished()) {
				phase_ = HudPhase::End;
				// 退場完了コールバック
				OnExitFinished();
			}
			break;

		default:
			break;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//		GUI表示
	//////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::Draw(SpriteRenderer* renderer) const {
		// 退場し終わったら表示しない
		if(phase_ == HudPhase::End) return;
		spriteObj_->Draw(renderer);
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	//		終了処理開始
	///////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::StartExit() {
		if(phase_ != HudPhase::Stay) return;

		// アニメーションリセット
		moveAnim_.Reset();
		moveAnim_.SetStart(config_.stayPos_);
		moveAnim_.SetEnd(config_.endPos_);
		moveAnim_.SetLoopCount(1);
		moveAnim_.Start();

		// フェーズ変更
		phase_ = HudPhase::Exit;
	}
	
	void BaseHud::StartEnter() {
		// アニメーションリセット
		moveAnim_.Reset();
		moveAnim_.SetStart(config_.startPos_);
		moveAnim_.SetEnd(config_.stayPos_);
		moveAnim_.SetLoopCount(1);
		moveAnim_.Start();

		// フェーズ変更
		phase_ = HudPhase::Enter;
	}
	///////////////////////////////////////////////////////////////////////////////////////////
	//		滞在フェーズ更新
	///////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::StayUpdate(float dt) {
		(void)dt;
	}

} // namespace Calyx2D
