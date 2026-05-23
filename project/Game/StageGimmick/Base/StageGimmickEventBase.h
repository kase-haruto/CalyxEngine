#pragma once

#include "Engine\Objects\Event\BaseEventObject.h"

#include <Engine/Scene/Context/SceneContext.h>

#include <algorithm>
#include <string_view>
#include <unordered_map>
#include <vector>

/// <summary>
/// ギミックを発生させるイベントの基底クラス
/// </summary>
class StageGimmickEventBase : public BaseEventObject 
{
public:

	StageGimmickEventBase() = default;
	StageGimmickEventBase(const std::string& name);
	~StageGimmickEventBase() override = default;

	void Initialize() override;
	void AlwaysUpdate(float dt) override;

	std::string_view GetObjectClassName() const override {
		return "StageGimmickEventBase";
	}

	uint32_t GetObjectCount() const { return objectCount_; }

protected:
	template <class TObject>
	std::shared_ptr<TObject> ResolveLinkedObject(const Guid& guid,
												 std::string_view expectedClassName) const;

	template <class TObject>
	std::vector<std::shared_ptr<TObject>> ResolveLinkedObjects(
		const std::vector<Guid>& guids,
		std::string_view expectedClassName) const;

	template <class TObject>
	std::shared_ptr<TObject> FindOwnedObjectByClassName(std::string_view expectedClassName) const;

	template <class TObject>
	std::vector<std::shared_ptr<TObject>> FindOwnedObjectsByClassName(std::string_view expectedClassName) const;

	template <class TObject>
	std::vector<std::shared_ptr<TObject>> FindChildrenByClassName(
		const SceneObject& parent,
		std::string_view expectedClassName) const;

	static void RemapGuid(Guid& guid, const std::unordered_map<Guid, Guid>& guidMap);
	static void RemapGuids(std::vector<Guid>& guids, const std::unordered_map<Guid, Guid>& guidMap);
	
	// ギミックの初期化
	virtual void EventInitialize() = 0;

	// ギミックの更新
	virtual void EventUpdate(float dt) = 0;

	// ギミックが作動しているか
	bool isActive_ = true;

	// ターゲットのギミックオブジェクトの数
	uint32_t objectCount_ = 1;

};

template <class TObject>
std::shared_ptr<TObject> StageGimmickEventBase::ResolveLinkedObject(
	const Guid& guid,
	std::string_view expectedClassName) const {
	if(!guid.isValid()) return nullptr;

	auto* ctx = SceneContext::Current();
	if(!ctx || !ctx->GetObjectLibrary()) return nullptr;

	auto object = ctx->GetObjectLibrary()->Find(guid);
	if(!object || object->GetObjectClassName() != expectedClassName) return nullptr;
	return std::static_pointer_cast<TObject>(object);
}

template <class TObject>
std::vector<std::shared_ptr<TObject>> StageGimmickEventBase::ResolveLinkedObjects(
	const std::vector<Guid>& guids,
	std::string_view expectedClassName) const {
	std::vector<std::shared_ptr<TObject>> result;
	result.reserve(guids.size());
	for(const auto& guid : guids) {
		if(auto object = ResolveLinkedObject<TObject>(guid, expectedClassName)) {
			result.push_back(object);
		}
	}
	return result;
}

template <class TObject>
std::shared_ptr<TObject> StageGimmickEventBase::FindOwnedObjectByClassName(
	std::string_view expectedClassName) const {
	for(const auto& child : GetChildren()) {
		if(child && child->GetObjectClassName() == expectedClassName) {
			return std::static_pointer_cast<TObject>(child);
		}
	}
	return nullptr;
}

template <class TObject>
std::vector<std::shared_ptr<TObject>> StageGimmickEventBase::FindOwnedObjectsByClassName(
	std::string_view expectedClassName) const {
	return FindChildrenByClassName<TObject>(*this, expectedClassName);
}

template <class TObject>
std::vector<std::shared_ptr<TObject>> StageGimmickEventBase::FindChildrenByClassName(
	const SceneObject& parent,
	std::string_view expectedClassName) const {
	std::vector<std::shared_ptr<TObject>> result;
	for(const auto& child : parent.GetChildren()) {
		if(child && child->GetObjectClassName() == expectedClassName) {
			result.push_back(std::static_pointer_cast<TObject>(child));
		}
	}

	std::sort(result.begin(), result.end(),
			  [](const std::shared_ptr<TObject>& lhs,
				 const std::shared_ptr<TObject>& rhs) {
				  return lhs->GetGuid().ToString() < rhs->GetGuid().ToString();
			  });
	return result;
}
