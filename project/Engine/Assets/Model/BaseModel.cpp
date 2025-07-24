#include "BaseModel.h"
/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Assets/Model/ModelManager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

// lib
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Foundation/Clock/ClockManager.h>


//external
#include "externals/imgui/imgui.h"

const std::string BaseModel::directoryPath_ = "Resource/models";

void BaseModel::Update() {
	// --- まだ modelData_ を取得していないなら、取得を試みる ---
	if (!modelData_) {
		if (ModelManager::GetInstance()->IsModelLoaded(fileName_)) {
			auto loaded = ModelManager::GetInstance()->GetModelData(fileName_);
			modelData_ = loaded;
			OnModelLoaded();
		}
		// loaded が nullptr の場合、まだ読み込み中
	} else {
		// テクスチャの更新
		UpdateTexture();

		// UV transform を行列化 
		Matrix4x4 uvTransformMatrix = MakeScaleMatrix(Vector3(uvTransform.scale.x, uvTransform.scale.y, 1.0f));
		uvTransformMatrix = Matrix4x4::Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransform.rotate));
		uvTransformMatrix = Matrix4x4::Multiply(uvTransformMatrix, MakeTranslateMatrix(Vector3(uvTransform.translate.x, uvTransform.translate.y, 0.0f)));

		materialData_.uvTransform = uvTransformMatrix;
		materialBuffer_.TransferData(materialData_);

		// カメラ行列との掛け合わせ
		modelData_->vertexBuffer.TransferVectorData(modelData_->meshData.vertices);
		modelData_->indexBuffer.TransferVectorData(modelData_->meshData.indices);
		Map();
	}

}

void BaseModel::OnModelLoaded() {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	modelData_->vertexBuffer.Initialize(device, UINT(modelData_->meshData.vertices.size()));
	modelData_->indexBuffer.Initialize(device, UINT(modelData_->meshData.indices.size()));


	// テクスチャ設定
	if (!handle_) {
		handle_ = TextureManager::GetInstance()->LoadTexture(
			"Textures/" + modelData_->meshData.material.textureFilePath);
		if (!handle_) { // 読み込み失敗・空文字列など
			handle_ = TextureManager::GetInstance()->LoadTexture("textures/white1x1.png");
		}
	}

	// -------- インスタンシングバッファの初期確保 --------
	if (!instanceBufferCreated_){
		instanceBufferCapacity_ = 1024; // 初期インスタンス数（適宜調整）
		instanceBuffer_.Initialize(device, instanceBufferCapacity_);
		instanceBuffer_.CreateSrv(device);
		instanceBufferCreated_ = true;
	}
}

void BaseModel::UpdateTexture() {
	if (textureHandles_.size() <= 1) return; // アニメーション不要
	elapsedTime_ += ClockManager::GetInstance()->GetDeltaTime();
	if (elapsedTime_ >= animationSpeed_) {
		elapsedTime_ -= animationSpeed_;
		currentFrameIndex_ = (currentFrameIndex_ + 1) % textureHandles_.size();
		handle_ = textureHandles_[currentFrameIndex_]; // テクスチャを切り替え
	}
}

void BaseModel::ShowImGuiInterface() {


	ImGui::Separator();
	ImGui::Text("Model: %s", fileName_.c_str());

	uvTransform.ShowImGui("uvTransform");

	if (ImGui::CollapsingHeader("Material")) {
		materialData_.ShowImGui();

		auto& textures = TextureManager::GetInstance()->GetLoadedTextures();
		if (ImGui::BeginCombo("Texture", textureName_.c_str())) {
			for (const auto& texture : textures) {
				bool is_selected = (textureName_ == texture.first);
				if (ImGui::Selectable(texture.first.c_str(), is_selected)) {
					textureName_ = texture.first;
					handle_ = TextureManager::GetInstance()->LoadTexture(texture.first);
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	if (ImGui::CollapsingHeader("Draw")) {
		static const char* blendModeNames[] = {
			"NONE", "ALPHA", "ADD", "SUB", "MUL", "NORMAL", "SCREEN"
		};

		int currentBlendMode = static_cast<int>(blendMode_);
		if (ImGui::Combo("Blend Mode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
			blendMode_ = static_cast<BlendMode>(currentBlendMode);
		}
	}
}

void BaseModel::Draw(const WorldTransform& transform) {
	if (!isDrawEnable_)return;

	ID3D12GraphicsCommandList* cmdList = GraphicsGroup::GetInstance()->GetCommandList().Get();
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// マテリアル & 行列バッファをセット
	materialBuffer_.SetCommand(cmdList, 0);
	transform.SetCommand(cmdList, 1);

	cmdList->SetGraphicsRootDescriptorTable(2, handle_.value());

	//環境マップ
	D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle = TextureManager::GetInstance()->GetEnvironmentTextureSrvHandle();
	cmdList->SetGraphicsRootDescriptorTable(6, envMapHandle);

	// 描画
	cmdList->DrawIndexedInstanced(UINT(modelData_->meshData.indices.size()), 1, 0, 0, 0);
}

void BaseModel::DrawInstanced(const std::vector<WorldTransform>& transforms,
							  ID3D12GraphicsCommandList* cmdList){
	if (!isDrawEnable_ || !modelData_ || transforms.empty()) return;

	// 転送
	std::vector<TransformationMatrix> matrices;
	matrices.reserve(transforms.size());
	for (const auto& tf : transforms){
		TransformationMatrix mat;
		mat.world = tf.matrix.world;
		mat.WorldInverseTranspose = Matrix4x4::Transpose(Matrix4x4::Inverse(tf.matrix.world));
		matrices.push_back(mat);
	}
	instanceBuffer_.TransferVectorData(matrices);

	cmdList->SetGraphicsRootDescriptorTable(1, instanceBuffer_.GetGpuHandle());

	// マテリアル・テクスチャ等
	materialBuffer_.SetCommand(cmdList, 0);
	cmdList->SetGraphicsRootDescriptorTable(2, handle_.value());

	auto envMapHandle = TextureManager::GetInstance()->GetEnvironmentTextureSrvHandle();
	cmdList->SetGraphicsRootDescriptorTable(6, envMapHandle);

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	modelData_->vertexBuffer.SetCommand(cmdList);
	modelData_->indexBuffer.SetCommand(cmdList);

	cmdList->DrawIndexedInstanced(
		static_cast< UINT >(modelData_->meshData.indices.size()),
		static_cast< UINT >(transforms.size()), 0, 0, 0);
}


void BaseModel::ApplyConfig(const BaseModelConfig& config){
	materialData_.ApplyConfig(config.materialConfig);
	uvTransform.ApplyConfig(config.uvTransConfig);
	blendMode_ = static_cast< BlendMode >(config.blendMode);
	fileName_ = config.modelName;

	if (config.textureName.empty()){
		textureName_ = "textures/white1x1.png";
		handle_ = TextureManager::GetInstance()->LoadTexture(textureName_);
	} else{
		textureName_ = config.textureName;
		handle_ = TextureManager::GetInstance()->LoadTexture(textureName_);
	}
}

BaseModelConfig BaseModel::ExtractConfig() const{
	BaseModelConfig config;
	config.materialConfig = materialData_.ExtractConfig();
	config.uvTransConfig = uvTransform.ExtractConfig();
	config.blendMode = static_cast< int >(blendMode_);
	config.modelName = fileName_;
	config.textureName = textureName_;
	return config;
}

void BaseModel::ShowImGui(BaseModelConfig& config){
	uvTransform.ShowImGui(config.uvTransConfig,"uvTransform");

	if (ImGui::CollapsingHeader("Material")){
		materialData_.ShowImGui(config.materialConfig);

		auto& textures = TextureManager::GetInstance()->GetLoadedTextures();
		if (ImGui::BeginCombo("Texture", textureName_.c_str())){
			for (const auto& texture : textures){
				bool is_selected = (textureName_ == texture.first);
				if (ImGui::Selectable(texture.first.c_str(), is_selected)){
					textureName_ = texture.first;
					handle_ = TextureManager::GetInstance()->LoadTexture(texture.first);
				}
				if (is_selected){
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	if (ImGui::CollapsingHeader("Draw")){
		static const char* blendModeNames[] = {
			"NONE", "ALPHA", "ADD", "SUB", "MUL", "NORMAL", "SCREEN"
		};

		int currentBlendMode = static_cast< int >(blendMode_);
		if (ImGui::Combo("Blend Mode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames))){
			config.blendMode = currentBlendMode;
		}
	}
}

const std::optional<ModelData>& BaseModel::GetModelData() const{
	return modelData_;
}