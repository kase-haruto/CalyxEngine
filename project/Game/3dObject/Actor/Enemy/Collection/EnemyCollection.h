#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Engine/objects/ConfigurableObject/IConfigurable.h>
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
class SceneContext;

class EnemyCollection :
	public SceneObject,
	public IConfigurable{
public:
	//===================================================================*/
	//                      Public Methods
	//===================================================================*/
	EnemyCollection(const std::string& name = "EnemyCollection");
	~EnemyCollection() = default;

	void Initialize()override;
	void Update(float dt) override;
	void AlwaysUpdate(float dt)override;
	void ShowGui() override;

	void SetPlayerTransform(WorldTransform* pTransform);

	void AddEnemy(const std::shared_ptr<Enemy>& enemy);
	void AddSpawner(const std::shared_ptr<class EnemySpawner>& spawner);
	void CreateSpawners();
	void Clear();

	//--------- config ------------------------------------------------
	virtual void ApplyConfig();
	virtual void ExtractConfig();
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	const std::list<std::shared_ptr<Enemy>>& GetEnemies() const{ return enemies_; }

	int GetDeadEnemyCount() const{ return deadEnemyCount; }
	std::string_view GetTypeName() const override{ return "EnemyCollection"; }
private:
	//===================================================================*/
	//                      Private variables
	//===================================================================*/
	std::list<std::shared_ptr<Enemy>> enemies_;
	std::vector<std::shared_ptr<class EnemySpawner>> spawners_;
	int deadEnemyCount = 0;
	

	ConfigurableObject<SceneObjectConfig> config_;
};

