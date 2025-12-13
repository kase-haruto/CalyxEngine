#include "PlayerUpdaterCollection.h"

void PlayerUpdaterCollection::AddCommandUpdater(std::unique_ptr<IPlayerUpdater> updater) {
	commandUpdaters_.push_back(std::move(updater));
}

void PlayerUpdaterCollection::AddTickUpdater(std::unique_ptr<IPlayerTickUpdater> updater) {
	tickUpdaters_.push_back(std::move(updater));
}

void PlayerUpdaterCollection::CommandUpdate(const std::vector<PlayerCommand>& commands, float dt) const{
	for(const auto& cmd : commands) {
		for(auto& updater : commandUpdaters_) {
			updater->Update(cmd, dt);
		}
	}
}

void PlayerUpdaterCollection::TickUpdate(float dt)const {
	for(auto& updater : tickUpdaters_) {
		updater->Update(dt);
	}
}
void PlayerUpdaterCollection::Clear() {
	commandUpdaters_.clear();
	tickUpdaters_.clear();
}