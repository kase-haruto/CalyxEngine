#include "EnemyDirectory.h"
#include <Game/3dObject/Actor/Enemy/Enemy.h>

void EnemyDirectory::Clear() {
	std::scoped_lock lk(mtx_);
	enemies_.clear();
	cachedSnapshot_.clear();
	isDirty_ = true;
}

void EnemyDirectory::Register(const std::shared_ptr<Enemy>& e) {
	std::scoped_lock lk(mtx_);
	enemies_.push_back(e);
	isDirty_ = true;
}

void EnemyDirectory::Unregister(const Enemy* e) {
	std::scoped_lock lk(mtx_);
	enemies_.remove_if([&](const std::weak_ptr<Enemy>& w){
		auto sp = w.lock();
		return !sp || sp.get() == e;
	});
	isDirty_ = true;
}

const std::vector<std::shared_ptr<Enemy>>& EnemyDirectory::SnapshotAlive() {
	std::scoped_lock lk(mtx_);

	if (!isDirty_) {
		// キャッシュ内生存チェック
		bool anyDead = false;
		for(auto& e : cachedSnapshot_) {
			if(!e || !e->GetIsAlive()) {
				anyDead = true;
				break;
			}
		}
		if(!anyDead) return cachedSnapshot_;
	}

	cachedSnapshot_.clear();
	cachedSnapshot_.reserve(enemies_.size());

	for (auto it = enemies_.begin(); it != enemies_.end(); ) {
		if (auto sp = it->lock()) {
			if (sp->GetIsAlive()) { 
				cachedSnapshot_.push_back(sp); 
				++it; 
			}
			else { 
				it = enemies_.erase(it); 
				isDirty_ = true; // 構造変わった
			}
		} else {
			it = enemies_.erase(it);
			isDirty_ = true;
		}
	}
	
	isDirty_ = false;
	return cachedSnapshot_;
}