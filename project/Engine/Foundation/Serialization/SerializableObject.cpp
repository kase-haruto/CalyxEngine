#include "SerializableObject.h"
#include "SerializableUtil.h"
#include "ParamStore.h"
#include "imgui/imgui.h"

#include <algorithm>
#include <unordered_set>

namespace CalyxEngine {
	namespace {
		// 遅延Capture中に破棄されたオブジェクトを識別するため、生存中インスタンスだけを追跡する。
		std::unordered_set<const SerializableObject*> gLiveObjects;

		// シーン生成スレッドごとにCapture状態を分離し、並列ロード時の混線を防ぐ。
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
		// まずParamStoreの永続値を読み込み、シーン固有Overrideを後から優先適用する。
		const bool loaded = ParamStore::Load(*this);

		if(gOverrides && gOverrides->is_object()) {
			const std::string key = GetParamStorageKey();
			if(gOverrides->contains(key)) {
				// 現行形式の完全修飾キーが存在する場合はそれを最優先する。
				ApplyParamsFromJson(gOverrides->at(key));
			} else if(gOverrides->contains("param") && gOverrides->at("param").is_object()) {
				const Json& legacyParam = gOverrides->at("param");
				// 旧param形式のシーンを読み込むため、保存時の正規キーが無い場合だけ互換適用する。
				if(legacyParam.contains(key)) {
					ApplyParamsFromJson(legacyParam.at(key));
				} else if(legacyParam.contains("fields")) {
					ApplyParamsFromJson(legacyParam);
				}
			}
		}

		if(gCaptureTarget) {
			// 同一オブジェクトが複数回LoadしてもCapture一覧へ重複登録しない。
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
		// 登録済みフィールドだけを保存し、クラス内部の非公開実装状態をシリアライズ対象から除外する。
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

		// JSONに存在するフィールドだけを上書きし、追加された新規フィールドは既定値を維持する。
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
		// Domain/SubDirectory/Nameを連結し、同名パラメータ同士の保存先衝突を避ける。
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
		// Capture期間中にLoadしたParamを呼び出し側の所有一覧へ関連付ける。
		gCaptureTarget = captureTarget;
		gOverrides = overrides;
	}

	void SerializableObject::EndCapture() {
		// 次のロードへOverrideや出力先が漏れないよう、スレッドローカル状態を解除する。
		gCaptureTarget = nullptr;
		gOverrides = nullptr;
	}

	void SerializableObject::BeginPendingCapture() {
		// 所有先が確定する前に生成されるParamを一時収集する。
		gPendingCapture.clear();
		gPendingCaptureActive = true;
	}

	void SerializableObject::EndPendingCapture(std::vector<SerializableObject*>* captureTarget,
											   const Json* overrides) {
		if(captureTarget) {
			for(auto* param : gPendingCapture) {
				// Capture中に破棄されたアドレスを参照せず、生存するParamだけを引き渡す。
				if(!param) continue;
				if(!IsAlive(param)) continue;
				if(std::find(captureTarget->begin(), captureTarget->end(), param) == captureTarget->end()) {
					captureTarget->push_back(param);
				}
				if(overrides && overrides->is_object()) {
					// 所有先確定後にOverrideを適用し、通常Captureと同じ読み込み結果へ揃える。
					const std::string key = param->GetParamStorageKey();
					if(overrides->contains(key)) {
						param->ApplyParamsFromJson(overrides->at(key));
					} else if(overrides->contains("param") && overrides->at("param").is_object()) {
						const Json& legacyParam = overrides->at("param");
						// 旧param形式のシーンを読み込むため、保存時の正規キーが無い場合だけ互換適用する。
						if(legacyParam.contains(key)) {
							param->ApplyParamsFromJson(legacyParam.at(key));
						} else if(legacyParam.contains("fields")) {
							param->ApplyParamsFromJson(legacyParam);
						}
					}
				}
			}
		}

		// 一時一覧を破棄してCaptureを閉じ、後続生成物の誤登録を防ぐ。
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
