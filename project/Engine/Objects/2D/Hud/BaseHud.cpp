#include "BaseHud.h"

namespace CalyxEngine {
	/*-----------------------------------------------------------------------------------------
	 * HudEnterState
	 * - HUDの登場モーション開始と完了判定を担当する状態
	 * - モーションとスプライトの所有権はBaseHudに残す
	 *---------------------------------------------------------------------------------------*/
	class HudEnterState final : public IHudState {
	public:
		/** \brief 登場モーションを開始する \param hud 状態を保持するHUD */
		void Enter(BaseHud& hud) const override { hud.motion_.Reset(); hud.motion_.ApplyMotionSet(hud.config_.enterMotion); }
		/** \brief 登場完了時に滞在状態へ遷移する \param hud 状態を保持するHUD \param dt 前フレームからの経過時間（秒） */
		void Update(BaseHud& hud, float dt) const override;
		/** \brief 登場状態の終了処理を実行する \param hud 状態を保持するHUD */
		void Exit(BaseHud& hud) const override { (void)hud; }
	};

	/*-----------------------------------------------------------------------------------------
	 * HudStayState
	 * - HUDの滞在中に派生クラス固有の更新を委譲する状態
	 *---------------------------------------------------------------------------------------*/
	class HudStayState final : public IHudState {
	public:
		/** \brief 滞在状態を開始する \param hud 状態を保持するHUD */
		void Enter(BaseHud& hud) const override { (void)hud; }
		/** \brief 派生HUDの滞在処理を更新する \param hud 状態を保持するHUD \param dt 前フレームからの経過時間（秒） */
		void Update(BaseHud& hud, float dt) const override { hud.StayUpdate(dt); }
		/** \brief 滞在状態の終了処理を実行する \param hud 状態を保持するHUD */
		void Exit(BaseHud& hud) const override { (void)hud; }
	};

	/*-----------------------------------------------------------------------------------------
	 * HudExitState
	 * - HUDの退場モーション開始と完了判定を担当する状態
	 *---------------------------------------------------------------------------------------*/
	class HudExitState final : public IHudState {
	public:
		/** \brief 退場モーションを開始する \param hud 状態を保持するHUD */
		void Enter(BaseHud& hud) const override { hud.motion_.Reset(); hud.motion_.ApplyMotionSet(hud.config_.exitMotion); }
		/** \brief 退場完了時に終了状態へ遷移する \param hud 状態を保持するHUD \param dt 前フレームからの経過時間（秒） */
		void Update(BaseHud& hud, float dt) const override;
		/** \brief 退場状態の終了処理を実行する \param hud 状態を保持するHUD */
		void Exit(BaseHud& hud) const override { (void)hud; }
	};

	/*-----------------------------------------------------------------------------------------
	 * HudEndState
	 * - HUD更新と描画を停止した終了状態
	 *---------------------------------------------------------------------------------------*/
	class HudEndState final : public IHudState {
	public:
		/** \brief 終了状態を開始する \param hud 状態を保持するHUD */
		void Enter(BaseHud& hud) const override { (void)hud; }
		/** \brief 終了状態を維持する \param hud 状態を保持するHUD \param dt 前フレームからの経過時間（秒） */
		void Update(BaseHud& hud, float dt) const override { (void)hud; (void)dt; }
		/** \brief 終了状態から離れる前処理を実行する \param hud 状態を保持するHUD */
		void Exit(BaseHud& hud) const override { (void)hud; }
	};

	namespace {
		const HudEnterState kHudEnterState;
		const HudStayState kHudStayState;
		const HudExitState kHudExitState;
		const HudEndState kHudEndState;
	}

	void HudEnterState::Update(BaseHud& hud, float dt) const {
		(void)dt;
		// 補間中は登場状態を維持し、完了コールバックの多重発火を防ぐ。
		if(!hud.motion_.IsFinished()) return;

		// Stayへ遷移して状態を確定してから、派生HUDへ登場完了を通知する。
		hud.ChangeState(kHudStayState, BaseHud::HudPhase::Stay);
		hud.OnEnterFinished();
	}

	void HudExitState::Update(BaseHud& hud, float dt) const {
		(void)dt;
		// 退場モーションが表示値を更新している間は描画可能なExit状態を維持する。
		if(!hud.motion_.IsFinished()) return;

		// Endへ遷移して描画を停止してから、派生HUDへ退場完了を通知する。
		hud.ChangeState(kHudEndState, BaseHud::HudPhase::End);
		hud.OnExitFinished();
	}

	BaseHud::BaseHud() = default;
	BaseHud::~BaseHud() = default;

	void BaseHud::Initialize(uint32_t moveFlags) {
		// 未設定時もEditorで状態遷移を確認できるようデバッグテクスチャを補う。
		if(config_.texturePath.empty()) config_.texturePath = "Textures/uvChecker.dds";

		// Spriteの所有権はHUD本体に残し、Stateは表示リソースを所有しない。
		spriteObj_ = std::make_unique<SpriteObject2d>();
		spriteObj_->Initialize(config_.texturePath);

		// 派生HUDが選択したチャネルだけを補間対象としてモーションを初期化する。
		motion_.Initialize(moveFlags);

		// 初期化直後から外部Update APIを変えずに登場シーケンスを開始する。
		StartEnter();
	}

	void BaseHud::Update(float dt) {
		// 未初期化または終了済みの場合は、StateとSpriteの両方を更新しない。
		if(!currentState_ || IsFinished()) return;

		// State判定より先に補間を進め、同じフレームで完了遷移できるようにする。
		motion_.Update(dt);

		// 状態コールバックが参照する前に、最新の補間値をSpriteへ反映する。
		ApplyMotionValue();
		currentState_->Update(*this, dt);

		// Exit完了でEndへ遷移したフレームには不要なSprite更新を記録しない。
		if(!IsFinished()) spriteObj_->Update(dt);
	}

	void BaseHud::ShowGui() {
		TopGui();
		motion_.ShowTimelineGui();
		if(ImGui::Button("Start Enter")) StartEnter();
		ImGui::SameLine();
		if(ImGui::Button("Start Exit")) StartExit();
		DerivedGui();
	}

	void BaseHud::Draw(SpriteRenderer* renderer) const {
		if(IsFinished() || !spriteObj_) return;
		spriteObj_->Draw(renderer);
	}

	void BaseHud::StartEnter() { ChangeState(kHudEnterState, HudPhase::Enter); }
	void BaseHud::StartExit() { if(phase_ == HudPhase::Stay) ChangeState(kHudExitState, HudPhase::Exit); }

	void BaseHud::ChangeState(const IHudState& state, HudPhase phase) {
		// 状態所有者だけが遷移順を管理し、状態クラス間の循環参照を作らない。
		if(currentState_) currentState_->Exit(*this);

		// 静的Stateへの非所有参照と外部互換用phaseを同時に切り替える。
		currentState_ = &state;
		phase_ = phase;

		// 新状態の初期化は参照差し替え後に実行し、再遷移時も現在状態を正しく参照させる。
		currentState_->Enter(*this);
	}

	void BaseHud::ApplyMotionValue() const {
		// 無効チャネルは派生HUDが直接設定した値を保持するため上書きしない。
		if(motion_.IsChannelEnabled(HudMotionChannel::Position)) spriteObj_->SetPosition(motion_.GetPosition());
		if(motion_.IsChannelEnabled(HudMotionChannel::Scale)) spriteObj_->SetScale(motion_.GetScale());
		if(motion_.IsChannelEnabled(HudMotionChannel::Rotation)) spriteObj_->SetRotation(motion_.GetRotation());
		if(motion_.IsChannelEnabled(HudMotionChannel::Alpha)) spriteObj_->SetAlpha(motion_.GetAlpha());
	}

	void BaseHud::StayUpdate(float dt) { (void)dt; }
} // namespace CalyxEngine
