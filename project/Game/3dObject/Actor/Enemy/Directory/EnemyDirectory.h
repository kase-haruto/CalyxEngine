#pragma once
#include "IEnemyDirectory.h"
#include <list>
#include <memory>
#include <mutex>

class Enemy;

class EnemyDirectory final
	: public IEnemyDirectory {
public:
	void Clear() override;
	void Register(const std::shared_ptr<Enemy>& e) override;
	void Unregister(const Enemy* e) override;
	std::list<std::shared_ptr<Enemy>> SnapshotAlive() override;

private:
	std::mutex mtx_;
	std::list<std::weak_ptr<Enemy>> enemies_;
};