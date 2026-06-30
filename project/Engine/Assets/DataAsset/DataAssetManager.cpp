#include "DataAssetManager.h"

#include "MaterialAsset.h"
#include "SpriteAnimationAsset.h"

#include <Engine/Foundation/Log/EngineLogger.h>
#include <Engine\Foundation\Serialization\SerializableField.h>
#include <Engine\Foundation\Utility\FileSystem\FileSystemHelper.h>
#include <externals\nlohmann\json.hpp>

#include <fstream>

namespace CalyxEngine {
	namespace {
		/**
		 * @brief データアセットのシリアライズ対象フィールド群をJSONオブジェクトに変換するヘルパー関数
		 */
		nlohmann::json FieldsToJson(const DataAsset& asset) {
			nlohmann::json fields = nlohmann::json::object();
			for(const auto& field : asset.Fields()) {
				nlohmann::json value;
				// 登録されたフィールドのアドレス(ptr)から型を推論してJSONにシリアライズ
				WriteValue(value, field.ptr);
				fields[field.key] = std::move(value);
			}
			return fields;
		}

		/**
		 * @brief JSONからデータアセットのフィールド値を復元するヘルパー関数
		 */
		void ApplyFieldsFromJson(DataAsset& asset, const nlohmann::json& root) {
			const nlohmann::json* fields = nullptr;
			// "fields"というキーの中にオブジェクトがある場合はそれを読み込み、なければroot直下を対象とする
			if(root.contains("fields") && root["fields"].is_object()) {
				fields = &root["fields"];
			} else if(root.is_object()) {
				fields = &root;
			}
			if(!fields) return;

			// アセットに定義されている各シリアライズ対象フィールドに対して値をデシリアライズ
			for(auto& field : asset.FieldsMutable()) {
				if(!fields->contains(field.key)) continue;
				ReadValue((*fields)[field.key], field.ptr);
			}
		}

		/**
		 * @brief 指定パスのJSONファイルを読み込む
		 */
		bool ReadJsonFile(const std::filesystem::path& path, nlohmann::json& out) {
			std::ifstream ifs(path);
			if(!ifs) return false;
			try {
				ifs >> out;
			} catch(...) {
				// パースエラー等が発生した場合は読み込み失敗とする
				return false;
			}
			return true;
		}

		/**
		 * @brief 指定パスにJSONオブジェクトを出力する（必要なディレクトリも自動生成）
		 */
		bool WriteJsonFile(const std::filesystem::path& path, const nlohmann::json& j) {
			// ディレクトリが存在しない場合は、親ディレクトリ群を自動作成
			FileSystemHelper::CreateDirectoryPath(path.parent_path().string());
			std::ofstream ofs(path);
			if(!ofs) return false;
			ofs << j.dump(2); // インデント2スペースでフォーマットして保存
			return true;
		}

		/**
		 * @brief スプライトアニメーションクリップをJSONオブジェクトに変換する
		 */
		nlohmann::json ClipToJson(const SpriteAnimationClip& clip) {
			return nlohmann::json{
				{"name", clip.name},
				{"startFrame", clip.startFrame},
				{"frameCount", clip.frameCount},
				{"frameDuration", clip.frameDuration},
				{"loop", clip.loop}};
		}

		/**
		 * @brief JSONからスプライトアニメーションクリップの情報を構築する
		 */
		SpriteAnimationClip ClipFromJson(const nlohmann::json& root) {
			SpriteAnimationClip clip;
			clip.name = root.value("name", clip.name);
			clip.startFrame = root.value("startFrame", clip.startFrame);
			clip.frameCount = root.value("frameCount", clip.frameCount);
			clip.frameDuration = root.value("frameDuration", clip.frameDuration);
			clip.loop = root.value("loop", clip.loop);
			return clip;
		}
	}

	void DataAssetManager::RegisterAsset(const std::shared_ptr<DataAsset>& asset) {
		if (!asset) return;
		// GUIDを一意のキーとして内部マップに登録
		assets_[asset->GetGuid()] = asset;
	}

	std::shared_ptr<DataAsset> DataAssetManager::GetAsset(const Guid& guid) const {
		auto it = assets_.find(guid);
		if (it != assets_.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<DataAsset> DataAssetManager::GetAssetByName(const std::string& name) const {
		// 全アセットを線形走査し、名称が一致するものを取得（重複がある場合は最初に見つかったもの）
		for (auto& pair : assets_) {
			if (pair.second->GetName() == name) {
				return pair.second;
			}
		}
		return nullptr;
	}

	void DataAssetManager::UnregisterAsset(const Guid& guid) {
		// 管理マップからアセットのエントリを削除
		assets_.erase(guid);
	}

	std::shared_ptr<MaterialAsset> DataAssetManager::LoadMaterialAsset(const std::filesystem::path& path, const Guid& guid) {
		if(!guid.isValid()) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Asset, "Material load failed because its GUID is invalid: " + path.generic_string(), "DataAssetManager");
			return nullptr;
		}

		// マテリアルアセットの初期オブジェクトを生成して基本情報を設定
		auto material = std::make_shared<MaterialAsset>();
		material->SetGuid(guid);
		material->SetName(path.stem().string());

		nlohmann::json root;
		if(ReadJsonFile(path, root)) {
			// ファイル読み込み成功時はJSONの内容を適用
			material->SetName(root.value("name", material->GetName()));
			ApplyFieldsFromJson(*material, root);
			if(root.contains("graph")) {
				// マテリアルノードグラフ情報が存在すればデシリアライズ
				material->graph = root.at("graph").get<NodeGraph>();
			}
		} else {
			// ファイルが存在しない、または破損している場合はデフォルト設定で新規保存
			SaveAsset(*material, path);
		}

		// キャッシュに登録して返す
		RegisterAsset(material);
		EngineLogger::GetInstance().Add(LogLevel::Trace, LogCategory::Asset, "Material loaded: " + path.generic_string(), "DataAssetManager");
		return material;
	}

	std::shared_ptr<SpriteAnimationAsset> DataAssetManager::LoadSpriteAnimationAsset(const std::filesystem::path& path, const Guid& guid) {
		if(!guid.isValid()) return nullptr;

		// スプライトアニメーションアセットの初期オブジェクトを生成
		auto spriteAnimation = std::make_shared<SpriteAnimationAsset>();
		spriteAnimation->SetGuid(guid);
		spriteAnimation->SetName(path.stem().string());

		nlohmann::json root;
		if(ReadJsonFile(path, root)) {
			// ファイルが存在すればJSONの内容を適用
			spriteAnimation->SetName(root.value("name", spriteAnimation->GetName()));
			ApplyFieldsFromJson(*spriteAnimation, root);
			spriteAnimation->texturePath = root.value("texturePath", spriteAnimation->texturePath);
			
			spriteAnimation->clips.clear();
			// アニメーションクリップ配列のパース
			if(root.contains("clips") && root["clips"].is_array()) {
				for(const auto& clipJson : root["clips"]) {
					if(clipJson.is_object()) {
						spriteAnimation->clips.push_back(ClipFromJson(clipJson));
					}
				}
			}
			// クリップが一つもない場合は、デフォルトクリップを1つ追加して安全を保証
			if(spriteAnimation->clips.empty()) {
				spriteAnimation->clips.push_back({});
			}
		} else {
			// ファイルが存在しない、または破損している場合はデフォルト設定で保存
			SaveAsset(*spriteAnimation, path);
		}

		// キャッシュに登録して返す
		RegisterAsset(spriteAnimation);
		return spriteAnimation;
	}

	bool DataAssetManager::SaveAsset(const DataAsset& asset, const std::filesystem::path& path) const {
		nlohmann::json root;
		// 基本メタ情報の格納
		root["type"] = asset.GetAssetTypeName();
		root["guid"] = asset.GetGuid();
		root["name"] = asset.GetName();
		// 各シリアライズ対象パラメータフィールドの格納
		root["fields"] = FieldsToJson(asset);

		// --- 派生アセット特有のプロパティのダウンキャスト保存 ---
		
		// マテリアルアセットの場合はノードグラフを追加でシリアライズ
		if(auto material = dynamic_cast<const MaterialAsset*>(&asset)) {
			root["graph"] = material->graph;
		}
		// スプライトアニメーションアセットの場合はテクスチャパスとクリップ配列を追加でシリアライズ
		if(auto spriteAnimation = dynamic_cast<const SpriteAnimationAsset*>(&asset)) {
			root["texturePath"] = spriteAnimation->texturePath;
			root["clips"] = nlohmann::json::array();
			for(const auto& clip : spriteAnimation->clips) {
				root["clips"].push_back(ClipToJson(clip));
			}
		}
		// JSONファイルとして書き込み
		const bool succeeded = WriteJsonFile(path, root);
		EngineLogger::GetInstance().Add(
			succeeded ? LogLevel::Info : LogLevel::Error,
			LogCategory::Asset,
			(succeeded ? "Data asset saved: " : "Data asset save failed: ") + path.generic_string(),
			"DataAssetManager");
		return succeeded;
	}

	std::shared_ptr<MaterialAsset> DataAssetManager::CreateMaterialAsset(const std::filesystem::path& path, const Guid& guid, const std::string& name) {
		auto material = std::make_shared<MaterialAsset>();
		// GUIDの指定がない場合は自動的に新規発番
		material->SetGuid(guid.isValid() ? guid : Guid::New());
		material->SetName(name.empty() ? path.stem().string() : name);
		
		// 初期アセットファイルを作成保存
		if(!SaveAsset(*material, path)) return nullptr;
		// キャッシュに登録
		RegisterAsset(material);
		EngineLogger::GetInstance().Add(LogLevel::Info, LogCategory::Asset, "Material created: " + path.generic_string(), "DataAssetManager");
		return material;
	}

	std::shared_ptr<SpriteAnimationAsset> DataAssetManager::CreateSpriteAnimationAsset(const std::filesystem::path& path, const Guid& guid, const std::string& name) {
		auto spriteAnimation = std::make_shared<SpriteAnimationAsset>();
		// GUIDの指定がない場合は自動的に新規発番
		spriteAnimation->SetGuid(guid.isValid() ? guid : Guid::New());
		spriteAnimation->SetName(name.empty() ? path.stem().string() : name);
		
		// 初期アセットファイルを作成保存
		if(!SaveAsset(*spriteAnimation, path)) return nullptr;
		// キャッシュに登録
		RegisterAsset(spriteAnimation);
		return spriteAnimation;
	}

}
