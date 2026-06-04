#include "BaseModel.h"
/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Assets/Model/ModelManager.h>
#include <Engine/Assets/System/AssetDragPayload.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Assets/DataAsset/MaterialAsset.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/System/Command/EditorCommand/ValueEditCommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>


// lib
#include <Engine/Foundation/Utility/Func/MyFunc.h>

//external
#include "Engine/Assets/Manager/AssetManager.h"
#include "Engine/Foundation/Math/MathUtil.h"
#include "externals/imgui/imgui.h"
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <algorithm>
#include <Engine/Objects/3D/Details/BillboardParams.h>
#include "Engine/Foundation/Utility/Func/CxUtils.h"
#include "Engine/Graphics/Context/GraphicsGroup.h"

#include <array>

const std::string BaseModel::directoryPath_ = "Resource/models";

namespace {
	uint64_t gBaseModelUploadFrameGeneration = 1;
	constexpr uint32_t kMaxGraphTextures = 8;
}

BaseModel::~BaseModel() {
	for(const DescriptorHandle& textureTable : materialGraphTextureTables_) {
		DescriptorAllocator::FreeRange(DescriptorUsage::CbvSrvUav, textureTable, kMaxGraphTextures);
	}
}

void BaseModel::BeginUploadFrame() {
	++gBaseModelUploadFrameGeneration;
}

void BaseModel::Update(float deltaTime) {
	// --- まだ modelData_ を取得していないなら、取得を試みる ---
	if(!modelData_) {
		if(CalyxEngine::AssetManager::GetInstance()->GetModelManager()->IsModelLoaded(fileName_)) {
			ModelData& loaded = CalyxEngine::AssetManager::GetInstance()->GetModelManager()->GetModelData(fileName_);
			modelData_        = &loaded; // ModelData* を保持する

			OnModelLoaded();
		}
		// loaded が nullptr の場合、まだ読み込み中
	} else {
		// テクスチャの更新
		UpdateTexture(deltaTime);

		// UV transform を行列化
		CalyxEngine::Matrix4x4 uvTransformMatrix = CalyxEngine::MakeScaleMatrix(CalyxEngine::Vector3(uvTransform.scale.x, uvTransform.scale.y, 1.0f));
		uvTransformMatrix					   = CalyxEngine::Matrix4x4::Multiply(uvTransformMatrix, CalyxEngine::MakeRotateZMatrix(uvTransform.rotate));
		uvTransformMatrix					   = CalyxEngine::Matrix4x4::Multiply(uvTransformMatrix, CalyxEngine::MakeTranslateMatrix(CalyxEngine::Vector3(uvTransform.translate.x, uvTransform.translate.y, 0.0f)));

		TransferMaterial();

		// カメラ行列との掛け合わせ
		// modelData_->vertexBuffer.TransferVectorData(modelData_->meshData.vertices);
		// modelData_->indexBuffer.TransferVectorData(modelData_->meshData.indices);
		Map();
	}
}

void BaseModel::OnModelLoaded() {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();

	if(!modelData_->meshResource.VertexBuffer().IsInitialized()) {
		modelData_->meshResource.VertexBuffer().Initialize(device, UINT(modelData_->meshResource.Vertices().size()));
		modelData_->meshResource.VertexBuffer().TransferVectorData(modelData_->meshResource.Vertices());
	}
	if(!modelData_->meshResource.IndexBuffer().IsInitialized()) {
		modelData_->meshResource.IndexBuffer().Initialize(device, UINT(modelData_->meshResource.Indices().size()));
		modelData_->meshResource.IndexBuffer().TransferVectorData(modelData_->meshResource.Indices());
	}

	// テクスチャ設定。モデル側マテリアルの相対パスは Asset ルート相対に解決済み。
	materialTextureHandles_.clear();
	materialNormalTextureHandles_.clear();
	for(const auto& material : modelData_->meshResource.Materials()) {
		auto h = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTexture(material.textureFilePath);
		if(!h.ptr) {
			h = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTexture("textures/white1x1.dds");
		}
		materialTextureHandles_.push_back(h);

		D3D12_GPU_DESCRIPTOR_HANDLE normalHandle{};
		if(!material.normalTextureFilePath.empty()) {
			normalHandle = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTextureLinear(material.normalTextureFilePath);
		}
		materialNormalTextureHandles_.push_back(normalHandle);
	}
	if(materialTextureHandles_.empty()) {
		materialTextureHandles_.push_back(
			CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTexture("textures/white1x1.dds"));
	}
	if(materialNormalTextureHandles_.empty()) {
		materialNormalTextureHandles_.push_back({});
	}
	if(!handle_) {
		textureName_ = modelData_->meshResource.Material().textureFilePath;
	}

	// -------- インスタンシングバッファの初期確保 --------
	if(!instanceBufferCreated_) {
		instanceBufferCapacity_ = 1024; // 初期インスタンス数（適宜調整）
		instanceBuffer_.Initialize(device, instanceBufferCapacity_);
		instanceBuffer_.CreateSrv(device);
		instanceBufferCreated_ = true;
		currentInstanceBuffer_ = &instanceBuffer_;
	}
}

void BaseModel::UpdateTexture(float deltaTime) {
	if(textureHandles_.size() <= 1) return; // アニメーション不要
	elapsedTime_ += deltaTime;
	if(elapsedTime_ >= animationSpeed_) {
		elapsedTime_ -= animationSpeed_;
		currentFrameIndex_ = (currentFrameIndex_ + 1) % textureHandles_.size();
		handle_			   = textureHandles_[currentFrameIndex_]; // テクスチャを切り替え
	}
}

void BaseModel::EnsureRaytracingBLAS(ID3D12Device5* device5, ID3D12GraphicsCommandList4* cmdList4) {
	if(blasBuilt_) return;
	if(!modelData_) return;
	if(!device5 || !cmdList4) return;

	// VB/IB が初期化されていることが前提（OnModelLoaded 済み）
	rayMesh_.BuildBLAS(device5, cmdList4, *modelData_);
	blasBuilt_ = true;
}

void BaseModel::ShowImGuiInterface() {

	uvTransform.ShowImGui("uvTransform");

	if(GuiCmd::CollapsingHeader("Material")) {
		ImGui::Text("Material GUID: %s", materialGuid_.ToString().c_str());
		// TODO: MaterialAsset の編集 UI や選択 UI を追加

		auto& textures = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->GetLoadedTextures();
		if(ImGui::BeginCombo("Texture", textureName_.c_str())) {
			for(const auto& texture : textures) {
				bool is_selected = (textureName_ == texture.first);
				if(ImGui::Selectable(texture.first.c_str(), is_selected)) {
					textureName_ = texture.first;
					handle_		 = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTexture(texture.first);
				}
				if(is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	if(GuiCmd::CollapsingHeader("Draw")) {
		static const char* blendModeNames[] = {
			"NONE", "ALPHA", "ADD", "SUB", "MUL", "NORMAL", "SCREEN"};

		int currentBlendMode = static_cast<int>(blendMode_);
		if(GuiCmd::Combo("Blend Mode", currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
			blendMode_ = static_cast<BlendMode>(currentBlendMode);
		}
	}
}

void BaseModel::Draw(const WorldTransform& transform) {
	if(!isDrawEnable_) return;
	if(!modelData_) return;
	if(!handle_ && materialTextureHandles_.empty()) return;

	ID3D12GraphicsCommandList* cmdList = GraphicsGroup::GetInstance()->GetCommandList().Get();
	ID3D12Device*			   device  = GraphicsGroup::GetInstance()->GetDevice().Get();
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	std::vector<WorldTransform> singleTransform{transform};
	EnsureInstanceCapacity(device, 1);
	UploadInstanceMatrices(singleTransform);

	GpuBillboardParams billboard{};
	std::vector<GpuBillboardParams> singleBillboard{billboard};
	EnsureBillboardCapacity(device, 1);
	UploadBillboardParams(singleBillboard);

	// マテリアル & インスタンス行列バッファをセット
	materialBuffer_.SetCommand(cmdList, 0);
	cmdList->SetGraphicsRootDescriptorTable(1, GetInstanceSrv());

	cmdList->SetGraphicsRootDescriptorTable(2, GetTexSrv());
	cmdList->SetGraphicsRootDescriptorTable(12, GetMaterialGraphTextureSrvTable(0));
	cmdList->SetGraphicsRootDescriptorTable(14, GetNormalMapSrv());

	// 環境マップ
	D3D12_GPU_DESCRIPTOR_HANDLE envMapHandle = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->GetEnvironmentTextureSrvHandle();
	cmdList->SetGraphicsRootDescriptorTable(6, envMapHandle);
	cmdList->SetGraphicsRootDescriptorTable(7, GetBillboardSrv());

	const auto& subMeshes = modelData_->meshResource.SubMeshes();
	if(subMeshes.empty()) {
		cmdList->DrawIndexedInstanced(UINT(modelData_->meshResource.Indices().size()), 1, 0, 0, 0);
		return;
	}

	for(const auto& subMesh : subMeshes) {
		cmdList->SetGraphicsRootDescriptorTable(2, GetTexSrv(subMesh.materialIndex));
		cmdList->SetGraphicsRootDescriptorTable(12, GetMaterialGraphTextureSrvTable(subMesh.materialIndex));
		cmdList->SetGraphicsRootDescriptorTable(14, GetNormalMapSrv(subMesh.materialIndex));
		cmdList->DrawIndexedInstanced(subMesh.indexCount, 1, subMesh.indexStart, 0, 0);
	}
}

void BaseModel::ApplyConfig(const BaseModelConfig& config) {
	materialGuid_ = config.materialGuid;
	uvTransform.ApplyConfig(config.uvTransConfig);
	blendMode_ = static_cast<BlendMode>(config.blendMode);
	fileName_  = config.modelName;

	bool ok = false;

	// GUID があれば手動上書きとして最優先
	if(config.textureGuid.isValid()) {
		ok = LoadTextureByGuid(config.textureGuid);
	}

	if(!ok && config.legacyTextureName && !config.legacyTextureName->empty()) {
		auto*		db	 = AssetDatabase::GetInstance();
		const auto& view = db->GetView();

		// 旧フィールドは「Assets ルート相対パス」やファイル名の可能性があるので両方見る
		const std::string want = *config.legacyTextureName;
		for(auto* r : view) {
			if(!r || r->type != AssetType::Texture) continue;

			std::error_code	  ec;
			auto			  rel = std::filesystem::relative(r->sourcePath, db->GetRoot(), ec);
			const std::string key = (ec ? r->sourcePath : rel).generic_string();

			if(key == want || r->sourcePath.filename().string() == want) {
				ok = LoadTextureByGuid(r->guid);
				break;
			}
		}
	}

	if(!ok) {
		handle_.reset();
		textureGuid_ = Guid{}; // 未設定
	}
}

BaseModelConfig BaseModel::ExtractConfig() const {
	BaseModelConfig config;
	config.materialGuid = materialGuid_;
	config.uvTransConfig  = uvTransform.ExtractConfig();
	config.blendMode	  = static_cast<int>(blendMode_);
	config.modelName	  = fileName_;

	// 保存は GUID のみ
	config.textureGuid = textureGuid_;
	// config.legacyTextureName は保存しない（後方互換用の読取専用）

	return config;
}

void BaseModel::ShowImGui(BaseModelConfig& config) {
	uvTransform.ShowImGui("uvTransform");

	if(ImGui::TreeNodeEx("Material Asset (Drag & Drop from Assets)", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
		auto labelFromGuid = [](const Guid& g) -> std::string {
			if(!g.isValid()) return "(none)";
			auto* db = AssetDatabase::GetInstance();
			for(auto* r : db->GetView()) {
				if(r && r->type == AssetType::Material && r->guid == g) {
					return r->sourcePath.filename().string();
				}
			}
			return "(missing)";
		};

		Guid droppedGuid = materialGuid_;
		if(CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::Material, &droppedGuid)) {
			const Guid before = materialGuid_;
			if(before != droppedGuid) {
				auto apply = [this, &config](const Guid& guid) {
					config.materialGuid = guid;
					SetMaterialGuid(guid);
				};
				CommandManager::GetInstance()->Execute(
					std::make_unique<ValueEditCommand<Guid>>("Apply Material Asset", before, droppedGuid, apply));
			}
		}

		ImGui::SameLine();
		if(materialGuid_.isValid() && ImGui::SmallButton("Copy GUID##material")) {
			ImGui::SetClipboardText(materialGuid_.ToString().c_str());
		}

		if(auto ma = CalyxEngine::AssetManager::GetInstance()->GetDataAssetManager()->GetAsset<CalyxEngine::MaterialAsset>(materialGuid_)) {
			if(ImGui::TreeNodeEx("Edit Shared Material", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				if(ma->ShowGui()) {
					TransferMaterial();
				}

				if(ImGui::TreeNodeEx("Normal Map Texture", ImGuiTreeNodeFlags_SpanAvailWidth)) {
					Guid droppedNormalMapGuid = ma->normalMapGuid;
					if(CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::Texture, &droppedNormalMapGuid)) {
						if(ma->normalMapGuid != droppedNormalMapGuid) {
							ma->normalMapGuid = droppedNormalMapGuid;
							ma->useNormalMap = droppedNormalMapGuid.isValid();
							TransferMaterial();
						}
					}
					if(ma->normalMapGuid.isValid()) {
						ImGui::SameLine();
						if(ImGui::SmallButton("Clear##normalMap")) {
							ma->normalMapGuid = Guid{};
							ma->useNormalMap = false;
							TransferMaterial();
						}
					}
					ImGui::TreePop();
				}

				auto* db = AssetDatabase::GetInstance();
				for(auto* r : db->GetView()) {
					if(r && r->type == AssetType::Material && r->guid == materialGuid_) {
						if(ImGui::Button("Save Material")) {
							CalyxEngine::AssetManager::GetInstance()->GetDataAssetManager()->SaveAsset(*ma, r->sourcePath);
						}
						break;
					}
				}
				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	if(ImGui::TreeNodeEx("Texture (Drag & Drop from Assets)", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
		// ---- ドラッグ&ドロップでテクスチャ適用 ----
		// ドロップ領域（InvisibleButton で有効アイテム化）
		ImVec2 dropSize(ImGui::GetContentRegionAvail().x, 56.0f);
		ImGui::InvisibleButton("##TextureDrop", dropSize);

		// 見た目（枠とテキスト）
		const bool	 hovered = ImGui::IsItemHovered();
		const ImVec2 rmin	 = ImGui::GetItemRectMin();
		const ImVec2 rmax	 = ImGui::GetItemRectMax();
		ImGui::GetWindowDrawList()->AddRect(
			rmin, rmax, hovered ? IM_COL32(120, 180, 255, 220) : IM_COL32(90, 90, 90, 160),
			8.0f, 0, 2.0f);
		ImGui::GetWindowDrawList()->AddText(
			ImVec2(rmin.x + 8.0f, rmin.y + 8.0f),
			IM_COL32(230, 230, 230, 255),
			"Drop a Texture here");

		// 受け取り
		if(ImGui::BeginDragDropTarget()) {
			if(const ImGuiPayload* p = ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
				const AssetDragPayload payload =
					*reinterpret_cast<const AssetDragPayload*>(p->Data);
				if(payload.type == AssetType::Texture) {
					const Guid before = textureGuid_;
					const Guid after = payload.guid;
					if(before != after && !CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->GetSrvHandle(after).ptr) {
						ImGui::OpenPopup("TextureDropError");
					} else if(before != after) {
						auto apply = [this, &config](const Guid& guid) {
							config.textureGuid = guid;
							SetTextureGuid(guid);
						};
						CommandManager::GetInstance()->Execute(
							std::make_unique<ValueEditCommand<Guid>>("Apply Texture Asset", before, after, apply));
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		// 失敗メッセージ（2D 以外の SRV 等）
		if(ImGui::BeginPopup("TextureDropError")) {
			ImGui::TextUnformatted("このテクスチャは適用できません（2D以外/未対応形式）。");
			ImGui::EndPopup();
		}

		// 現在のテクスチャ表示（GUID→ファイル名）
		auto labelFromGuid = [](const Guid& g) -> std::string {
			if(!g.isValid()) return "(none)";
			auto* db = AssetDatabase::GetInstance();
			for(auto* r : db->GetView()) {
				if(r && r->type == AssetType::Texture && r->guid == g) {
					return r->sourcePath.filename().string();
				}
			}
			return "(missing)";
		};
		ImGui::SameLine();
		if(textureGuid_.isValid() && ImGui::SmallButton("Copy GUID")) {
			ImGui::SetClipboardText(textureGuid_.ToString().c_str());
		}

		ImGui::TreePop();
	}

	// materialData_.ShowImGui(config.materialConfig); // TODO: マテリアルアセットの切り替えUIをここに実装

	// ブレンドモード
	if(ImGui::TreeNodeEx("BlendMode", ImGuiTreeNodeFlags_SpanAvailWidth)) {
		static const char* blendModeNames[] = {
			"NONE", "ALPHA", "ADD", "SUB", "MUL", "NORMAL", "SCREEN"};
		int currentBlendMode = static_cast<int>(blendMode_);
		if(GuiCmd::Combo("Blend Mode", currentBlendMode,
						 blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
			blendMode_		 = static_cast<BlendMode>(currentBlendMode);
			config.blendMode = currentBlendMode;
		}
		ImGui::TreePop();
	}
}

bool BaseModel::LoadTextureByGuid(const Guid& g) {
	if(!g.isValid()) return false;

	auto h = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTexture(g);
	if(!h.ptr) return false;

	handle_		 = h;
	textureGuid_ = g;
	return true;
}

void BaseModel::SetTextureGuid(const Guid& g) {
	if(!g.isValid()) {
		handle_.reset();
		textureGuid_ = Guid::Empty();
		return;
	}

	LoadTextureByGuid(g);
}

void BaseModel::SetMaterialGuid(const Guid& g) {
	materialGuid_ = g;
	TransferMaterial();
}

ModelData* BaseModel::GetModelData() const { return modelData_; }

// ======================================= renderer 専用 ==========================================

void BaseModel::SetTex(const std::string& name) {
	textureName_ = "textures/" + name;
	handle_ = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTexture(textureName_);
}

void BaseModel::EnsureInstanceCapacity(ID3D12Device* device, UINT needCount) {
	if(uploadFrameGenerationSeen_ != gBaseModelUploadFrameGeneration) {
		uploadFrameGenerationSeen_ = gBaseModelUploadFrameGeneration;
		instanceUploadCursor_ = 0;
		billboardUploadCursor_ = 0;
	}

	const size_t slot = instanceUploadCursor_++;
	if(instanceUploadBuffers_.size() <= slot) {
		instanceUploadBuffers_.push_back(std::make_unique<DxStructuredBuffer<TransformationMatrix>>());
	}

	auto& buffer = *instanceUploadBuffers_[slot];
	if(!buffer.IsValid()) {
		const UINT capacity = std::max<UINT>(1024, needCount);
		buffer.Initialize(GraphicsGroup::GetInstance()->GetDevice(), capacity);
		buffer.CreateSrv(device);
	} else if(needCount > buffer.GetElementCount()) {
		const UINT capacity = std::max<UINT>(needCount, buffer.GetElementCount() * 2);
		buffer.ReleaseSrv();
		buffer.Resize(GraphicsGroup::GetInstance()->GetDevice(), capacity);
		buffer.CreateSrv(device);
	}

	currentInstanceBuffer_ = &buffer;
}

void BaseModel::UploadInstanceMatrices(const std::vector<WorldTransform>& transforms) {
	if(!currentInstanceBuffer_) return;

	std::vector<TransformationMatrix> matrices;
	matrices.reserve(transforms.size());
	for(const auto& tf : transforms) {
		TransformationMatrix m{};
		m.world					= tf.matrix.world;
		m.WorldInverseTranspose = CalyxEngine::Matrix4x4::Transpose(CalyxEngine::Matrix4x4::Inverse(tf.matrix.world));
		matrices.push_back(m);
	}
	currentInstanceBuffer_->TransferVectorData(matrices);
}

D3D12_GPU_DESCRIPTOR_HANDLE BaseModel::GetInstanceSrv() const {
	return currentInstanceBuffer_ ? currentInstanceBuffer_->GetGpuSrvHandle() : instanceBuffer_.GetGpuSrvHandle();
}
D3D12_GPU_DESCRIPTOR_HANDLE BaseModel::GetTexSrv() const {
	if(auto material = GetMaterialAsset()) {
		if(material->objectTextureGuid.isValid()) {
			auto h = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTexture(material->objectTextureGuid);
			if(h.ptr) return h;
		}
	}
	if(handle_ && handle_->ptr) {
		return handle_.value();
	}
	if(!materialTextureHandles_.empty() && materialTextureHandles_[0].ptr) {
		return materialTextureHandles_[0];
	}
	return CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTexture("textures/white1x1.dds");
}
D3D12_GPU_DESCRIPTOR_HANDLE BaseModel::GetTexSrv(size_t materialIndex) const {
	if(auto material = GetMaterialAsset()) {
		if(material->objectTextureGuid.isValid()) {
			auto h = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTexture(material->objectTextureGuid);
			if(h.ptr) return h;
		}
	}
	if(handle_ && handle_->ptr) {
		return handle_.value();
	}
	if(materialIndex < materialTextureHandles_.size() && materialTextureHandles_[materialIndex].ptr) {
		return materialTextureHandles_[materialIndex];
	}
	return GetTexSrv();
}

D3D12_GPU_DESCRIPTOR_HANDLE BaseModel::GetNormalMapSrv() const {
	auto* textureManager = CalyxEngine::AssetManager::GetInstance()->GetTextureManager();
	if(auto material = GetMaterialAsset()) {
		if(material->normalMapGuid.isValid()) {
			auto h = textureManager->LoadTextureLinear(material->normalMapGuid);
			if(h.ptr) return h;
		}
	}
	if(!materialNormalTextureHandles_.empty() && materialNormalTextureHandles_[0].ptr) {
		return materialNormalTextureHandles_[0];
	}
	return textureManager->LoadTexture("textures/white1x1.dds");
}

D3D12_GPU_DESCRIPTOR_HANDLE BaseModel::GetNormalMapSrv(size_t materialIndex) const {
	auto* textureManager = CalyxEngine::AssetManager::GetInstance()->GetTextureManager();
	if(auto material = GetMaterialAsset()) {
		if(material->normalMapGuid.isValid()) {
			auto h = textureManager->LoadTextureLinear(material->normalMapGuid);
			if(h.ptr) return h;
		}
	}
	if(materialIndex < materialNormalTextureHandles_.size() && materialNormalTextureHandles_[materialIndex].ptr) {
		return materialNormalTextureHandles_[materialIndex];
	}
	return GetNormalMapSrv();
}

D3D12_GPU_DESCRIPTOR_HANDLE BaseModel::GetMaterialGraphTextureSrvTable(size_t materialIndex) const {
	auto* textureManager = CalyxEngine::AssetManager::GetInstance()->GetTextureManager();
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	if(!textureManager || !device) return {};

	if(materialGraphTextureTables_.size() <= materialIndex) {
		materialGraphTextureTables_.resize(materialIndex + 1);
	}
	DescriptorHandle& textureTable = materialGraphTextureTables_[materialIndex];
	if(!textureTable.IsValid()) {
		textureTable = DescriptorAllocator::AllocateRange(DescriptorUsage::CbvSrvUav, kMaxGraphTextures);
	}

	const UINT descriptorSize = DescriptorAllocator::GetDescriptorSize(DescriptorUsage::CbvSrvUav);
	auto writeFallback = [this, textureManager, materialIndex](D3D12_CPU_DESCRIPTOR_HANDLE dest) {
		if(textureGuid_.isValid() && textureManager->WriteSrvTo(textureGuid_, dest)) return;
		if(handle_ && !textureName_.empty() && textureManager->WriteSrvTo(textureName_, dest)) return;
		if(modelData_) {
			const auto& materials = modelData_->meshResource.Materials();
			if(materialIndex < materials.size() && textureManager->WriteSrvTo(materials[materialIndex].textureFilePath, dest)) return;
			if(!materials.empty() && textureManager->WriteSrvTo(materials[0].textureFilePath, dest)) return;
		}
		if(!textureName_.empty() && textureName_ != "textures/white1x1.dds" && textureManager->WriteSrvTo(textureName_, dest)) return;
		textureManager->WriteSrvTo("textures/white1x1.dds", dest);
	};

	for(uint32_t i = 0; i < kMaxGraphTextures; ++i) {
		D3D12_CPU_DESCRIPTOR_HANDLE dest = textureTable.cpu;
		dest.ptr += static_cast<SIZE_T>(i) * descriptorSize;
		writeFallback(dest);
	}

	if(auto material = GetMaterialAsset()) {
		uint32_t slot = 0;
		for(const auto& node : material->graph.nodes) {
			if(node.type != "ObjectTexture" && node.type != "Texture2D") continue;
			if(slot >= kMaxGraphTextures) break;

			D3D12_CPU_DESCRIPTOR_HANDLE dest = textureTable.cpu;
			dest.ptr += static_cast<SIZE_T>(slot) * descriptorSize;
			if(node.type == "ObjectTexture") {
				if(material->objectTextureGuid.isValid()) {
					textureManager->WriteSrvTo(material->objectTextureGuid, dest);
				}
			} else if(auto it = node.properties.find("textureGuid"); it != node.properties.end() && it->is_string()) {
				const Guid guid = Guid::FromString(it->get<std::string>());
				if(guid.isValid()) {
					textureManager->WriteSrvTo(guid, dest);
				}
			}
			++slot;
		}
	}

	return textureTable.gpu;
}
D3D12_GPU_DESCRIPTOR_HANDLE BaseModel::GetEnvMapSrv() const { return CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->GetEnvironmentTextureSrvHandle(); }

std::shared_ptr<CalyxEngine::MaterialAsset> BaseModel::GetMaterialAsset() const {
	return CalyxEngine::AssetManager::GetInstance()->GetDataAssetManager()->GetAsset<CalyxEngine::MaterialAsset>(materialGuid_);
}

bool BaseModel::UsesRuntimeMaterialGraph() const {
	auto material = GetMaterialAsset();
	if(!material) return false;

	for(const auto& node : material->graph.nodes) {
		if(node.type != "Output") continue;
		for(const auto& pin : node.inputs) {
			if(pin.name != "Surface") continue;
			return std::any_of(material->graph.links.begin(), material->graph.links.end(), [&pin](const auto& graphLink) {
				return graphLink.toPinId == pin.id;
			});
		}
	}
	return false;
}

void BaseModel::BindVertexIndexBuffers(ID3D12GraphicsCommandList* cmdList) const {
	modelData_->meshResource.SetCommand(cmdList);
}

void BaseModel::BindMaterialCB(ID3D12GraphicsCommandList* cmdList) const { materialBuffer_.SetCommand(cmdList, 0); }

// ================= billboard (VS:t1) =================
void BaseModel::EnsureBillboardCapacity(ID3D12Device* device, UINT needCount) {
	if(uploadFrameGenerationSeen_ != gBaseModelUploadFrameGeneration) {
		uploadFrameGenerationSeen_ = gBaseModelUploadFrameGeneration;
		instanceUploadCursor_ = 0;
		billboardUploadCursor_ = 0;
	}

	const size_t slot = billboardUploadCursor_++;
	if(billboardUploadBuffers_.size() <= slot) {
		billboardUploadBuffers_.push_back(std::make_unique<DxStructuredBuffer<GpuBillboardParams>>());
	}

	auto& buffer = *billboardUploadBuffers_[slot];
	if(!buffer.IsValid()) {
		const UINT capacity = std::max<UINT>(needCount, 256u);
		buffer.Initialize(GraphicsGroup::GetInstance()->GetDevice(), capacity);
		buffer.CreateSrv(device);
	} else if(needCount > buffer.GetElementCount()) {
		const UINT capacity = std::max<UINT>(needCount, buffer.GetElementCount() * 2);
		buffer.ReleaseSrv();
		buffer.Resize(GraphicsGroup::GetInstance()->GetDevice(), capacity);
		buffer.CreateSrv(device);
	}

	currentBillboardBuffer_ = &buffer;
}

void BaseModel::UploadBillboardParams(const std::vector<GpuBillboardParams>& params) {
	if(!currentBillboardBuffer_) return;
	if(params.empty()) {
		GpuBillboardParams defaultParam{};
		std::memcpy(currentBillboardBuffer_->Data(), &defaultParam, sizeof(GpuBillboardParams));
		return;
	}
	std::memcpy(currentBillboardBuffer_->Data(), params.data(), sizeof(GpuBillboardParams) * params.size());
}

D3D12_GPU_DESCRIPTOR_HANDLE BaseModel::GetBillboardSrv() const {
	return currentBillboardBuffer_ ? currentBillboardBuffer_->GetGpuSrvHandle() : billboardBuffer_.GetGpuSrvHandle();
}

void BaseModel::TransferMaterial() {
	auto am = CalyxEngine::AssetManager::GetInstance();
	auto ma = am->GetDataAssetManager()->GetAsset<CalyxEngine::MaterialAsset>(materialGuid_);

	Material data{};
	if (ma) {
		data.color = ma->color;
		data.lightingMode = ma->lightingMode;
		data.shininess = ma->shininess;
		data.isReflect = ma->isReflect ? 1 : 0;
		data.envirometCoefficient = ma->envirometCoefficient;
		data.roughness = ma->roughness;
		data.toonHighlightColor = ma->toonHighlightColor;
		data.toonBaseColor = ma->toonBaseColor;
		data.toonMidShadowColor = ma->toonMidShadowColor;
		data.toonShadowColor = ma->toonShadowColor;
		data.toonBaseStep = ma->toonBaseStep;
		data.toonBaseFeather = ma->toonBaseFeather;
		data.toonShadeStep = ma->toonShadeStep;
		data.toonShadeFeather = ma->toonShadeFeather;
		data.toonSpecularThreshold = ma->toonSpecularThreshold;
		data.toonSpecularSoftness = ma->toonSpecularSoftness;
		data.toonSpecularIntensity = ma->toonSpecularIntensity;
		data.emissiveColor = ma->emissiveColor;
		data.emissiveIntensity = ma->emissiveIntensity;
		data.useNormalMap = (ma->useNormalMap || ma->normalMapGuid.isValid()) ? 1 : 0;
		data.normalMapStrength = ma->normalMapStrength;
		data.normalMapFlipY = ma->normalMapFlipY ? 1 : 0;
	} else {
		// Default fallback
		data.color = {1, 1, 1, 1};
		data.lightingMode = 0;
		data.shininess = 20.0f;
	}
	if(data.useNormalMap == 0 && modelData_) {
		for(const auto& material : modelData_->meshResource.Materials()) {
			if(!material.normalTextureFilePath.empty()) {
				data.useNormalMap = 1;
				break;
			}
		}
	}
	if(colorOverride_) {
		data.color = *colorOverride_;
	}

	// UV transform を適用
	CalyxEngine::Matrix4x4 uvTransformMatrix = CalyxEngine::MakeScaleMatrix(CalyxEngine::Vector3(uvTransform.scale.x, uvTransform.scale.y, 1.0f));
	uvTransformMatrix = CalyxEngine::Matrix4x4::Multiply(uvTransformMatrix, CalyxEngine::MakeRotateZMatrix(uvTransform.rotate));
	uvTransformMatrix = CalyxEngine::Matrix4x4::Multiply(uvTransformMatrix, CalyxEngine::MakeTranslateMatrix(CalyxEngine::Vector3(uvTransform.translate.x, uvTransform.translate.y, 0.0f)));
	data.uvTransform = uvTransformMatrix;
	data.pad3 = ClockManager::GetInstance()->GetTotalTime();

	currentMaterial_ = data;
	materialBuffer_.TransferData(data);
}

const CalyxEngine::Vector4& BaseModel::GetColor() const {
	if(colorOverride_) return *colorOverride_;
	auto ma = CalyxEngine::AssetManager::GetInstance()->GetDataAssetManager()->GetAsset<CalyxEngine::MaterialAsset>(materialGuid_);
	if (ma) return ma->color;
	static CalyxEngine::Vector4 fallback = {1, 1, 1, 1};
	return fallback;
}

void BaseModel::SetColor(const CalyxEngine::Vector4& color) {
	colorOverride_ = color;
	TransferMaterial();
}

void BaseModel::SetLightingMode(LightingMode mode) {
	auto ma = CalyxEngine::AssetManager::GetInstance()->GetDataAssetManager()->GetAsset<CalyxEngine::MaterialAsset>(materialGuid_);
	if (ma) ma->lightingMode = static_cast<int32_t>(mode);
}

LightingMode BaseModel::GetLightingMode() const {
	auto ma = CalyxEngine::AssetManager::GetInstance()->GetDataAssetManager()->GetAsset<CalyxEngine::MaterialAsset>(materialGuid_);
	if (ma) return static_cast<LightingMode>(ma->lightingMode);
	return LightingMode::HalfLambert;
}
