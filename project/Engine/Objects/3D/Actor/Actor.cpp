#include "Actor.h"

#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Objects/Collider/CapsuleCollider.h>
#include <Engine/Objects/Collider/SphereCollider.h>
#include <externals/imgui/imgui.h>

Actor::Actor() {
	EnsureActorPhysicsBody();
	characterMovement_.SetOwner(this);
}

Actor::Actor(const std::string& modelName,
			 std::optional<std::string> objectName) :
	BaseGameObject::BaseGameObject(modelName, objectName) {
	EnsureActorPhysicsBody();
	characterMovement_.SetOwner(this);
}

void Actor::AlwaysUpdate(float dt) {
	
	BaseGameObject::AlwaysUpdate(dt);
}
void Actor::Update(float dt) {
	// Actor は CharacterMovementComponent で床探索・接地補正を行ってから、
	// BaseGameObject 側で Transform / Collider / Model を通常更新する。
	characterMovement_.Tick(dt);
	BaseGameObject::Update(dt);

}

void Actor::ApplyConfig() {
	// まずBaseGameObjectとしてモデル、コライダー、Transform、保存済みPhysicsBodyを復元する。
	BaseGameObject::ApplyConfig();

	// 保存データ側にphysicsBodyConfigが無い古いシーンでは、PhysicsBodyConfigのデフォルトでStaticに戻る。
	// Actorはキャラクター移動や押し戻しで自分の座標を補正する側なので、適用後に必ずKinematicへ戻す。
	EnsureActorPhysicsBody();
}

void Actor::ExtractConfig() {
	// 保存直前にActorの物理種別を保証する。
	// これにより、次回ロード時にもphysicsBodyConfig.bodyType=Kinematicとして復元できる。
	EnsureActorPhysicsBody();

	// BaseGameObjectConfigへ現在状態を抽出する。
	BaseGameObject::ExtractConfig();

	// Base側の抽出後にも念のため保存用ConfigをActor向けに固定する。
	// 将来Base側の抽出順が変わっても、Actorの保存値がStaticへ戻らないようにする。
	config_.GetConfig().physicsBodyConfig.bodyType = static_cast<int>(PhysicsBodyType::Kinematic);
}

void Actor::EnsureActorPhysicsBody() {
	// Actorは床探索、ジャンプ、押し戻しによってTransformを変更する可動オブジェクトとして扱う。
	physicsBody_.SetBodyType(PhysicsBodyType::Kinematic);

	// Config側も同時に更新して、Inspector表示とシーン保存値を実体のPhysicsBodyに揃える。
	config_.GetConfig().physicsBodyConfig.bodyType = static_cast<int>(PhysicsBodyType::Kinematic);
}

void Actor::DerivativeGui() {
	/*BaseGameObject::DerivativeGui();
	if(ImGui::TreeNodeEx("CharacterMovement", ImGuiTreeNodeFlags_SpanAvailWidth)) {
		characterMovement_.ShowGui();
		const FindFloorResult& floor = characterMovement_.GetCurrentFloor();
		ImGui::Text("Mode: %s", characterMovement_.IsMovingOnGround() ? "Walking" : "Falling");
		ImGui::Text("Floor Hit: %s", floor.blockingHit ? "true" : "false");
		ImGui::Text("Walkable: %s", floor.walkableFloor ? "true" : "false");
		ImGui::Text("Floor Distance: %.3f", floor.floorDistance);
		ImGui::TreePop();
	}*/
}

float Actor::GetCollisionRadius() const {
	if (!collider_) return 0.0f;
	if (auto s = dynamic_cast<const SphereCollider*>(collider_.get())) return s->GetColliderRadius();
	if (auto b = dynamic_cast<const BoxCollider*>(collider_.get())) {
		auto half = b->GetSize() * 0.5f;
		return std::sqrt(half.x * half.x + half.y * half.y + half.z * half.z);
	}
	return collider_->GetColliderRadius();
}

const CalyxEngine::Vector3 Actor::GetCenterPos() const {
	// Actorは「足元にTransform原点があるキャラクター」として扱う。
	// そのため、衝突中心はTransform位置にコライダーの半分の高さを足して求める。
	CalyxEngine::Vector3 center = worldTransform_.GetWorldPosition();

	if(!collider_) {
		return center;
	}

	if(auto capsule = dynamic_cast<const CapsuleCollider*>(collider_.get())) {
		// カプセルは全体高さの中央が衝突中心になる。
		center.y += capsule->GetHeight() * 0.5f;
		return center;
	}

	if(auto sphere = dynamic_cast<const SphereCollider*>(collider_.get())) {
		// 球は足元から半径分だけ上げた位置を中心にする。
		center.y += sphere->GetColliderRadius();
		return center;
	}

	if(auto box = dynamic_cast<const BoxCollider*>(collider_.get())) {
		// Boxは足元から高さの半分だけ上げた位置を中心にする。
		center.y += box->GetSize().y * 0.5f;
		return center;
	}

	return center;
}
