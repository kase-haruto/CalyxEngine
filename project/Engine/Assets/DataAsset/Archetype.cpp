#include "Archetype.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>

namespace CalyxEngine {

	void Archetype::CreateFromObject(const std::shared_ptr<SceneObject>& object) {
		if (!object) return;

		serializedData_ = nlohmann::json();
		serializedData_["type"] = std::string(object->GetObjectClassName());
		
		// IConfigurable を持っていれば設定を抽出
		if (auto* cfg = dynamic_cast<IConfigurable*>(object.get())) {
			cfg->ExtractConfigToJson(serializedData_);
		}

		// 名前をアセット名として設定（デフォルト）
		if (name_ == "New DataAsset") {
			name_ = object->GetName() + "_Archetype";
		}
	}

	std::shared_ptr<SceneObject> Archetype::Instantiate() const {
		if (serializedData_.empty()) return nullptr;

		std::string typeName = serializedData_.value("type", "");
		if (typeName.empty()) return nullptr;

		auto sp = SceneObjectRegistry::Get().Create(typeName);
		if (!sp) return nullptr;

		// IConfigurable を持っていれば設定を適用
		if (auto* cfg = dynamic_cast<IConfigurable*>(sp.get())) {
			cfg->ApplyConfigFromJson(serializedData_);
		}

		// GUIDは新しく生成される（SceneObjectのコンストラクタで生成されるはず）
		sp->Initialize();

		return sp;
	}

}
