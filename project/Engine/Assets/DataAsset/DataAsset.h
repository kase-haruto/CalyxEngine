#pragma once

#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <string>
#include <memory>

namespace CalyxEngine {

	/**
	 * @brief 全データアセット（Material, Archetype等）の基底クラス
	 */
	class DataAsset : public SerializableObject {
	public:
		DataAsset() : guid_(Guid::New()), name_("New DataAsset") {}
		virtual ~DataAsset() = default;

		// --- Accessors ---
		const Guid& GetGuid() const { return guid_; }
		void SetGuid(const Guid& guid) { guid_ = guid; }

		const std::string& GetName() const { return name_; }
		void SetName(const std::string& name) { name_ = name; }

		/**
		 * @brief アセットのタイプ名を取得（派生クラスで実装）
		 */
		virtual std::string GetAssetTypeName() const = 0;

	protected:
		Guid guid_;
		std::string name_;
	};

}
