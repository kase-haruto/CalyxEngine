#pragma once
#include <Engine/Objects/3D/Actor/Actor.h>

class CalyxHuman :
	public Actor{
public:
	CalyxHuman(const std::string& modelName,
			   std::optional<std::string> objectName = std::nullopt);
	CalyxHuman();
	~CalyxHuman()override = default;

	void Initialize()override;
	void Update(float dt)override;

	std::optional<CalyxEngine::Vector3> GetJointWorldPos(const std::string& name) const;
	std::string_view GetObjectClassName() const override{ return "CalyxHuman"; }
private:
	void TransitionAnimation();
	void Move(float dt);
	void Turn();
};