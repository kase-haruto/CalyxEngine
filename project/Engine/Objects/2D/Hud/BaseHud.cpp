#include "BaseHud.h"

namespace Calyx2D {

	/////////////////////////////////////////////////////////////////////////////////////////
	//		コンストラクタ / デストラクタ
	/////////////////////////////////////////////////////////////////////////////////////////
	BaseHud::BaseHud()  = default;
	BaseHud::~BaseHud() = default;

	//////////////////////////////////////////////////////////////////////////////////////////
	//		初期化
	//////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::Initialize(uint32_t moveFlags) {

		// スプライトテクスチャが未設定ならデバッグ用に差し替える
		if(config_.texturePath.empty()) { config_.texturePath = "Textures/uvChecker.dds"; }

		// スプライト生成
		spriteObj_ = std::make_unique<SpriteObject2d>();
		spriteObj_->Initialize(config_.texturePath);

		// モーション初期化（全チャネル使用）
		motion_.Initialize(moveFlags);

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

		ApplyMotionValue();

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

		spriteObj_->Update(dt);
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	//		GUI表示
	///////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::ShowGui() {

		TopGui();

		// タイムライン表示
		motion_.ShowTimelineGui();

		if(ImGui::Button("Start Enter")) { StartEnter(); }
		ImGui::SameLine();
		if(ImGui::Button("Start Exit")) { StartExit(); }

		bool changed = false;

		// if(ImGui::CollapsingHeader("Enter Motion")) { changed |= config_.enterMotion.ShowGui("Enter Motion"); }
		//
		// if(ImGui::CollapsingHeader("Exit Motion")) { changed |= config_.exitMotion.ShowGui("Exit Motion"); }

		// GUI変更があったら即 Enter 再生
		if(changed) { StartEnter(); }

		//派生先の調整Gui
		DerivedGui();
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
		motion_.Reset();
		// 登場モーション開始
		motion_.ApplyMotionSet(config_.enterMotion);
		phase_ = HudPhase::Enter;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//		退場開始
	//////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::StartExit() {
		if(phase_ != HudPhase::Stay) return;
		motion_.Reset();
		// 退場モーション開始
		motion_.ApplyMotionSet(config_.exitMotion);
		phase_ = HudPhase::Exit;
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	//		モーション値適用
	///////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::ApplyMotionValue() const {
		// Position
		if(motion_.IsChannelEnabled(HudMotionChannel::Position)) { spriteObj_->SetPosition(motion_.GetPosition()); }

		// Scale
		if(motion_.IsChannelEnabled(HudMotionChannel::Scale)) { spriteObj_->SetScale(motion_.GetScale()); }

		// Rotation
		if(motion_.IsChannelEnabled(HudMotionChannel::Rotation)) { spriteObj_->SetRotation(motion_.GetRotation()); }

		// Alpha
		if(motion_.IsChannelEnabled(HudMotionChannel::Alpha)) { spriteObj_->SetAlpha(motion_.GetAlpha()); }
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//		滞在フェーズ更新
	//////////////////////////////////////////////////////////////////////////////////////////
	void BaseHud::StayUpdate(float dt) { (void)dt; }

} // namespace Calyx2D