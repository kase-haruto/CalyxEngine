#pragma once
#include <Engine/Objects/3D/Actor/Actor.h>

class CalyxHuman :
	public Actor{
public:
	CalyxHuman(const std::string& modelName,
			   std::optional<std::string> objectName = std::nullopt);
	~CalyxHuman()override = default;

	void Initialize()override;
	void Update()override;

private:
	void TransitionAnimation();
	void Move(float dt);
	void Turn();
private:
};

