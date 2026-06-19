#include "AssetDatabase.h"

#include "Engine/Assets/Manager/AssetManager.h"

#include <Engine/Assets/DataAsset/DataAssetManager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Editor/AssetPreviewManager.h>
#include <externals/nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

/////////////////////////////////////////////////////////////////////////////////////////
//		インスタンス取得（シングルトン）
/////////////////////////////////////////////////////////////////////////////////////////
AssetDatabase* AssetDatabase::GetInstance() {
	// 静的ローカル変数を用いて、唯一のインスタンスを生成・保持する（スレッドセーフ）
	static AssetDatabase inst;
	return &inst;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void AssetDatabase::Initialize(const std::filesystem::path& assetsRoot) {
	// 既存の管理データを初期化（再初期化に対応するため）
	records_.clear();
	normPathToGuid_.clear();
	viewCache_.clear();

	// アセットフォルダのルートパスを正規化して保持。
	// weakly_canonical はシンボリックリンクや相対パスを解決した絶対パスを返します。
	std::error_code ec;
	assetsRoot_ = std::filesystem::weakly_canonical(assetsRoot, ec);
	if(ec) {
		// パス解決に失敗した場合は、文字列レベルでの正規化フォールバックを行う
		assetsRoot_ = assetsRoot.lexically_normal();
		ec.clear();
	}

	// アセットルートフォルダが物理的に存在しない場合は自動で新規作成する
	if(!std::filesystem::exists(assetsRoot_)) {
		std::filesystem::create_directories(assetsRoot_, ec);
	}

	// 初期化時にアセットフォルダ全体を走査してデータベースに登録
	Scan();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ルート下の絶対パスを取得
/////////////////////////////////////////////////////////////////////////////////////////
std::filesystem::path AssetDatabase::ToAbsoluteUnderRoot(const std::filesystem::path& absOrRel) const {
	// 引数で指定されたパスが絶対パスの場合は、そのまま正規化（解決）して返す
	if(absOrRel.is_absolute()) {
		return std::filesystem::weakly_canonical(absOrRel);
	}
	// 相対パスの場合は、アセットルートの絶対パスと結合してから正規化して返す
	return std::filesystem::weakly_canonical(assetsRoot_ / absOrRel);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		パス正規化（大文字小文字無視・スラッシュ統一）
/////////////////////////////////////////////////////////////////////////////////////////
std::string AssetDatabase::NormalizePath(const std::filesystem::path& p) {
	// パスを絶対パスかつスラッシュ統一（generic_string）に変換
	auto canon = std::filesystem::weakly_canonical(p).generic_string();
	// 大文字・小文字を区別しないように、すべて小文字に変換（Windows等でのパス比較用）
	for(auto& c : canon) c = (char)std::tolower((unsigned char)c);
	return canon;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		拡張子からアセット種別を推定
/////////////////////////////////////////////////////////////////////////////////////////
AssetType AssetDatabase::GuessTypeFromExtension(const std::string& extIn) {
	// 拡張子の大文字・小文字を統一するため、すべて小文字に変換
	std::string ext = extIn;
	for(auto& c : ext) c = (char)std::tolower((unsigned char)c);

	// 主要な拡張子に対応するアセットタイプをマッピングして返す
	if(ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".tga") return AssetType::Texture;
	if(ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx") return AssetType::Model;
	if(ext == ".hlsl" || ext == ".fxc" || ext == ".cso") return AssetType::Shader;
	if(ext == ".mat") return AssetType::Material;
	if(ext == ".prefab") return AssetType::Prefab;
	if(ext == ".effect" || ext == ".fxasset") return AssetType::Effect;
	if(ext == ".spriteanim") return AssetType::SpriteAnimation;
	if(ext == ".wav" || ext == ".mp3" || ext == ".ogg") return AssetType::Audio;
	
	// 未知の拡張子の場合
	return AssetType::Unknown;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		メタファイルをロードまたは新規作成
/////////////////////////////////////////////////////////////////////////////////////////
AssetGUID AssetDatabase::LoadOrCreateMeta(const std::filesystem::path& absPath, AssetType type) {
	// アセットファイルのパスに ".meta" を付与してメタファイルパスを構築
	auto metaPath = absPath;
	metaPath += ".meta";

	AssetGUID guid = Guid::Empty();

	// 既存の .meta ファイルが存在する場合は読み込みを試みる
	if(std::filesystem::exists(metaPath)) {
		try {
			std::ifstream ifs(metaPath);
			json		  j;
			ifs >> j;
			// JSON内に "guid" キーが存在するか確認
			if(j.contains("guid")) {
				// GUID情報をJSONからデシリアライズして取得
				guid = j.at("guid").get<Guid>();
			}
		} catch(...) {
			// JSONのパースエラーやファイル破損が発生した場合は、異常なメタファイルとみなして新規生成処理へ移行する
		}
	}

	// GUID が無効（未読み込み、または破損）な場合は、新規にGUIDを生成してメタファイルを再作成する
	if(!guid.isValid()) {
		guid = Guid::New();
		try {
			// 新規メタデータのJSONオブジェクトを作成
			json j{
				{"guid", guid}, // Guid クラスのシリアライズ機構により自動で文字列変換される
				{"type", (int)type}};
			std::ofstream ofs(metaPath);
			ofs << j.dump(2); // インデント2でフォーマットして保存
		} catch(...) {
			// ディスク書き込みエラー等の例外を処理
			std::cerr << "[AssetDB] meta write failed: " << metaPath << std::endl;
		}
	}
	return guid;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アセットのプレビュー用テクスチャを構築
/////////////////////////////////////////////////////////////////////////////////////////
void AssetDatabase::BuildPreview(AssetRecord& rec) {
	// プレビュー表示用テクスチャのロードのため、TextureManagerを取得
	auto& tm = *CalyxEngine::AssetManager::GetInstance()->GetTextureManager();
	try {
		if(rec.type == AssetType::Texture) {
			// モデルフォルダ配下にあるテクスチャは、アセットパネルでの個別プレビュー時にロード失敗する可能性があるためスキップする
			std::filesystem::path rel	 = std::filesystem::relative(rec.sourcePath, assetsRoot_);
			std::string			  relStr = rel.generic_string();
			if(relStr.find("models/") == 0 || relStr.find("Models/") == 0) {
				// モデル内のテクスチャは汎用の共通アイコンで代用
				auto icon	   = tm.LoadTexture("UI/Tool/AssetPanel/generic.dds");
				rec.previewTex = (ImTextureID)icon.ptr;
			} else {
				// プレビュー表示用の画像パスと拡張子を取得
				std::filesystem::path previewPath = rec.sourcePath;
				std::string			  ext		  = previewPath.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				// DDS形式の場合は、対応するPNG形式のプレビュー画像への差し替えを試みる
				if(ext == ".dds") {
					previewPath.replace_extension(".png");
					ext = ".png";
				}

				// PNGファイルが存在する場合は、その画像ファイルをテクスチャとしてロードしてプレビューに設定
				if(ext == ".png" && std::filesystem::exists(previewPath)) {
					auto previewRel = std::filesystem::relative(previewPath, assetsRoot_);
					auto texHandle	= tm.LoadTexture(previewRel.generic_string());
					rec.previewTex	= (ImTextureID)texHandle.ptr;
				} else {
					// プレビュー用画像が見つからない場合は、汎用の共通アイコンを使用
					auto icon	   = tm.LoadTexture("UI/Tool/AssetPanel/generic.dds");
					rec.previewTex = (ImTextureID)icon.ptr;
				}
			}
		} else {
			// テクスチャ以外のアセットタイプは、すべて一律で汎用の共通アイコンを使用
			auto icon	   = tm.LoadTexture("UI/Tool/AssetPanel/generic.dds");
			rec.previewTex = (ImTextureID)icon.ptr;
		}
	} catch(...) {
		// ロード失敗や例外発生時はプレビューをクリア（クラッシュ防止）
		rec.previewTex = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ビューキャッシュ再構築（ImGuiなどで一覧表示用）
/////////////////////////////////////////////////////////////////////////////////////////
void AssetDatabase::RebuildViewCache() {
	// キャッシュ用ベクターをクリア
	viewCache_.clear();
	// メモリ再確保のコストを削減するため、マップのサイズ分メモリ領域を予め確保
	viewCache_.reserve(records_.size());
	// マップに登録されている全アセットレコードの生のポインタをキャッシュに格納
	for(auto& [g, recPtr] : records_) {
		viewCache_.push_back(recPtr.get());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		GUID からアセット情報を取得
/////////////////////////////////////////////////////////////////////////////////////////
const AssetRecord* AssetDatabase::Get(const AssetGUID& guid) const {
	// 指定された GUID をキーにマップを検索
	auto it = records_.find(guid);
	if(it == records_.end()) return nullptr; // 存在しない場合は nullptr を返す
	return it->second.get();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		パスからアセットを検索
/////////////////////////////////////////////////////////////////////////////////////////
const AssetRecord* AssetDatabase::FindByPath(const std::filesystem::path& p) const {
	// パスを「アセットルート基準の絶対パス」に変換した上で正規化キーを取得
	auto norm = NormalizePath(ToAbsoluteUnderRoot(p));
	// 正規化パスから GUID への変換マップを検索
	auto it	  = normPathToGuid_.find(norm);
	if(it == normPathToGuid_.end()) return nullptr; // 存在しない場合は nullptr
	// 取得した GUID をもとにレコードを検索して返す
	return Get(it->second);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アセットルートを取得
/////////////////////////////////////////////////////////////////////////////////////////
const std::filesystem::path& AssetDatabase::GetRoot() const noexcept {
	// アセットフォルダのルート絶対パスを返す
	return assetsRoot_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アセット登録または更新
/////////////////////////////////////////////////////////////////////////////////////////
AssetGUID AssetDatabase::RegisterOrUpdate(const std::filesystem::path& absOrRelPath, AssetType forceType) {
	// 入力されたパスをアセットルート下の絶対パスへ変換
	auto abs = ToAbsoluteUnderRoot(absOrRelPath);
	// 物理的なファイルが存在しない場合は空の GUID を返して早期リターン
	if(!std::filesystem::exists(abs)) return Guid::Empty();

	// アセットの種別を特定（forceType が Unknown であれば、拡張子から自動判定を試みる）
	AssetType type = forceType;
	if(type == AssetType::Unknown) type = GuessTypeFromExtension(abs.extension().string());
	// 種別が特定できなかった場合は登録を行わずにリターン
	if(type == AssetType::Unknown) return Guid::Empty();

	// 対応する .meta ファイルから GUID を読み込む（存在しなければ新規に発番して保存）
	auto guid = LoadOrCreateMeta(abs, type);
	// パス比較用に正規化パスを生成
	auto norm = NormalizePath(abs);
	// ファイルの最終更新日時を取得（更新判定などに利用）
	auto ft	  = std::filesystem::last_write_time(abs);

	// 既に同じ GUID でアセットレコードが登録されているか検索
	auto it = records_.find(guid);
	if(it == records_.end()) {
		// --- 新規登録処理 ---
		auto rec		= std::make_unique<AssetRecord>();
		rec->guid		= guid;
		rec->type		= type;
		rec->sourcePath = abs;
		rec->lastWrite	= ft;
		// プレビュー表示用テクスチャ/アイコンを生成・ロード
		BuildPreview(*rec);

		// ルックアップマップおよびレコードマップにアセット情報を追加
		normPathToGuid_[norm] = guid;
		records_.emplace(guid, std::move(rec));
	} else {
		// --- 既存レコードの更新処理 ---
		auto& r				  = *it->second;
		// アセット種別やパスに変更があった場合はプレビューの再構築フラグを立てる
		bool  needPreview	  = (r.type != type) || (r.sourcePath != abs);
		r.type				  = type;
		r.sourcePath		  = abs;
		r.lastWrite			  = ft;
		normPathToGuid_[norm] = guid;

		if(needPreview) {
			BuildPreview(r);
			// キャッシュされているエディタプレビュー画像を無効化して更新を促す
			if(auto* previews = CalyxEngine::AssetPreviewManager::GetInstance()) {
				previews->Invalidate(r.guid);
			}
		}
	}

	// 特定のアセットタイプに対して個別ロード・解析処理を行う
	if(type == AssetType::Material) {
		// マテリアルアセットの場合は、DataAssetManagerを通じてファイルをメモリ上にロードする
		if(auto* manager = CalyxEngine::AssetManager::GetInstance()->GetDataAssetManager()) {
			manager->LoadMaterialAsset(abs, guid);
		}
	}
	if(type == AssetType::SpriteAnimation) {
		// スプライトアニメーションアセットの場合は、DataAssetManagerを通じてロードする
		if(auto* manager = CalyxEngine::AssetManager::GetInstance()->GetDataAssetManager()) {
			manager->LoadSpriteAnimationAsset(abs, guid);
		}
	}

	// 表示用のフラット配列キャッシュを再構築
	RebuildViewCache();
	return guid;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		フォルダ全体をスキャンしアセット登録
/////////////////////////////////////////////////////////////////////////////////////////
void AssetDatabase::Scan() {
	// アセットルートディレクトリが物理的に存在しない場合は処理を中断
	if(!std::filesystem::exists(assetsRoot_)) return;

	// --- 削除されたアセットのクリーンアップ (ガーベジコレクション) ---
	// 登録されているレコードのうち、ファイルが物理的に削除されたレコードを取り除く
	std::erase_if(records_, [this](const auto& item) {
		const auto& rec = item.second;
		// アセットレコードが存在し、その元ファイルが存在していれば維持
		if(rec && std::filesystem::exists(rec->sourcePath)) return false;
		// ファイルが存在しない場合、パス逆引きマップから削除した上でレコードを消去する
		if(rec) normPathToGuid_.erase(NormalizePath(rec->sourcePath));
		return true;
	});

	// --- ルートディレクトリ以下の全ファイルを再帰的に走査してアセット登録 ---
	for(auto& entry : std::filesystem::recursive_directory_iterator(assetsRoot_)) {
		// ディレクトリやシンボリックリンク等はスキップし、通常のファイルのみ処理する
		if(!entry.is_regular_file()) continue;
		const auto& abs = entry.path();
		
		// メタデータファイル (*.meta) 自体はアセットではないためスキップ
		if(abs.extension() == ".meta") continue;

		// ファイル拡張子から対応するアセットタイプを推定
		auto type = GuessTypeFromExtension(abs.extension().string());
		// 登録対象外（Unknown）の拡張子を持つファイルは処理対象外とする
		if(type == AssetType::Unknown) continue;

		// アセット情報をデータベースに登録、または最新の状態に更新
		RegisterOrUpdate(abs, type);
	}

	// 走査結果を反映した表示用キャッシュを再構築
	RebuildViewCache();
}
