#include "DemoPlayer.h"

#include <Engine/Scene/Context/SceneContext.h>

/////////////////////////////////////////////////////////////////////////////////////////
//			ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
DemoPlayer::DemoPlayer()
	: Actor("PlayerIdle.gltf", "DemoPlayer") {
}

void DemoPlayer::Initialize() {

	Actor::Initialize();
}
/////////////////////////////////////////////////////////////////////////////////////////
//			初期化
/////////////////////////////////////////////////////////////////////////////////////////
void DemoPlayer::Update(float dt) {
	// 入力デバイスの状態をDemoPlayer用の入力状態へ変換する。
	// DemoPlayer自身はキーコードやパッドボタンを直接知らない構成にしておく。
	input_.Update();

	// 入力状態を移動命令へ変換する。
	// 実際の床判定、重力、押し戻しはActorが持つCharacterMovementComponentに任せる。
	motor_.Update(*this, input_.GetState(), dt);

	// Actor本体の更新を行う。
	// CharacterMovementComponentのTickはActor::AlwaysUpdate側で実行されるため、
	// ここではDemoPlayer固有処理の後に通常更新だけを通す。
	Actor::Update(dt);
}
