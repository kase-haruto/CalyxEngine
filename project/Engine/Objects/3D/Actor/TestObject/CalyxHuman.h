#pragma once
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>

class CalyxHuman :
	public Actor{
public:
	CalyxHuman(const std::string& modelName,
			   std::optional<std::string> objectName = std::nullopt);
	CalyxHuman();
	~CalyxHuman()override = default;

	void Initialize()override;
	void Update()override;

	std::optional<Vector3> GetJointWorldPos(const std::string& name) const;

private:
	void TransitionAnimation();
	void Move(float dt);
	void Turn();
private:
	std::shared_ptr<ParticleSystemObject> trailFx_;
};