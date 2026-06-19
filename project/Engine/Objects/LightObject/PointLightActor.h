#pragma once

// engine
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Objects/3D/Actor/Actor.h>
// c++
#include <memory>
#include <optional>
#include <string>

// Forward declaration
class PointLight;
/*-----------------------------------------------------------------------------------------
 * PointLightActor
 * - pointLightを所有しているオブジェクト
 * - meshを持つオブジェクトがポイントライトを使用したいときに使う
 *---------------------------------------------------------------------------------------*/
class PointLightActor
	: public Actor {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	PointLightActor();
	PointLightActor(const std::string& modelName, std::optional<std::string> objectName = std::nullopt);
	~PointLightActor() override;

	void Initialize() override;
	void AlwaysUpdate(float dt) override;
	void Destroy() override;
	void DerivativeGui() override;
	void SetDrawEnable(bool enable) override;
	void SetName(const std::string& name);

	void ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) override;
	void ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const override;

	std::string_view GetObjectClassName() const override { return "PointLightActor"; }

	PointLight* GetPointLight() const { return pointLight_.get(); }
	const std::shared_ptr<PointLight>& GetPointLightShared() const { return pointLight_; }

	void SetLightLocalPosition(const CalyxEngine::Vector3& localPosition);
	const CalyxEngine::Vector3& GetLightLocalPosition() const { return lightLocalPosition_; }

	void SetLightColor(const CalyxEngine::Vector4& color);
	void SetLightIntensity(float intensity);
	void SetLightRadius(float radius);
	void SetLightDecay(float decay);

private:
	//===================================================================*/
	//                    private methods / members
	//===================================================================*/
	void EnsurePointLight();
	void RegisterPointLight();
	void UnregisterPointLight();
	void SyncLightTransform();

	std::shared_ptr<PointLight> pointLight_;
	CalyxEngine::Vector3 lightLocalPosition_ = {};
};
