#pragma once

#include "Engine\Objects\3D\Actor\BaseGameObject.h"

/// <summary>
/// ギミックの実体オブジェクトの基底クラス
/// </summary>
class StageGimmickObjectBase : public BaseGameObject 
{
public:

	StageGimmickObjectBase() = default;
	StageGimmickObjectBase(const std::string& modelName,
		std::optional<std::string> objectName = std::nullopt);
	~StageGimmickObjectBase() override = default;

	void Initialize() override;
	void Update(float dt) override;

	const CalyxEngine::Vector3 GetCenterPos() const override;
	
	std::string_view GetObjectClassName() const override {
		return "StageGimmickObjectBase";
	}

protected:

	// ギミックの初期化
	virtual void ObjectInitialize() = 0;

	// ギミックの更新
	virtual void ObjectUpdate(float dt) = 0;

	// ギミックが作動しているか
	bool isActive_ = true;
};
