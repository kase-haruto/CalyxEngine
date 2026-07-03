#include "SerializableObject.h"
#include "SerializableUtil.h"
#include "ParamStore.h"
#include "imgui/imgui.h"

#include <algorithm>
#include <unordered_set>

namespace CalyxEngine {
	namespace {
		std::unordered_set<const SerializableObject*> gLiveObjects;
		thread_local std::vector<SerializableObject*>* gCaptureTarget = nullptr;
		thread_local const Json* gOverrides = nullptr;
		thread_local bool gPendingCaptureActive = false;
		thread_local std::vector<SerializableObject*> gPendingCapture;

		std::string ToString(ParamDomain domain) {
			switch(domain) {
			case ParamDomain::Game:
				return "Game";
			case ParamDomain::Engine:
				return "Engine";
			case ParamDomain::Editor:
				return "Editor";
			}
			return "Unknown";
		}
	}

	SerializableObject::SerializableObject() {
		gLiveObjects.insert(this);
	}

	SerializableObject::~SerializableObject() {
		gLiveObjects.erase(this);
	}

	bool SerializableObject::SaveParams() const { return ParamStore::Save(*this); }

	bool SerializableObject::LoadParams() {
		const bool loaded = ParamStore::Load(*this);

		if(gOverrides && gOverrides->is_object()) {
			const std::string key = GetParamStorageKey();
			if(gOverrides->contains(key)) {
				ApplyParamsFromJson(gOverrides->at(key));
			}
		}

		if(gCaptureTarget) {
			if(std::find(gCaptureTarget->begin(), gCaptureTarget->end(), this) == gCaptureTarget->end()) {
				gCaptureTarget->push_back(this);
			}
		} else if(gPendingCaptureActive) {
			if(std::find(gPendingCapture.begin(), gPendingCapture.end(), this) == gPendingCapture.end()) {
				gPendingCapture.push_back(this);
			}
		}

		return loaded;
	}

	void SerializableObject::ExtractParamsToJson(Json& j) const {
		j["fields"] = Json::object();
		for(const auto& f : Fields()) {
			Json v;
			WriteValue(v, f.ptr);
			j["fields"][f.key] = v;
		}
	}

	void SerializableObject::ApplyParamsFromJson(const Json& j) {
		if(!j.contains("fields") || !j["fields"].is_object()) {
			return;
		}

		for(auto& f : FieldsMutable()) {
			if(!j["fields"].contains(f.key)) continue;
			ReadValue(j["fields"][f.key], f.ptr);
		}
	}

	void SerializableObject::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
		// フィールド型を走査し、SceneObject参照だけへGUID対応表を適用する。
		for(auto& field : fields_) {
			std::visit([&](auto* value) {
				using Value = std::remove_pointer_t<decltype(value)>;
				if constexpr(std::is_same_v<Value, ISceneObjectReference>) {
					value->Remap(guidMap);
				}
			}, field.ptr);
		}
	}

	std::string SerializableObject::GetParamStorageKey() const {
		const ParamPath path = GetParamPath();
		std::string key = ToString(path.domain);
		if(path.subDirectory.has_value() && !path.subDirectory->empty()) {
			key += "/";
			key += *path.subDirectory;
		}
		key += "/";
		key += path.name;
		return key;
	}

	void SerializableObject::BeginCapture(std::vector<SerializableObject*>* captureTarget,
										  const Json* overrides) {
		gCaptureTarget = captureTarget;
		gOverrides = overrides;
	}

	void SerializableObject::EndCapture() {
		gCaptureTarget = nullptr;
		gOverrides = nullptr;
	}

	void SerializableObject::BeginPendingCapture() {
		gPendingCapture.clear();
		gPendingCaptureActive = true;
	}

	void SerializableObject::EndPendingCapture(std::vector<SerializableObject*>* captureTarget,
											   const Json* overrides) {
		if(captureTarget) {
			for(auto* param : gPendingCapture) {
				if(!param) continue;
				if(!IsAlive(param)) continue;
				if(std::find(captureTarget->begin(), captureTarget->end(), param) == captureTarget->end()) {
					captureTarget->push_back(param);
				}
				if(overrides && overrides->is_object()) {
					const std::string key = param->GetParamStorageKey();
					if(overrides->contains(key)) {
						param->ApplyParamsFromJson(overrides->at(key));
					}
				}
			}
		}

		gPendingCapture.clear();
		gPendingCaptureActive = false;
	}

	bool SerializableObject::IsAlive(const SerializableObject* object) {
		return object && gLiveObjects.contains(object);
	}

	void SerializableObject::SaveAndLoadButtonGui() {
		auto path = GetParamPath();
		std::string loadLabel = "Load " + path.name;
		std::string saveLabel = "Save " + path.name;

		if(ImGui::Button(loadLabel.c_str())) { LoadParams(); }
		ImGui::SameLine();
		if(ImGui::Button(saveLabel.c_str())) { SaveParams(); }
	}

	bool SerializableObject::ShowGui() {
		VariableCategoryNode root;
		BuildCategoryTree(root, Fields());

		bool changed = false;
		for (const auto& [_, node] : root.children) {
			// Integrate Save/Load buttons into each root category tab
			changed |= DrawCategoryNode(node, this);
		}

		return changed;
	}

} // namespace CalyxEngine
