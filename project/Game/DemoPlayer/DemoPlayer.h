#pragma once
#include "Engine/Application/Effects/EffectAsset.h"
#include "Engine\Objects\3D\Actor\Actor.h"
#include "Engine\Foundation\Serialization\SerializableObject.h"

#include <Game\DemoHammer\DemoHammer.h>

class DemoPlayer :
public Actor {
public:
	DemoPlayer();
	DemoPlayer(const std::string&		  modelName,
			   std::optional<std::string> objectName);
	~DemoPlayer() override;

public:
	void Initialize() override;
	void Update(float dt) override;

	//--------- ui/gui --------------------------------------------------

	void DerivativeGui() override;

	//--------- Collision -----------------------------------------------

	void OnCollisionEnter(Collider* other) override;

	void TakeDamage(int32_t damage);

	//--------- config ------------------------------------------------

	//--------- accessor ------------------------------------------------
	std::string_view GetObjectClassName() const override { return "DemoPlayer"; }
private:
	struct JumpEvents {
		bool jumpStart = false;
		bool diveStart = false;
	};

	struct LandingEvents {
		bool landed = false;
		bool hardLanded = false;
	};

	void Move(float dt);
	JumpEvents HandleJump(float dt);
	LandingEvents ApplyGravity(float dt);
	void UpdatePopScale(float dt);
	void HammerControl(float dt);
	void DamageFlash(float dt);

private:

	struct PlayerParameter : public CalyxEngine::SerializableObject {
		int	  playerHP = 10;
		float moveSpeed = 2.0f;
		float jumpForce = 15.0f;
		float diveForce = -30.0f;
		float gravity	= 40.0f;

		float stiffness = 400.0f;
		float damping	= 20.0f;

		float diveRotationTime = 0.5f;

		float defaultShockScale = 1.0f;
		float strongShockScale = 1.5f;

		float damageFlashDuration = 1.0f;

		float variableJumpMultiplier = 2.0f;

		CalyxEngine::Vector3 colliderOffset = {0.0f, 0.4f, 0.0f};
		CalyxEngine::Vector3 colliderSize = {0.8f, 1.6f, 0.5f};

		CalyxEngine::Vector3 jumpScale = {0.8f, 1.3f, 0.8f};
		CalyxEngine::Vector3 diveScale = {0.9f, 1.2f, 0.9f};
		CalyxEngine::Vector3 landScale = {1.4f, 0.5f, 1.4f};

		CalyxEngine::Vector3 hammerSwingScale = {2.5f, 1.5f, 2.5f};

		struct ShockParm {
			float duration = 0.2f;
			float intensity = 0.5f;
		}shakeParm;

		PlayerParameter() {
			AddField("HP", playerHP).Category("Base Param").Tooltip("体力");
			AddField("Move Speed", moveSpeed).Category("Move Param").Tooltip("移動速度");
			AddField("Jump Force", jumpForce).Category("Move Param").Tooltip("ジャンプの初速");
			AddField("Dive Force", diveForce).Category("Move Param").Tooltip("ダイブの初速");
			AddField("Gravity", gravity).Category("Move Param").Tooltip("重力加速度");
			AddField("Stiffness", stiffness).Category("Pop Scale").Tooltip("ポップスケールの硬さ（大きいほど速く元に戻る）");
			AddField("Damping", damping).Category("Pop Scale").Tooltip("ポップスケールの減衰（大きいほど揺れが早く収まる）");
			AddField("Dive Rotation Time", diveRotationTime).Category("Move Param");
			AddField("Default ShockScale", defaultShockScale).Category("Shock Power");
			AddField("Strong ShockScale", strongShockScale).Category("Shock Power");
			AddField("Damage Flash Duration", damageFlashDuration).Category("Damage Param");
			AddField("Collider Offset", colliderOffset).Category("Collider Param");
			AddField("Collider Size", colliderSize).Category("Collider Param");
			AddField("Jump Scale", jumpScale).Category("Pop Scale");
			AddField("Dive Scale", diveScale).Category("Pop Scale");
			AddField("Land Scale", landScale).Category("Pop Scale");
			AddField("Hammer Swing Scale", hammerSwingScale).Category("Pop Scale");

			AddField("cameraShakeDuration",shakeParm.duration).Category("Shock Power").Tooltip("カメラシェイクの時間");
			AddField("cameraShakeIntensity",shakeParm.intensity).Category("Shock Power").Tooltip("カメラシェイクの強さ");
		}
		
		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "DemoPlayer", "Player"};
		}
	};

	PlayerParameter param_;
	bool  isJumping_	 = false;
	bool  isDiving_		 = false;
	CalyxEngine::Quaternion preRotate_;

	// ダメージ点滅用
	float damageFlashTimer_ = 0.0f;
	bool  isInvincible_	 = false;

	// ハンマーリカバリー用
	bool  isRecovering_	 = false;
	float recoveryTimer_ = 0.0f;
	float recoveryDuration_ = 0.3f; // リカバリー時間（秒）

	// 回転補間用
	float rotationSpeed_ = 10.0f;
	CalyxEngine::Quaternion baseRotation_ = CalyxEngine::Quaternion::MakeIdentity();

	// ジャンプ回転用
	float jumpRotation_			= 0.0f; // 現在の回転角
	float jumpRotationSpeed_	= 0.0f; // 現在の回転速度
	float jumpRotationRemaining_ = 0.0f; // 残りの回転量

	// Pop Scale
	CalyxEngine::Vector3 targetScale_ = {1.0f, 1.0f, 1.0f};
	CalyxEngine::Vector3 scaleVelocity_ = {0.0f, 0.0f, 0.0f};

	CalyxEngine::Vector3 hammerTargetScale_ = {1.0f, 1.0f, 1.0f};
	CalyxEngine::Vector3 hammerScaleVelocity_ = {0.0f, 0.0f, 0.0f};
	CalyxEngine::Vector3 hammerInitialScale_ = {1.0f, 1.0f, 1.0f};

	// ハンマー
	std::shared_ptr<DemoHammer> hammer_;
	bool						firstSetting_ = true;

	CalyxEngine::EffectAsset effectData;

};