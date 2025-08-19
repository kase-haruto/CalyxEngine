#include "EnemyDirectory.h"
#include <Game/3dObject/Actor/Enemy/Enemy.h>

void EnemyDirectory::Clear() {
	std::scoped_lock lk(mtx_);
	enemies_.clear();
}

void EnemyDirectory::Register(const std::shared_ptr<Enemy>& e) {
	std::scoped_lock lk(mtx_);
	enemies_.push_back(e);
}

void EnemyDirectory::Unregister(const Enemy* e) {
	std::scoped_lock lk(mtx_);
	enemies_.remove_if([&](const std::weak_ptr<Enemy>& w){
		auto sp = w.lock();
		return !sp || sp.get() == e;
	});
}

std::list<std::shared_ptr<Enemy>> EnemyDirectory::SnapshotAlive() {
	std::list<std::shared_ptr<Enemy>> out;
	std::scoped_lock lk(mtx_);
	for (auto it = enemies_.begin(); it != enemies_.end(); ) {
		if (auto sp = it->lock()) {
			if (sp->GetIsAlive()) { out.push_back(sp); ++it; }
			else { it = enemies_.erase(it); }
		} else {
			it = enemies_.erase(it);
		}
	}
	return out;
}