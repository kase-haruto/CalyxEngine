#pragma once
#include "Engine/Objects/3D/Actor/Actor.h"
#include "Engine\Foundation\Serialization\SerializableObject.h"

class DemoHammer : public Actor {
public:
	DemoHammer();
	DemoHammer(const std::string&		  modelName,
			   std::optional<std::string> objectName);
	~DemoHammer() override = default;

public:
	void Initialize() override;
	void Update(float dt) override;

	//--------- ui/gui --------------------------------------------------

	void DerivativeGui() override;

	//--------- Collision -----------------------------------------------

	//--------- config ------------------------------------------------

	//--------- accessor ------------------------------------------------
	std::string_view GetObjectClassName() const override { return "DemoHammer"; }
	void			 SetSwingAngle(const CalyxEngine::Vector3& axis, float angle) {
		swingAxis_	= axis;
		swingAngle_ = angle;
	}


private:
	struct HammerParameter : public CalyxEngine::SerializableObject {
		float moveSpeed = 2.0f;

		HammerParameter() {
			AddField("Move Speed", moveSpeed).Category("Move Param");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "DemoHammer", "Hammer"};
		}
	};

	HammerParameter param_;

	CalyxEngine::Vector3 swingAxis_	 = {1.0f, 0.0f, 0.0f};
	float				 swingAngle_ = 0.0f;
};
