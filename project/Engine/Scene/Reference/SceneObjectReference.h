#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

/**
 * \brief GUIDからSceneObjectを取得するための最小インターフェース。
 *
 * 参照型をSceneContextやSceneObjectLibraryの具象型へ依存させないための境界である。
 * テストではこのインターフェースだけを実装したResolverへ差し替えられる。
 */
class CALYX_API ISceneObjectResolver {
public:
	virtual ~ISceneObjectResolver() = default;
	virtual std::shared_ptr<SceneObject> ResolveSceneObject(const Guid& guid) const = 0;
};

namespace CalyxEngine {

	/**
	 * \brief Inspectorとシリアライザが型に依存せず参照を操作するためのインターフェース。
	 */
	class CALYX_API ISceneObjectReference {
	public:
		virtual ~ISceneObjectReference() = default;

		virtual const Guid& GetGuid() const = 0;
		virtual void SetGuid(const Guid& guid) = 0;
		virtual void Clear() = 0;
		virtual bool CanAssign(const SceneObject& object) const = 0;
		virtual std::optional<std::string> ResolveDisplayName(const ISceneObjectResolver& resolver) const = 0;
		virtual std::string_view GetReferenceKindName() const = 0;
		virtual void Remap(const std::unordered_map<Guid, Guid>& guidMap) = 0;
	};

	/**
	 * \brief SceneContextで現在有効なSceneObjectLibraryを使う標準Resolverを取得する。
	 * \return SceneContextが存在しない場合はnullptr。
	 */
	CALYX_API const ISceneObjectResolver* GetCurrentSceneObjectResolver();

	/**
	 * \brief SceneObjectへの非所有・型付き参照。
	 *
	 * 保存対象はGUIDだけであり、実行時ポインタはweak_ptrとしてキャッシュする。
	 * そのため参照先の寿命を延長せず、削除後も安全にnullptrへ解決される。
	 */
	template<class T>
	/**
	 * @brief SceneObjectRefの機能を提供するクラスです。
	 */
	class SceneObjectRef final : public ISceneObjectReference {
		static_assert(std::is_base_of_v<SceneObject, T>, "T must derive from SceneObject");

	public:
		SceneObjectRef() = default;
		explicit SceneObjectRef(const Guid& guid) : guid_(guid) {}

		const Guid& GetGuid() const override { return guid_; }
		bool IsAssigned() const { return guid_.isValid(); }

		void SetGuid(const Guid& guid) override {
			// GUIDが変わった時点で旧オブジェクトのキャッシュを破棄する。
			guid_ = guid;
			cache_.reset();
		}

		void Set(const std::shared_ptr<T>& object) {
			// nullptr代入は参照解除として扱い、無効GUIDへ戻す。
			guid_ = object ? object->GetGuid() : Guid::Empty();
			cache_ = object;
		}

		void Clear() override {
			// 永続GUIDと実行時キャッシュを同時に消し、不整合を残さない。
			guid_ = Guid::Empty();
			cache_.reset();
		}

		bool CanAssign(const SceneObject& object) const override {
			// Inspectorから異なる型が渡された場合は、保存前に拒否する。
			return dynamic_cast<const T*>(&object) != nullptr;
		}

		std::shared_ptr<T> Resolve(const ISceneObjectResolver& resolver) const {
			// 未設定参照はResolverへ問い合わせず、即座にnullptrを返す。
			if(!guid_.isValid()) {
				cache_.reset();
				return nullptr;
			}

			// シーンから削除済みかを正しく判定するため、weakキャッシュが生存していても
			// Resolverで現在のシーン所属を毎回確認する。別のshared_ptrが削除済みObjectを
			// 延命している場合に、古いキャッシュを有効なシーン参照として返さないためである。
			// SceneObjectLibraryのGUID検索はunordered_mapによるO(1)なので、正しさを優先する。
			auto resolved = std::dynamic_pointer_cast<T>(resolver.ResolveSceneObject(guid_));
			cache_ = resolved;
			return resolved;
		}

		std::shared_ptr<T> Resolve() const {
			// 通常利用では現在のSceneContextから標準Resolverを取得する。
			const auto* resolver = GetCurrentSceneObjectResolver();
			return resolver ? Resolve(*resolver) : nullptr;
		}

		std::optional<std::string> ResolveDisplayName(const ISceneObjectResolver& resolver) const override {
			// Editorには対象名だけを返し、型消去されたSceneObjectポインタを公開しない。
			if(auto resolved = Resolve(resolver)) {
				return resolved->GetDisplayName();
			}
			return std::nullopt;
		}

		std::string_view GetReferenceKindName() const override { return "Scene Object"; }

		void Remap(const std::unordered_map<Guid, Guid>& guidMap) override {
			// Prefab複製元GUIDに対応する複製先GUIDがあれば参照を付け替える。
			if(const auto it = guidMap.find(guid_); it != guidMap.end()) {
				SetGuid(it->second);
			}
		}

	private:
		Guid guid_ = Guid::Empty();
		mutable std::weak_ptr<T> cache_;
	};

	/** GUIDだけをJSONへ保存し、メモリアドレスやキャッシュは保存しない。 */
	template<class T>
	void to_json(nlohmann::json& json, const SceneObjectRef<T>& reference) {
		json = reference.GetGuid();
	}

	/** JSONからGUIDを復元し、実行時ポインタは必要になった時に遅延解決する。 */
	template<class T>
	void from_json(const nlohmann::json& json, SceneObjectRef<T>& reference) {
		reference.SetGuid(json.get<Guid>());
	}

} // namespace CalyxEngine
