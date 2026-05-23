#include "StageGimmickEventBase.h"

StageGimmickEventBase::StageGimmickEventBase(const std::string& name) : BaseEventObject(name) {}

void StageGimmickEventBase::Initialize() {

	// イベントオブジェの初期化
	BaseEventObject::Initialize();

	worldTransform_.scale = {10.0f, 5.0f, 1.0f};
}

void StageGimmickEventBase::AlwaysUpdate(float dt) {

	// イベントオブジェの更新
	BaseEventObject::AlwaysUpdate(dt);

	// アクティブが無ければ更新止める
	if(!isActive_) { return; }

	// イベントの更新
	EventUpdate(dt);
}

void StageGimmickEventBase::RemapGuid(Guid& guid,
									  const std::unordered_map<Guid, Guid>& guidMap) {
	if(!guid.isValid()) return;
	if(auto it = guidMap.find(guid); it != guidMap.end()) {
		guid = it->second;
	}
}

void StageGimmickEventBase::RemapGuids(std::vector<Guid>& guids,
									   const std::unordered_map<Guid, Guid>& guidMap) {
	for(auto& guid : guids) {
		RemapGuid(guid, guidMap);
	}
}
