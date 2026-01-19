#pragma once
#include "IEnemyDirectory.h"

#include <list>
#include <memory>
#include <mutex>
#include <vector>

class Enemy;

class EnemyDirectory final
	: public IEnemyDirectory {
public:
	void Clear() override;
	void Register(const std::shared_ptr<Enemy>& e) override;
	void Unregister(const Enemy* e) override;
	const std::vector<std::shared_ptr<Enemy>>& SnapshotAlive() override;
	bool IsDirty() const override { return isDirty_; }

private:
	std::mutex mtx_;
	std::list<std::weak_ptr<Enemy>> enemies_;
	
	// cache
	std::vector<std::shared_ptr<Enemy>> cachedSnapshot_;
	bool isDirty_ = true;
};