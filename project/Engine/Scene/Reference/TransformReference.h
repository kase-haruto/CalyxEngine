#pragma once

#include <Engine/Scene/Reference/SceneObjectReference.h>

namespace CalyxEngine {

	/**
	 * \brief SceneObjectが所有するWorldTransformへの、永続化可能な読み取り専用参照。
	 *
	 * WorldTransform自体は独立した寿命やGUIDを持たない。そのためWorldTransform*を
	 * フィールドへ直接保存せず、所有元SceneObjectのGUIDをSceneObjectRefで保持する。
	 * これによりシーン保存、Prefab複製、対象削除を既存のSceneObject参照基盤で扱える。
	 *
	 * 外部にはSceneObjectを返さずconst WorldTransformだけを公開する。追跡や注視など、
	 * 位置を読むだけの利用側がSceneObjectの変更APIへ依存することを防ぐためである。
	 */
	class CALYX_API TransformRef final : public ISceneObjectReference {
	public:
		TransformRef() = default;
		explicit TransformRef(const Guid& ownerGuid) : owner_(ownerGuid) {}

		/**
		 * \brief 現在のSceneContextから所有元を解決し、読み取り専用Transformを返す。
		 * \return 未設定、別シーン、削除済みの場合はnullptr。
		 *
		 * 対象SceneObjectは参照後に削除され得るため、戻り値を長期間キャッシュせず、
		 * 利用するフレームでResolveし直すこと。
		 */
		const WorldTransform* Resolve() const {
			// 追跡・注視用途の大半は座標参照だけで足りるため、変更可能ポインタは提供しない。
			// 書き込み責務を持つ具体的な機能が現れるまでは、ResolveMutableを追加しない方針とする。
			const auto* resolver = GetCurrentSceneObjectResolver();
			return resolver ? Resolve(*resolver) : nullptr;
		}

		/** テストや独立したシーン処理ではResolverを明示的に注入できる。 */
		const WorldTransform* Resolve(const ISceneObjectResolver& resolver) const {
			// 内部では所有元SceneObjectを安全に解決するが、SceneObject自体は呼び出し側へ公開しない。
			auto owner = owner_.Resolve(resolver);
			return owner ? &owner->GetWorldTransform() : nullptr;
		}

		const Guid& GetGuid() const override { return owner_.GetGuid(); }
		bool IsAssigned() const { return owner_.IsAssigned(); }

		void SetGuid(const Guid& guid) override {
			// 永続化するのはTransformポインタではなく、その所有元SceneObjectのGUIDである。
			owner_.SetGuid(guid);
		}

		void Clear() override { owner_.Clear(); }

		bool CanAssign(const SceneObject& object) const override {
			// 現在は全SceneObjectがWorldTransformを必ず所有するため、任意のSceneObjectを割り当てられる。
			return owner_.CanAssign(object);
		}

		std::optional<std::string> ResolveDisplayName(const ISceneObjectResolver& resolver) const override {
			// Editorへは表示名だけを提供し、SceneObjectポインタがTransformRef APIから漏れないようにする。
			return owner_.ResolveDisplayName(resolver);
		}

		std::string_view GetReferenceKindName() const override { return "Transform"; }

		void Remap(const std::unordered_map<Guid, Guid>& guidMap) override {
			// Prefab内の相互参照が複製元を指し続けないよう、所有元GUIDを複製先GUIDへ置換する。
			owner_.Remap(guidMap);
		}

	private:
		// WorldTransformは独立したIDを持たないため、現段階では所有元SceneObjectの参照を保存する。
		// 将来Component化する場合はobjectGuidとcomponentGuidを持つComponentRef<T>へ拡張できる。
		SceneObjectRef<SceneObject> owner_;
	};

	/** TransformRefもSceneObject参照と同じく、所有元GUIDだけをJSONへ保存する。 */
	inline void to_json(nlohmann::json& json, const TransformRef& reference) {
		json = reference.GetGuid();
	}

	/** 読み込み時はGUIDだけを復元し、Transformの解決は利用時まで遅延する。 */
	inline void from_json(const nlohmann::json& json, TransformRef& reference) {
		reference.SetGuid(json.get<Guid>());
	}

	/**
	 * \brief 将来Component参照を導入する際の永続アドレス設計。
	 *
	 * 同一SceneObjectに同型Componentが複数存在し得るため、型名だけでは一意にならない。
	 * そのため所有Object GUIDに加えてComponent固有GUIDを保存する必要がある。
	 */
	struct ComponentAddress {
		Guid objectGuid;
		Guid componentGuid;
	};

} // namespace CalyxEngine
