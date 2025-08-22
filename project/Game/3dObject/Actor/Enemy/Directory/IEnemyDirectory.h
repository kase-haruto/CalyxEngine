#pragma once
#include <list>
#include <memory>

class Enemy;

struct IEnemyDirectory {
	virtual ~IEnemyDirectory() = default;
	virtual void Clear() = 0;
	virtual void Register(const std::shared_ptr<Enemy>& e) = 0;
	virtual void Unregister(const Enemy* e) = 0;
	virtual std::list<std::shared_ptr<Enemy>> SnapshotAlive() = 0; // 生存のみ返す
};