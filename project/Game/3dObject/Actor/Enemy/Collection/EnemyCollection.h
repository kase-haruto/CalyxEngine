#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Enemy/Enemy.h>

class SceneContext;

class EnemyCollection :
	public SceneObject{
public:
	//===================================================================*/
	//                      Public Methods
	//===================================================================*/
	EnemyCollection(const std::string& name = "EnemyCollection");
	~EnemyCollection() = default;

	void Update() override;
	void ShowGui() override;

	void SetSceneContext(SceneContext* context);
	void SetPlayerTransform(WorldTransform* pTransform);

	void AddEnemy(const std::shared_ptr<Enemy>& enemy);
	void AddSpawner(const std::shared_ptr<class EnemySpawner>& spawner);
	void CreateSpawners();
	void Clear();

	const std::list<std::shared_ptr<Enemy>>& GetEnemies() const{ return enemies_; }

	int GetDeadEnemyCount() const{ return deadEnemyCount; }
private:
	//===================================================================*/
	//                      Private variables
	//===================================================================*/
	std::list<std::shared_ptr<Enemy>> enemies_;
	std::vector<std::shared_ptr<class EnemySpawner>> spawners_;
	int deadEnemyCount = 0;

	SceneContext* sceneContext_ = nullptr;
	WorldTransform* playerTransform_ = nullptr;
};

