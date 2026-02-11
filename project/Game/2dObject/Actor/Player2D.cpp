#include "Player2D.h"

#include "Engine/Application/System/Environment.h"
#include "Engine/Renderer/Sprite/Sprite.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

///////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ / デストラクタ
///////////////////////////////////////////////////////////////////////////////////////////
Player2D::Player2D() {
	spriteObj_ = std::make_unique<Calyx2D::SpriteObject2d>();
}
Player2D::~Player2D() = default;

///////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
///////////////////////////////////////////////////////////////////////////////////////////
void Player2D::Initialize() {
	spriteObj_->Initialize("Textures/Player/flyingPlayer.dds");
	InitializeSpriteAnimation();
	InitializeSerializableParm();
	// ============================
	// 上下ゆらゆらアニメーション
	// ============================
	auto& swayY = animator_.Add<CalyxMath::Vector2>("SwayY");

	 swayY.Animation().SetStart({0.0f, -amplitude_}); // 上
	 swayY.Animation().SetEnd({0.0f, amplitude_});	 // した
	 swayY.Animation().Start();
}

///////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
///////////////////////////////////////////////////////////////////////////////////////////
void Player2D::Update(float deltaTime) {
	StateUpdate(deltaTime);

	// 状態ごとにアニメーションをセット
	if(prevState_ != currentState_) {
		ChangeAnimation(currentState_);
	}
	// スプライトの更新
	Character2D::Update(deltaTime);
	// 状態保存
	prevState_ = currentState_;
}

///////////////////////////////////////////////////////////////////////////////////////////
//	デバッグUIを表示
///////////////////////////////////////////////////////////////////////////////////////////
void Player2D::ShowGui() {
	bool isShow = true;
	ImGui::Begin("Player2D", &isShow);

	// パラメータセーブ
	if(ImGui::Button("SaveParams")) {
		SerializableObject::SaveParams();
	}

	ImGui::SameLine();

	// パラメータロード
	if(ImGui::Button("LoadParams")) {
		SerializableObject::LoadParams();
	}

	ImGui::Separator();
	// transform 表示
	ImGui::DragFloat2("position",&basePos_.x, 1.0f);
	ImGui::DragFloat2("scale",&size_.x, 0.1f);
	spriteObj_->SetScale(size_);
	
	ImGui::SeparatorText("material");
	// 色設定
	CalyxMath::Vector4 color = Character2D::GetSprite()->GetColor();
	if(ImGui::ColorEdit4("color", &color.x)) {
		Character2D::GetSprite()->SetColor(color);
	}

	ImGui::SeparatorText("state");
	// 状態を変える
	if(ImGui::Button("dirLeft")) {
		currentState_ = Player2dState::MoveL;
	}
	if(ImGui::Button("dirRight")) {
		currentState_ = Player2dState::MoveR;
	}
	if(ImGui::Button("attack")) {
		currentState_ = Player2dState::Attack;
	}

	// アニメーション
	ImGui::SeparatorText("Animator");
	animator_.ShowGui();

	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////////////////
//	スプライトアニメーションの初期化
///////////////////////////////////////////////////////////////////////////////////////////
void Player2D::InitializeSpriteAnimation() const {
	// アニメーション設定
	Calyx2D::SpriteAnimation anim;
	anim.texturePath   = "Textures/Player/flyingPlayer.dds";
	anim.division	   = {3, 2};
	anim.frameDuration = 0.1f;
	anim.loop		   = true;

	// アニメーション登録
	// 移動反転
	spriteObj_->AddAnimation("MoveInv", anim);
	// 通常移動
	anim.texturePath = "Textures/Player/flyingPlayer_inv.dds";
	spriteObj_->AddAnimation("Move", anim);
	// 攻撃
	anim.loop		 = false;
	anim.texturePath = "Textures/Player/attackPlayer.dds";
	spriteObj_->AddAnimation("Attack", anim);
	// 攻撃反転
	anim.texturePath = "Textures/Player/attackPlayer_inv.dds";
	spriteObj_->AddAnimation("AttackInv", anim);

	spriteObj_->SetAnimation("Move", true);
}

///////////////////////////////////////////////////////////////////////////////////////////
//	デバッグUIを表示
///////////////////////////////////////////////////////////////////////////////////////////
void Player2D::InitializeSerializableParm() {
	SerializableObject::AddField("amplitude", amplitude_);
	SerializableObject::AddField("attackCoolTime", attackCoolTime_);
	SerializableObject::AddField("size", size_ );
	SerializableObject::AddField("moveSpeed", moveSpeed_ );
	SerializableObject::AddField("moveRange", moveRange_ );
	

	SerializableObject::LoadParams();
}

///////////////////////////////////////////////////////////////////////////////////////////
//	stateからアニメーションを設定
///////////////////////////////////////////////////////////////////////////////////////////
void Player2D::ChangeAnimation(Player2dState state) const {
	switch(state) {

		{
		case Player2dState::MoveL:
			spriteObj_->SetAnimation("MoveInv", false);
			break;
		}

		{
		case Player2dState::MoveR:
			spriteObj_->SetAnimation("Move", false);
			break;
		}

		{
		case Player2dState::Attack:
			spriteObj_->SetAnimation("Attack", true);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//	移動処理
///////////////////////////////////////////////////////////////////////////////////////////
void Player2D::Move() {

	CalyxMath::Vector2 pos = basePos_;

	 if(animator_.Has<CalyxMath::Vector2>("SwayY")) {
	 	pos = pos + animator_.Get<CalyxMath::Vector2>("SwayY");
	 }

	SetPosition(pos);
}

///////////////////////////////////////////////////////////////////////////////////////////
//	移動処理
///////////////////////////////////////////////////////////////////////////////////////////
void Player2D::StateUpdate(float dt) {
	// X方向自動移動
	basePos_.x += moveSpeed_ * moveDir_ * dt;

	// 範囲制限 [0, 1280]
	if (basePos_.x >= kGameWidth) {
		basePos_.x = kGameWidth;
		moveDir_ = -1;
		currentState_ = Player2dState::MoveL;
	}
	else if (basePos_.x <= 0.0f) {
		basePos_.x = 0.0f;
		moveDir_ = 1;
		currentState_ = Player2dState::MoveR;
	}

	animator_.Update(dt);
	Move();
}