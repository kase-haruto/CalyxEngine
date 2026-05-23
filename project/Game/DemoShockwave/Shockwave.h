#pragma once
#include "Engine/Objects/3D/Actor/Actor.h"
#include "Engine\Foundation\Serialization\SerializableObject.h"

class Shockwave : public Actor {
public:
	Shockwave();
	Shockwave(const std::string&		 modelName,
			  std::optional<std::string> objectName);
	~Shockwave() override = default;

public:
	void Initialize() override;
	void Update(float dt) override;

	/// <summary> 衝撃波を発生させる </summary>
	/// <param name="pos"> 位置 </param>
	/// <param name="scaleMultiplier"> 拡大の倍率 </param>
	void Activate(const CalyxEngine::Vector3& pos, float scaleMultiplier);

	/// <summary> 停止処理 </summary>
	void Deactivate();

	//--------- ui/gui --------------------------------------------------

	void DerivativeGui() override;

	//--------- Collision -----------------------------------------------

	void OnCollisionEnter(Collider* other) override;

	//--------- accessor ------------------------------------------------
	std::string_view GetObjectClassName() const override { return "Shockwave"; }
	float			 GetPushForce() const { return param_.pushForce * scaleMultiplier_; }
	bool			 IsActive() const { return isActive_; }
	bool			 IsTakeDamageForStage();

private:
	struct ShockwaveParameter : public CalyxEngine::SerializableObject {
		float lifeTime	 = 0.5f;
		float startScale = 1.0f;
		float endScale	 = 4.0f;
		float pushForce	 = 20.0f; // 跳ね返す力

		ShockwaveParameter() {
			AddField("Life Time", lifeTime);
			AddField("Start Scale", startScale);
			AddField("End Scale", endScale);
			AddField("Push Force", pushForce);
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "Shockwave", "Shockwave"};
		}
	};

	ShockwaveParameter param_;
	float			   scaleMultiplier_ = 1.0f;
	float			   timer_			= 0.0f;
	float			   currentMaxScale_ = 5.0f;
	bool			   isActive_			 = false;
	bool			   isTakeDamageForStage_ = false;
};
