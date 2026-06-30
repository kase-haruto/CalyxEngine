#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>

#include "Engine/Foundation/Serialization/SerializableObject.h"

#include <Engine/Foundation/Math/Vector3.h>

class BaseGameObject;
class Collider;

/*-----------------------------------------------------------------------------------------
 * CharacterMovementMode
 * - Unreal Engine の MovementMode に寄せたキャラクター移動状態
 * - まずは歩行と落下だけを扱い、後から Swimming / Flying などを追加できる形にする
 *---------------------------------------------------------------------------------------*/
enum class CharacterMovementMode {
	Walking,
	Falling,
};

/*-----------------------------------------------------------------------------------------
 * FindFloorResult
 * - CharacterMovementComponent が床探索で得た結果
 * - Unreal の FFindFloorResult に寄せ、床として使えるかと距離を分けて持つ
 *---------------------------------------------------------------------------------------*/
struct CALYX_API FindFloorResult {
	bool blockingHit = false;		   //< 何らかの地形に当たったか
	bool walkableFloor = false;		   //< 歩行可能な床として扱えるか
	float floorDistance = 0.0f;		   //< カプセル底面から床までの距離
	CalyxEngine::Vector3 hitPoint{};   //< 床として見つかった点
	CalyxEngine::Vector3 hitNormal{};  //< 床の法線
	Collider* hitCollider = nullptr;   //< 見つかった床コライダー
};

/*-----------------------------------------------------------------------------------------
 * CharacterMovementComponent
 * - キャラクター専用の移動補助コンポーネント
 * - 床探索、接地状態、重力、床へのスナップを担当する
 *---------------------------------------------------------------------------------------*/
class CALYX_API CharacterMovementComponent {
public:
	CharacterMovementComponent();
	/**
	 * \brief 所有者を設定する
	 * \param owner キャラクターとして扱うオブジェクト
	 */
	void SetOwner(BaseGameObject* owner) { owner_ = owner; }

	/**
	 * \brief 毎フレームの移動補助を更新する
	 * \param dt デルタタイム
	 */
	void Tick(float dt);

	/**
	 * \brief 水平移動入力を追加する
	 * \param worldDirection ワールド空間の移動方向
	 * \param scale 入力量
	 */
	void AddMovementInput(const CalyxEngine::Vector3& worldDirection, float scale = 1.0f);

	/**
	 * \brief ジャンプ要求を行う
	 */
	void Jump();

	/**
	 * \brief 現在位置から床を探索する
	 * \param outFloor 探索結果
	 */
	void FindFloor(FindFloorResult& outFloor) const;

	/**
	 * \brief デバッグGUIを表示する
	 */
	void ShowGui();

	/**
	 * \brief 現在の移動モードを取得する
	 * \return 移動モード
	 */
	CharacterMovementMode GetMovementMode() const { return movementMode_; }

	/**
	 * \brief 接地中かを取得する
	 * \return 接地中なら true
	 */
	bool IsMovingOnGround() const { return movementMode_ == CharacterMovementMode::Walking; }

	/**
	 * \brief 現在の床探索結果を取得する
	 * \return 床探索結果
	 */
	const FindFloorResult& GetCurrentFloor() const { return currentFloor_; }


	//パラメータをセットする
	
	/**
	 * \brief 重力を設定
	 */
	void SetGravity(float gravity) { param_.gravity_ = gravity; }
	/**
	 * \brief 最大落下速度を設定
	 */
	void SetMaxFallSpeed(float maxFallSpeed) { param_.maxFallSpeed_ = maxFallSpeed; }
	/**
	 * \brief 歩行可能な最大斜面角度を設定
	 */
	void SetWalkableFloorAngle(float walkableFloorAngle) { param_.walkableFloorAngle_ = walkableFloorAngle; }
	/**
	 * \brief カプセル底面から床を探す距離を設定
	 */
	void SetFloorProbeDistance(float floorProbeDistance) { param_.floorProbeDistance_ = floorProbeDistance; }
	/**
	 * \brief 歩行中に床へ吸着する距離を設定
	 */
	void SetFloorSnapDistance(float floorSnapDistance) { param_.floorSnapDistance_ = floorSnapDistance; }
	/**
	 * \brief 床から少し浮かせる安全幅を設定
	 */
	void SetSkinWidth(float skinWidth) { param_.skinWidth_ = skinWidth; }
	/**
	 * \brief 最大歩行速度を設定
	 */
	void SetMaxWalkSpeed(float maxWalkSpeed) { param_.maxWalkSpeed_ = maxWalkSpeed; }
	/**
	 * \brief ジャンプ初速を設定
	 */
	void SetJumpVelocity(float jumpVelocity) { param_.jumpVelocity_ = jumpVelocity; }


private:
	/**
	 * \brief 所有者のカプセル寸法を取得する
	 * \param outRadius 半径
	 * \param outHalfHeight 半分の高さ
	 * \return 取得できたら true
	 */
	bool GetOwnerCapsule(float& outRadius, float& outHalfHeight) const;

	/**
	 * \brief 床法線が歩行可能かを判定する
	 * \param normal 床候補の法線
	 * \return 歩行可能なら true
	 */
	bool IsWalkable(const CalyxEngine::Vector3& normal) const;

	/**
	 * \brief 現在の床へ吸着する
	 */
	void SnapToFloor();

	/**
	 * \brief このフレームに適用したCharacter速度をデバッグラインで表示する
	 * \param characterVelocity 水平移動とジャンプ／落下を合成した速度
	 */
	void DrawMovementDebugLine(const CalyxEngine::Vector3& characterVelocity) const;

private:
	BaseGameObject* owner_ = nullptr;							 //< 所有者
	CharacterMovementMode movementMode_ = CharacterMovementMode::Falling; //< 現在の移動モード
	FindFloorResult currentFloor_{};								 //< 現在の床情報
	CalyxEngine::Vector3 velocity_{};								 //< CharacterMovement が管理する縦方向速度


	CalyxEngine::Vector3 pendingInput_{};  //< 次のTickで消費する水平移動入力

	struct CharacterMovementDefautlParam :CalyxEngine::SerializableObject{
		CharacterMovementDefautlParam() {
			AddField("gravity", gravity_).Category("Movement").Tooltip("落下時の重力加速度");
			AddField("maxFallSpeed", maxFallSpeed_).Category("Movement").Tooltip("最大落下速度");
			AddField("walkableFloorAngle", walkableFloorAngle_).Category("Movement").Tooltip("歩行可能な最大斜面角度");
			AddField("floorProbeDistance", floorProbeDistance_).Category("Movement").Tooltip("カプセル底面から床を探す距離");
			AddField("floorSnapDistance", floorSnapDistance_).Category("Movement").Tooltip("歩行中に床へ吸着する距離");
			AddField("skinWidth", skinWidth_).Category("Movement").Tooltip("床から少し浮かせる安全幅");
			AddField("maxWalkSpeed", maxWalkSpeed_).Category("Movement").Tooltip("最大歩行速度");
			AddField("jumpVelocity", jumpVelocity_).Category("Movement").Tooltip("ジャンプ初速");
			AddField("showMovementDebugLine", showMovementDebugLine_).Category("Debug").Tooltip("坂、ジャンプ、落下を含むCharacter速度をデバッグラインで表示する");
			AddField("movementDebugLineScale", movementDebugLineScale_).Category("Debug").Tooltip("Character速度をデバッグライン長へ変換する倍率");
		}
		CalyxEngine::ParamPath GetParamPath() const{
			return {CalyxEngine::ParamDomain::Game,"CharacterMovementComponent","Actor"};
		}

		float gravity_ = 9.8f;				  //< 落下時の重力加速度
		float maxFallSpeed_ = 50.0f;		  //< 最大落下速度
		float walkableFloorAngle_ = 45.0f;	  //< 歩行可能な最大斜面角度
		float floorProbeDistance_ = 0.35f;	  //< カプセル底面から床を探す距離
		float floorSnapDistance_ = 0.2f;	  //< 歩行中に床へ吸着する距離
		float skinWidth_ = 0.01f;			  //< 床から少し浮かせる安全幅
		float maxWalkSpeed_ = 6.0f;			  //< 最大歩行速度
		float jumpVelocity_ = 6.5f;			  //< ジャンプ初速
		bool showMovementDebugLine_ = true;	 //< Character速度デバッグラインを表示するか
		float movementDebugLineScale_ = 1.0f; //< Character速度をデバッグライン長へ変換する倍率
	}param_;
};
