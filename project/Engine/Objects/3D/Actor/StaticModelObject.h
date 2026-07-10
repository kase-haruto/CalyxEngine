#pragma once

#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>

/*-----------------------------------------------------------------------------------------
 * StaticModelObject
 * - エディタで配置可能な汎用静的モデルオブジェクト
 * - モデル表示に必要なComponent構成を管理する
 * - ゲーム固有のAIやスポーン処理は管理しない
 *---------------------------------------------------------------------------------------*/
CALYX_OBJECT(Category = GameObject, DisplayName = "Mesh Object", Icon = "UI/Tool/cube.dds")
class CALYX_API StaticModelObject : public BaseGameObject {
public:
	StaticModelObject();
	StaticModelObject(const std::string& modelName, std::optional<std::string> objectName = std::nullopt);
	~StaticModelObject() override = default;

	std::string_view GetObjectClassName() const override { return "StaticModelObject"; }
};
