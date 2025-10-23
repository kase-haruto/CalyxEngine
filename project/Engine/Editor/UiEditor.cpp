#include "engine/Editor/UiEditor.h"
/* ========================================================================
/* include space
/* ===================================================================== */
/* engine */
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// external
#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // _DEBUG

/* lib */
#include <string>
#include <externals/nlohmann/json.hpp>
#include <fstream>

UIEditor::UIEditor(const std::string& name):BaseEditor(name){
	// テクスチャマネージャーのインスタンスを取得
	textureManager_ = TextureManager::GetInstance();
	LoadSpriteDataFromJson("Resources/json/sprite/spriteData.json");
}

void UIEditor::ShowImGuiInterface(){
#ifdef _DEBUG


	// JSON 保存ボタンの追加
	if (ImGui::Button("Save to JSON")){
		// 実際にはパスをユーザに入力させても良いし、固定ファイル名でもOK
		SaveSpriteDataToJson("Resources/json/sprite/spriteData.json");
	}

	// テクスチャリストの表示
	static std::string selectedTexture = "uvChecker.png"; // デフォルトの選択テクスチャ名

	if (ImGui::BeginCombo("select", selectedTexture.c_str())){
		// テクスチャマネージャーからロード済みのテクスチャリストを取得
		for (const auto& texturePair : textureManager_->GetLoadedTextures()){
			bool isSelected = (selectedTexture == texturePair.first);
			if (ImGui::Selectable(texturePair.first.c_str(), isSelected)){
				selectedTexture = texturePair.first;
			}
			if (isSelected){
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}


	// 選択されたテクスチャのプレビュー表示
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = textureManager_->GetSrvHandle(selectedTexture);
	if (srvHandle.ptr != 0){
		ImTextureID textureId = reinterpret_cast< ImTextureID >(srvHandle.ptr);
		ImGui::Image(textureId, ImVec2(128, 128)); // プレビューのサイズを設定
	}

	// スプライトの生成用の入力欄
	static Vector2 newPosition = {400.0f, 0.0f}; // デフォルトの位置
	static Vector2 newSize = {64.0f, 64.0f};     // デフォルトのサイズ

	GuiCmd::SliderFloat2("Position", newPosition, 0.0f, 1280.0f); // 位置の入力
	GuiCmd::SliderFloat2("Size", newSize, 0.0f, 512.0f);          // サイズの入力

	// スプライトの作成ボタン
	if (ImGui::Button("Create")){
		AddSprite(selectedTexture, newPosition, newSize);
	}

	// スプライトリストの表示
	if (ImGui::CollapsingHeader("Sprites List", ImGuiTreeNodeFlags_DefaultOpen)){
		ImGui::Text("Select a sprite to edit its properties:");

		for (auto& sprite : sprites_){
			// 名前をわかりやすくするため、スプライトのインデックス番号を表示
			// アドレスを利用してのラベルを生成
			std::string spriteLabel = "Sprite " + std::to_string(reinterpret_cast< uintptr_t >(sprite.get()));

			// 各スプライトごとに CollapsingHeader を作成
			if (ImGui::CollapsingHeader(spriteLabel.c_str(), ImGuiTreeNodeFlags_None)){
				// スプライトのプロパティ編集
				Vector2 position = sprite->GetPosition();
				float rotate = sprite->GetRotation();
				Vector2 size = sprite->GetSize();

				// アドレスを使ったのIDを生成
				std::string uniqueId = std::to_string(reinterpret_cast< uintptr_t >(sprite.get()));

				// 位置の調整
				GuiCmd::SliderFloat2(("Position##" + uniqueId).c_str(), position, 0.0f, 1280.0f);
				sprite->SetPosition(position);

				// 回転の調整
				GuiCmd::SliderFloat(("Rotation##" + uniqueId).c_str(), rotate, -180.0f, 180.0f);
				sprite->SetRotation(rotate);

				// サイズの調整
				GuiCmd::SliderFloat2(("Size##" + uniqueId).c_str(), size, 1.0f, 512.0f);
				sprite->SetSize(size);
			}
		}

	}


#endif // _DEBUG
}

void UIEditor::AddSprite(const std::string& textureName, const Vector2& position, const Vector2& size){
	// 指定されたテクスチャ名でテクスチャをロード
	D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = textureManager_->LoadTexture(textureName);
	auto sprite = std::make_shared<Sprite>(textureName);

	// スプライトの初期化
	sprite->Initialize(position, size);
	sprite->SetTextureHandle(textureHandle);

	// スプライトリストに追加
	sprites_.emplace_back(sprite);
}

void UIEditor::Update(){
	for (auto& sprite: sprites_){
		sprite->Update();
	}
}

void UIEditor::Draw(){
	//for (const auto& sprite : sprites_){
	//	sprite->Draw();
	//}
}

void UIEditor::SaveSpriteDataToJson(const std::string& filePath){
	// すべてのスプライトの情報を格納する json 配列
	nlohmann::json spritesJson = nlohmann::json::array();

	for (auto& sprite : sprites_){
		// 1つのスプライトに対応する json オブジェクトを作成
		nlohmann::json spriteData;
		spriteData["textureName"] = sprite->GetTextureName();  // ※Sprite 側で GetTextureName() を用意してください

		// 位置
		Vector2 pos = sprite->GetPosition();
		spriteData["position"] = {
			{"x", pos.x},
			{"y", pos.y}
		};

		// サイズ
		Vector2 size = sprite->GetSize();
		spriteData["size"] = {
			{"x", size.x},
			{"y", size.y}
		};

		// 回転
		float rot = sprite->GetRotation();  // 度数法前提
		spriteData["rotation"] = rot;

		// 配列にプッシュ
		spritesJson.push_back(spriteData);
	}

	// 出力用 JSON オブジェクト全体
	nlohmann::json root;
	root["sprites"] = spritesJson;

	// JSON をファイルに書き出し
	std::ofstream ofs(filePath);
	if (ofs.is_open()){
		ofs << root.dump(4); // インデント幅4で整形出力
		ofs.close();
	}
}

void UIEditor::LoadSpriteDataFromJson(const std::string& filePath){
	std::ifstream ifs(filePath);
	if (!ifs.is_open()){
		return;
	}
	nlohmann::json root;
	ifs >> root;
	ifs.close();

	if (root.contains("sprites")){
		for (auto& spriteData : root["sprites"]){
			// 必要情報を取得
			std::string textureName = spriteData.value("textureName", "uvChecker.png");

			Vector2 pos;
			pos.x = spriteData["position"].value("x", 0.0f);
			pos.y = spriteData["position"].value("y", 0.0f);

			Vector2 size;
			size.x = spriteData["size"].value("x", 64.0f);
			size.y = spriteData["size"].value("y", 64.0f);

			float rotation = spriteData.value("rotation", 0.0f);

			// スプライト作成
			AddSprite(textureName, pos, size);
			// rotation は AddSprite 後に個別に設定
			sprites_.back()->SetRotation(rotation);
		}
	}
}
