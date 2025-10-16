#pragma once
#include <memory>
#include <cstddef>

class SceneContext;
class EnemyDirectory;
class EnemyBulletContainer;
class Player;

class EnemyRuntimeBindingService {
public:
	//===================================================================*/
	//		 public methods
	//===================================================================*/
	EnemyRuntimeBindingService() = default;

	void OnSceneLoaded(SceneContext& ctx,EnemyBulletContainer* bulletContainer);
	void Update(SceneContext& ctx, float dt);
	void OnSceneCleared(SceneContext& ctx);
	std::shared_ptr<EnemyDirectory> GetDirectory() const { return dir_; }

private:
	//===================================================================*/
	//		 private methods
	//===================================================================*/
	void WireAllSpawners(SceneContext& ctx);

private:
	//===================================================================*/
	//		 private methods
	//===================================================================*/
	std::shared_ptr<EnemyDirectory> dir_;
	std::weak_ptr<Player> wPlayer_;
	EnemyBulletContainer* bulletContainer_;
	size_t lastSpawnerCount_ = 0;

};