#include "GpuFxEmitter.h"
#include <CalyxEngine/Project.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/Module/ModuleConfig.h>
#include <Engine/Application/Effects/FxGuiHelpers.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/Manager/AssetManager.h>
#include <Engine/Assets/System/AssetDragPayload.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <filesystem>

namespace CalyxEngine {
	namespace {
		constexpr const char* kFallbackTexturePath = "Textures/white1x1.dds";

		constexpr uint32_t CeilDiv(uint32_t value, uint32_t divisor) {
			return (value + divisor - 1u) / divisor;
		}

		bool ExistsAssetTexture(const std::string& path) {
			if(path.empty()) return false;
			const std::filesystem::path assetPath = Calyx::ResolveAssetPath(path);
			if(std::filesystem::exists(assetPath)) return true;

			const std::filesystem::path ddsPath = assetPath.parent_path() / (assetPath.stem().string() + ".dds");
			return std::filesystem::exists(ddsPath);
		}

		std::string ResolveTexturePath(const std::string& path) {
			return ExistsAssetTexture(path) ? path : kFallbackTexturePath;
		}
	}

	// ────────────────────────────────────────────────────────────────
	//  ctor / dtor
	// ────────────────────────────────────────────────────────────────
	GpuFxEmitter::~GpuFxEmitter() = default;

	// ────────────────────────────────────────────────────────────────
	//  リソース生成
	// ────────────────────────────────────────────────────────────────
	void GpuFxEmitter::Initialize(){
		static_assert(sizeof(EmitterSphere) == 256);
		static_assert(offsetof(EmitterSphere,initialRotation) == 208);
		static_assert(offsetof(EmitterSphere,previousTranslate) == 224);
		ID3D12Device* dev = GraphicsGroup::GetInstance()->GetDevice().Get();

		// StructuredBuffer を DEFAULT + UAV で確保
		particleBuffer_.InitializeAsRW(dev, kMaxParticles);
		particleBuffer_.CreateUav(dev);   // u0
		particleBuffer_.CreateSrv(dev);   // t0

		paramBuffer_.Initialize(dev);
		paramBuffer_.TransferData(emitParam_);

		material_.color = CalyxEngine::Vector4(1, 1, 1, 1);
		materialBuffer_.Initialize(dev);

		emitterData_.translate = {0, 0, 0};
		emitterData_.radius = 6.0f;
		emitterData_.count = 1024;
		emitterData_.frequency = 0.1f;
		emitterData_.frequencyTime = 0.0f;
		emitterData_.emit = 1;
		emitterData_.scale = {0.1f, 0.1f, 0.1f};
		emitterData_.lifeTime = 3.0f;
		emitterData_.velocity = {0.0f, 1.0f, 0.0f};
		emitterData_.angle = 30.0f;
		emitterData_.color = {1.0f, 1.0f, 1.0f, 1.0f};
		emitterData_.shapeSize = {1.0f, 1.0f, 1.0f};
		emitterData_.shape = static_cast<uint32_t>(EmitterShape::Sphere);
		emitterData_.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
		emitterData_.gravity = {0.0f, -9.8f, 0.0f};
		emitterData_.gravityEnabled = 0;
		emitterData_.overLifeStart = {1.0f, 1.0f, 1.0f, 1.0f};
		emitterData_.overLifeEnd = {1.0f, 1.0f, 1.0f, 1.0f};
		emitterData_.overLifeTarget = 0;
		emitterData_.overLifeBlend = 0;
		emitterData_.overLifeEase = 0;
		emitterData_.overLifeEnabled = 0;
		emitterData_.overLifeClamp = 1;
		emitterData_.overLifeInvert = 0;
		emitterData_.sizeLifeEnabled = 0;
		emitterData_.sizeLifeGrowing = 1;
		emitterData_.sizeLifeEase = 0;
		emitterData_.initialRotation = {};
		emitterData_.previousTranslate = emitterData_.translate;
		emitterData_.complementEnabled = 0;
		emitterData_.complementSpacing = 0.02f;
		emitterData_.complementStartDistance = 0.0f;

		material_.texturePath = kFallbackTexturePath;
		textureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(material_.texturePath);
		noiseMaskTextureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(kFallbackTexturePath);

		emitterParamBuf_.Initialize(dev);
		perFrameBuffer_.Initialize(dev);

		freeListIndexBuffer_.InitializeAsRW(dev, 1);        // u1
		freeListIndexBuffer_.CreateUav(dev);

		freeListBuffer_.InitializeAsRW(dev, kMaxParticles); // u2
		freeListBuffer_.CreateUav(dev);
	}

	// ────────────────────────────────────────────────────────────────
	//  Update: 毎フレーム deltaTime を積む
	// ────────────────────────────────────────────────────────────────
	void GpuFxEmitter::Update(float dt){
		SyncEmitterDataFromBase();
		emitterData_.color = vertexColor_;
		uvElapsedTime_ += isPlaying_ ? dt : 0.0f;
		material_.uvOffsetTiling = {uvSettings_.offset.x,uvSettings_.offset.y,uvSettings_.tiling.x,uvSettings_.tiling.y};
		material_.uvScrollRotationTime = {uvSettings_.scrollSpeed.x,uvSettings_.scrollSpeed.y,uvSettings_.rotation,uvElapsedTime_};

		if(!isPlaying_) {
			emitterData_.emit = 0;
			emitterData_.complementEnabled = 0;
			hasPreviousPosition_ = false;
			complementDistanceRemainder_ = 0.0f;
			perFrame_.deltaTime = dt;
			perFrame_.time += dt;
			perFrameBuffer_.TransferData(perFrame_);
			emitterParamBuf_.TransferData(emitterData_);
			materialBuffer_.TransferData(material_);
			return;
		}
		material_.noiseMaskUv.x += noiseMaskScrollSpeed_.x * dt;
		material_.noiseMaskUv.y += noiseMaskScrollSpeed_.y * dt;

		emitParam_.deltaTime = dt;
		perFrame_.deltaTime = dt;
		perFrame_.time += dt;
		emitterData_.frequencyTime += dt;

		if(complementEnabled_) {
			emitterData_.previousTranslate = previousPosition_;
			emitterData_.complementEnabled = 1;
			emitterData_.complementSpacing = 0.02f / (std::max)(alphaMultiplier_,0.01f);
			const float distance = hasPreviousPosition_ ? (position_ - previousPosition_).Length() : 0.0f;
			const float distanceToFirst = emitterData_.complementSpacing - complementDistanceRemainder_;
			emitterData_.complementStartDistance = distanceToFirst;
			const float accumulatedDistance = complementDistanceRemainder_ + distance;
			const uint32_t requestedCount = static_cast<uint32_t>(accumulatedDistance / emitterData_.complementSpacing);
			emitterData_.count = (std::min)(requestedCount,kMaxParticles);
			emitterData_.emit = emitterData_.count > 0 ? 1u : 0u;
			complementDistanceRemainder_ = std::fmod(accumulatedDistance,emitterData_.complementSpacing);
		} else {
			emitterData_.complementEnabled = 0;
			complementDistanceRemainder_ = 0.0f;
			emitterData_.count = burstEmitCount_;
			const float frequency = (std::max)(emitterData_.frequency, 0.0001f);
			if(frequency <= emitterData_.frequencyTime) {
				emitterData_.frequencyTime -= frequency;
				emitterData_.emit = 1;
			} else {
				emitterData_.emit = 0;
			}
		}
		previousPosition_ = position_;
		hasPreviousPosition_ = true;

		if (perFrame_.time >= 50) {
			perFrame_.time = 0;
		}

		perFrameBuffer_.TransferData(perFrame_);
		emitterParamBuf_.TransferData(emitterData_);
		materialBuffer_.TransferData(material_);
	}

	void GpuFxEmitter::ShowGui() {
		ImGui::PushID(this);

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Quick Controls");
		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();
		ImGui::TextUnformatted("Rate");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120);
		ImGui::DragFloat("##gpu_rate_top", &emitterData_.frequency, 0.01f, 0.01f, 100.0f);
		ImGui::SameLine();
		ImGui::TextUnformatted("Playing");
		ImGui::SameLine();
		ImGui::Checkbox("##gpu_playing_top", &isPlaying_);
		ImGui::SameLine();
		ImGui::TextUnformatted("Draw");
		ImGui::SameLine();
		ImGui::Checkbox("##gpu_draw_top", &isDrawEnable_);

		if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Material)) {
			if(FxGui::GridScope sec{"Material"}; sec.open) {
				FxGui::RowLabel("Color");
				ImGui::ColorEdit4("##gpu_color", &material_.color.x,ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				FxGui::RowLabel("Vertex Color");
				ImGui::ColorEdit4("##gpu_vertex_color",&vertexColor_.x,ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				FxGui::RowLabel("UV Offset");
				ImGui::DragFloat2("##gpu_uv_offset",&uvSettings_.offset.x,0.01f);
				FxGui::RowLabel("UV Tiling");
				ImGui::DragFloat2("##gpu_uv_tiling",&uvSettings_.tiling.x,0.01f,0.001f,100.0f);
				FxGui::RowLabel("UV Scroll Speed");
				ImGui::DragFloat2("##gpu_uv_scroll",&uvSettings_.scrollSpeed.x,0.01f);
				FxGui::RowLabel("UV Rotation");
				float uvRotationDegrees = uvSettings_.rotation * 180.0f / CalyxEngine::kPi;
				if(ImGui::DragFloat("##gpu_uv_rotation",&uvRotationDegrees,0.25f,-360.0f,360.0f,"%.1f deg"))
					uvSettings_.rotation = CalyxEngine::ToRadians(uvRotationDegrees);

				FxGui::RowLabel("Texture");
				ImGui::BeginGroup();
				const ImVec2 dropSize(ImGui::GetContentRegionAvail().x, 56.0f);
				ImGui::InvisibleButton("##GpuTextureDrop", dropSize);

				const bool hovered = ImGui::IsItemHovered();
				const ImVec2 rmin = ImGui::GetItemRectMin();
				const ImVec2 rmax = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddRect(
					rmin, rmax, hovered ? IM_COL32(120, 180, 255, 220) : IM_COL32(90, 90, 90, 160),
					8.0f, 0, 2.0f);
				ImGui::GetWindowDrawList()->AddText(
					ImVec2(rmin.x + 8.0f, rmin.y + 8.0f),
					IM_COL32(230, 230, 230, 255),
					("Texture: " + material_.texturePath).c_str());

				if(ImGui::BeginDragDropTarget()) {
					if(const ImGuiPayload* p = ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
						const AssetDragPayload payload = *reinterpret_cast<const AssetDragPayload*>(p->Data);
						if(payload.type == AssetType::Texture) {
							if(!LoadTextureByGuid(payload.guid)) {
								ImGui::OpenPopup("GpuTextureDropError");
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				if(ImGui::BeginPopup("GpuTextureDropError")) {
					ImGui::TextUnformatted("このテクスチャは適用できません。");
					ImGui::EndPopup();
				}
				ImGui::EndGroup();

				FxGui::RowLabel("Noise Mask Texture");
				ImGui::BeginGroup();
				ImGui::TextUnformatted(material_.noiseMaskParams.x > 0.5f ? "Noise mask attached" : "Drop a noise texture here");
				ImGui::InvisibleButton("##GpuNoiseMaskTextureDrop", ImVec2(ImGui::GetContentRegionAvail().x,40.0f));
				const ImVec2 noiseMin = ImGui::GetItemRectMin();
				const ImVec2 noiseMax = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddRect(noiseMin,noiseMax,ImGui::IsItemHovered() ? IM_COL32(120,180,255,220) : IM_COL32(90,90,90,160),8.0f,0,2.0f);
				if(ImGui::BeginDragDropTarget()) {
					if(const ImGuiPayload* p = ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
						const AssetDragPayload payload = *reinterpret_cast<const AssetDragPayload*>(p->Data);
						if(payload.type == AssetType::Texture) {
							auto handle = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(payload.guid);
							if(handle.ptr) {
								const bool wasAttached = material_.noiseMaskParams.x > 0.5f;
								noiseMaskTextureHandle_ = handle;
								noiseMaskTextureGuid_ = payload.guid;
								noiseMaskTexturePath_.clear();
								if(auto* rec = AssetDatabase::GetInstance()->Get(payload.guid)) noiseMaskTexturePath_ = rec->sourcePath.generic_string();
								material_.noiseMaskParams.x = 1.0f;
								if(!wasAttached) material_.noiseMaskParams.y = 1.0f;
							}
						}
					}
					ImGui::EndDragDropTarget();
				}
				if(material_.noiseMaskParams.x > 0.5f && ImGui::SmallButton("Clear Noise Mask")) {
					noiseMaskTextureGuid_ = Guid::Empty();
					noiseMaskTexturePath_.clear();
					noiseMaskTextureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(kFallbackTexturePath);
					material_.noiseMaskParams.x = 0.0f;
				}
				ImGui::BeginDisabled(material_.noiseMaskParams.x <= 0.5f);
				ImGui::DragFloat("Tiling##gpu_noise_mask", &material_.noiseMaskParams.y, 0.05f, 0.01f, 32.0f);
				ImGui::SliderFloat("Strength##gpu_noise_mask", &material_.noiseMaskParams.z, 0.0f, 1.0f);
				ImGui::SliderFloat("Threshold##gpu_noise_mask", &material_.noiseMaskParams.w, 0.0f, 1.0f);
				ImGui::SliderFloat("Softness##gpu_noise_mask", &material_.noiseMaskUv.z, 0.0001f, 1.0f);
				ImGui::DragFloat2("Scroll Speed##gpu_noise_mask", &noiseMaskScrollSpeed_.x, 0.01f);
				ImGui::EndDisabled();
				if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
					ImGui::SetTooltip("アタッチしたNoise TextureのRチャンネルでAlphaをマスクします。\nThresholdで表示範囲、Softnessで境界、Scroll Speedで流れを調整します。");
				ImGui::EndGroup();
			}
			GuiCmd::EndSection();
		}

		if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::ParticleEmit)) {
			if(FxGui::GridScope sec{"Emitter"}; sec.open) {
				FxGui::RowLabel("Position");
				if(ImGui::DragFloat3("##gpu_position", &emitterData_.translate.x, 0.1f)) {
					position_ = emitterData_.translate;
				}

				FxGui::RowLabel("Shape");
				const char* shapeItems[] = {"Point", "Sphere", "Cone", "Circle", "Box"};
				int shapeIndex = static_cast<int>(emitterData_.shape);
				if(ImGui::Combo("##gpu_shape", &shapeIndex, shapeItems, IM_ARRAYSIZE(shapeItems))) {
					shapeIndex = std::clamp(shapeIndex, 0, static_cast<int>(IM_ARRAYSIZE(shapeItems) - 1));
					emitterData_.shape = static_cast<uint32_t>(shapeIndex);
					shape_ = static_cast<EmitterShape>(shapeIndex);
				}

				FxGui::RowLabel("Shape Size");
				if(ImGui::DragFloat3("##gpu_shape_size", &emitterData_.shapeSize.x, 0.01f, 0.0f, 100.0f)) {
					shapeSize_ = emitterData_.shapeSize;
				}

				FxGui::RowLabel("Radius");
				if(ImGui::DragFloat("##gpu_radius", &emitterData_.radius, 0.01f, 0.0f, 100.0f)) {
					shapeRadius_ = emitterData_.radius;
				}

				FxGui::RowLabel("Angle");
				if(ImGui::DragFloat("##gpu_angle", &emitterData_.angle, 0.1f, 0.0f, 89.0f)) {
					shapeAngle_ = emitterData_.angle;
				}

				FxGui::RowLabel("Emit Count");
				int count = static_cast<int>(burstEmitCount_);
				if(ImGui::DragInt("##gpu_emit_count", &count, 1, 0, static_cast<int>(kMaxParticles))) {
					burstEmitCount_ = static_cast<uint32_t>(std::clamp(count, 0, static_cast<int>(kMaxParticles)));
				}

				FxGui::RowLabel("Complement Trail");
				ImGui::Checkbox("##gpu_complement_trail",&complementEnabled_);

				FxGui::RowLabel("Frequency");
				ImGui::DragFloat("##gpu_frequency", &emitterData_.frequency, 0.01f, 0.01f, 100.0f);
			}
			GuiCmd::EndSection();
		}

		if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Object)) {
			if(FxGui::GridScope sec{"Particle"}; sec.open) {
				FxGui::RowLabel("Scale");
				ImGui::DragFloat3("##gpu_scale", &emitterData_.scale.x, 0.01f, 0.0f, 100.0f);

				FxGui::RowLabel("Velocity");
				ImGui::DragFloat3("##gpu_velocity", &emitterData_.velocity.x, 0.01f);

				FxGui::RowLabel("Life Time");
				ImGui::DragFloat("##gpu_lifetime", &emitterData_.lifeTime, 0.01f, 0.01f, 100.0f);

				FxGui::RowLabel("Initial Rotation");
				Vector3 initialRotationDegrees = initialRotation_ * (180.0f / CalyxEngine::kPi);
				if(ImGui::DragFloat3("##gpu_initial_rotation",&initialRotationDegrees.x,0.25f,-360.0f,360.0f,"%.1f deg"))
					initialRotation_ = initialRotationDegrees * (CalyxEngine::kPi / 180.0f);
			}
			GuiCmd::EndSection();
		}

		if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::ParticleModule)) {
			if(FxGui::GridScope sec{"GPU Modules"}; sec.open) {
				bool gravityEnabled = emitterData_.gravityEnabled != 0;
				FxGui::RowLabel("Gravity");
				if(ImGui::Checkbox("##gpu_gravity_enabled", &gravityEnabled)) {
					emitterData_.gravityEnabled = gravityEnabled ? 1u : 0u;
				}
				FxGui::RowLabel("Gravity Accel");
				ImGui::DragFloat3("##gpu_gravity", &emitterData_.gravity.x, 0.01f);

				bool sizeLifeEnabled = emitterData_.sizeLifeEnabled != 0;
				FxGui::RowLabel("Size Over Life");
				if(ImGui::Checkbox("##gpu_size_life_enabled", &sizeLifeEnabled)) {
					emitterData_.sizeLifeEnabled = sizeLifeEnabled ? 1u : 0u;
				}
				bool sizeGrowing = emitterData_.sizeLifeGrowing != 0;
				FxGui::RowLabel("Size Growing");
				if(ImGui::Checkbox("##gpu_size_growing", &sizeGrowing)) {
					emitterData_.sizeLifeGrowing = sizeGrowing ? 1u : 0u;
				}

				bool overLifeEnabled = emitterData_.overLifeEnabled != 0;
				FxGui::RowLabel("Over Lifetime");
				if(ImGui::Checkbox("##gpu_over_life_enabled", &overLifeEnabled)) {
					emitterData_.overLifeEnabled = overLifeEnabled ? 1u : 0u;
				}
				FxGui::RowLabel("Target");
				const char* targetItems[] = {"Scale", "Unused1", "Unused2", "Unused3", "Color", "Alpha"};
				int target = static_cast<int>(emitterData_.overLifeTarget);
				if(ImGui::Combo("##gpu_over_life_target", &target, targetItems, IM_ARRAYSIZE(targetItems))) {
					emitterData_.overLifeTarget = static_cast<uint32_t>(std::clamp(target, 0, 5));
				}
				FxGui::RowLabel("Start");
				ImGui::ColorEdit4("##gpu_over_life_start", &emitterData_.overLifeStart.x);
				FxGui::RowLabel("End");
				ImGui::ColorEdit4("##gpu_over_life_end", &emitterData_.overLifeEnd.x);
			}
			GuiCmd::EndSection();
		}

		ImGui::PopID();
	}
	void GpuFxEmitter::ApplyConfigFrom(const EmitterConfig& config) {
		position_ = config.position;
		worldRotation_ = config.rotation;
		worldScale_ = config.worldScale;
		emitterData_.translate = config.position;
		material_.color = config.color;
		vertexColor_ = config.vertexColor;
		uvSettings_ = config.uvSettings;
		uvElapsedTime_ = 0.0f;
		material_.uvOffsetTiling = {uvSettings_.offset.x,uvSettings_.offset.y,uvSettings_.tiling.x,uvSettings_.tiling.y};
		material_.uvScrollRotationTime = {uvSettings_.scrollSpeed.x,uvSettings_.scrollSpeed.y,uvSettings_.rotation,0.0f};
		noiseMaskTextureGuid_ = config.noiseMaskTextureGuid;
		noiseMaskTexturePath_ = config.noiseMaskTexturePath;
		noiseMaskTextureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(kFallbackTexturePath);
		bool hasNoiseMask = false;
		if(noiseMaskTextureGuid_.isValid()) {
			auto handle = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(noiseMaskTextureGuid_);
			if(handle.ptr) { noiseMaskTextureHandle_ = handle; hasNoiseMask = true; }
		} else if(!noiseMaskTexturePath_.empty()) {
			auto handle = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(noiseMaskTexturePath_);
			if(handle.ptr) { noiseMaskTextureHandle_ = handle; hasNoiseMask = true; }
		}
		material_.noiseMaskParams = {
			hasNoiseMask ? 1.0f : 0.0f,
			config.noiseMaskScale,
			config.noiseMaskStrength,
			config.noiseMaskThreshold};
		material_.noiseMaskUv = {0.0f,0.0f,config.noiseMaskSoftness,0.0f};
		noiseMaskScrollSpeed_ = config.noiseMaskScrollSpeed;
		material_.texturePath = ResolveTexturePath(config.texturePath);
		textureGuid_ = config.textureGuid;
		if(textureGuid_.isValid()) {
			textureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(textureGuid_);
			if(!textureHandle_.ptr) {
				textureGuid_ = Guid::Empty();
				textureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(material_.texturePath);
			}
		} else {
			textureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(material_.texturePath);
		}
		modelPath = config.modelPath;
		modelGuid_ = config.modelGuid;
		if(modelGuid_.isValid()) {
			LoadModelByGuid(modelGuid_);
		}

		isDrawEnable_ = config.isDrawEnable;
		blendMode_ = config.blendMode;
		shape_ = config.emitterShape;
		shapeSize_ = config.shapeSize;
		shapeRadius_ = config.shapeRadius;
		shapeAngle_ = config.shapeAngle;
		burstEmitCount_ = static_cast<uint32_t>((std::max)(config.emitCount, 0));
		emitterData_.count = burstEmitCount_;
		complementEnabled_ = config.isComplement;
		hasPreviousPosition_ = false;
		complementDistanceRemainder_ = 0.0f;
		emitterData_.frequency = (std::max)(config.emitRate, 0.0001f);
		emitterData_.radius = shapeRadius_;
		emitterData_.scale = config.scale.constant;
		emitterData_.velocity = config.velocity.constant;
		emitterData_.lifeTime = (std::max)(config.lifetime.constant, 0.01f);
		emitterData_.color = vertexColor_;
		emitterData_.shapeSize = shapeSize_;
		emitterData_.angle = shapeAngle_;
		emitterData_.shape = static_cast<uint32_t>(shape_);
		emitterData_.rotation = {worldRotation_.x, worldRotation_.y, worldRotation_.z, worldRotation_.w};
		initialRotation_ = config.initialRotation;
		emitterData_.initialRotation = initialRotation_;
		emitterData_.gravityEnabled = 0;
		emitterData_.overLifeEnabled = 0;
		emitterData_.sizeLifeEnabled = 0;

		for(const auto& module : config.modules) {
			if(!module || !module->enabled) continue;

			if(auto* gravity = dynamic_cast<const GravityModuleConfig*>(module.get())) {
				emitterData_.gravity = gravity->gravity;
				emitterData_.gravityEnabled = 1;
				continue;
			}

			if(auto* size = dynamic_cast<const SizeOverLifetimeConfig*>(module.get())) {
				emitterData_.sizeLifeEnabled = 1;
				emitterData_.sizeLifeGrowing = size->isGrowing ? 1u : 0u;
				emitterData_.sizeLifeEase = static_cast<uint32_t>(size->easeType);
				continue;
			}

			if(auto* overLife = dynamic_cast<const OverLifetimeModuleConfig*>(module.get())) {
				if(overLife->target == 0 || overLife->target == 4 || overLife->target == 5) {
					emitterData_.overLifeEnabled = 1;
					emitterData_.overLifeTarget = static_cast<uint32_t>(overLife->target);
					emitterData_.overLifeBlend = static_cast<uint32_t>(overLife->blend);
					emitterData_.overLifeEase = static_cast<uint32_t>(overLife->ease);
					emitterData_.overLifeClamp = overLife->clamp01 ? 1u : 0u;
					emitterData_.overLifeInvert = overLife->invert ? 1u : 0u;
					emitterData_.overLifeStart = overLife->start;
					emitterData_.overLifeEnd = overLife->end;
				}
			}
		}
	}
	void GpuFxEmitter::ExtractConfigTo(EmitterConfig& config) const {
		config.position = position_;
		config.rotation = worldRotation_;
		config.worldScale = worldScale_;
		config.color = material_.color;
		config.vertexColor = vertexColor_;
		config.uvSettings = uvSettings_;
		config.texturePath = material_.texturePath;
		config.textureGuid = textureGuid_;
		config.noiseMaskTextureGuid = noiseMaskTextureGuid_;
		config.noiseMaskTexturePath = noiseMaskTexturePath_;
		config.noiseMaskScale = material_.noiseMaskParams.y;
		config.noiseMaskStrength = material_.noiseMaskParams.z;
		config.noiseMaskThreshold = material_.noiseMaskParams.w;
		config.noiseMaskSoftness = material_.noiseMaskUv.z;
		config.noiseMaskScrollSpeed = noiseMaskScrollSpeed_;
		config.modelPath = modelPath;
		config.modelGuid = modelGuid_;
		config.isDrawEnable = isDrawEnable_;
		config.blendMode = blendMode_;
		config.emitterShape = static_cast<EmitterShape>(emitterData_.shape);
		config.shapeSize = emitterData_.shapeSize;
		config.shapeRadius = emitterData_.radius;
		config.shapeAngle = emitterData_.angle;
		config.emitCount = static_cast<int>(burstEmitCount_);
		config.emitRate = emitterData_.frequency;
		config.isComplement = complementEnabled_;
		config.scale.mode = FxValueMode::Constant;
		config.scale.constant = emitterData_.scale;
		config.velocity.mode = FxValueMode::Constant;
		config.velocity.constant = emitterData_.velocity;
		config.lifetime.mode = FxValueMode::Constant;
		config.lifetime.constant = emitterData_.lifeTime;
		config.initialRotation = initialRotation_;

		config.modules.clear();
		if(emitterData_.gravityEnabled != 0) {
			auto gravity = std::make_unique<GravityModuleConfig>();
			gravity->enabled = true;
			gravity->gravity = emitterData_.gravity;
			config.modules.push_back(std::move(gravity));
		}
		if(emitterData_.sizeLifeEnabled != 0) {
			auto size = std::make_unique<SizeOverLifetimeConfig>();
			size->enabled = true;
			size->isGrowing = emitterData_.sizeLifeGrowing != 0;
			size->easeType = static_cast<EaseType>(emitterData_.sizeLifeEase);
			config.modules.push_back(std::move(size));
		}
		if(emitterData_.overLifeEnabled != 0) {
			auto overLife = std::make_unique<OverLifetimeModuleConfig>();
			overLife->enabled = true;
			overLife->target = static_cast<int>(emitterData_.overLifeTarget);
			overLife->blend = static_cast<int>(emitterData_.overLifeBlend);
			overLife->ease = static_cast<int>(emitterData_.overLifeEase);
			overLife->clamp01 = emitterData_.overLifeClamp != 0;
			overLife->invert = emitterData_.overLifeInvert != 0;
			overLife->start = emitterData_.overLifeStart;
			overLife->end = emitterData_.overLifeEnd;
			config.modules.push_back(std::move(overLife));
		}
	}

	void GpuFxEmitter::SyncEmitterDataFromBase() {
		emitterData_.translate = position_;
		emitterData_.rotation = {worldRotation_.x, worldRotation_.y, worldRotation_.z, worldRotation_.w};
		emitterData_.initialRotation = initialRotation_;
	}

	void GpuFxEmitter::DrawEmitterShape(const WorldTransform&) {
		DrawEmitterShapeInternal(false);
	}

	void GpuFxEmitter::DrawEmitterShapePreview(const WorldTransform&) {
		DrawEmitterShapeInternal(true);
	}

	void GpuFxEmitter::DrawEmitterShapeInternal(bool effectPreview) {
		const CalyxEngine::Vector3 absScale{
			(std::max)(std::abs(worldScale_.x), 0.0001f),
			(std::max)(std::abs(worldScale_.y), 0.0001f),
			(std::max)(std::abs(worldScale_.z), 0.0001f)};

		const CalyxEngine::Vector4 color{0.2f, 0.7f, 1.0f, 1.0f};
		const auto shape = static_cast<EmitterShape>(std::clamp(
			static_cast<int>(emitterData_.shape),
			static_cast<int>(EmitterShape::Point),
			static_cast<int>(EmitterShape::Box)));

		switch(shape) {
		case EmitterShape::Circle: {
			const float radiusX = emitterData_.radius * absScale.x;
			const float radiusZ = emitterData_.radius * absScale.z;
			if(effectPreview) {
				PrimitiveDrawer::GetInstance()->DrawEffectPreviewCircle(position_,worldRotation_,radiusX,radiusZ,color);
			} else {
				PrimitiveDrawer::GetInstance()->DrawCircle(position_,worldRotation_,radiusX,radiusZ,color);
			}
		}
		break;

		case EmitterShape::Cone: {
			const float height = (std::max)(emitterData_.radius,0.0f) * absScale.y;
			const float angleRad = std::clamp(CalyxEngine::ToRadians(emitterData_.angle),0.0f,CalyxEngine::kPi * 0.5f);
			const float radiusX = height * std::tan(angleRad) * absScale.x;
			const float radiusZ = height * std::tan(angleRad) * absScale.z;
			if(effectPreview) {
				PrimitiveDrawer::GetInstance()->DrawEffectPreviewCone(position_,worldRotation_,height,radiusX,radiusZ,color);
			} else {
				PrimitiveDrawer::GetInstance()->DrawCone(position_,worldRotation_,height,radiusX,radiusZ,color);
			}
		}
		break;

		case EmitterShape::Sphere: {
			const float maxScale = (std::max)((std::max)(absScale.x,absScale.y),absScale.z);
			if(effectPreview) {
				PrimitiveDrawer::GetInstance()->DrawEffectPreviewSphere(position_,emitterData_.radius * maxScale,4,color);
			} else {
				PrimitiveDrawer::GetInstance()->DrawSphere(position_,emitterData_.radius * maxScale,4,color);
			}
		}
		break;

		case EmitterShape::Box: {
			const CalyxEngine::Vector3 scaledSize{
				emitterData_.shapeSize.x * absScale.x,
				emitterData_.shapeSize.y * absScale.y,
				emitterData_.shapeSize.z * absScale.z};
			if(effectPreview) {
				PrimitiveDrawer::GetInstance()->DrawEffectPreviewBox(position_,worldRotation_,scaledSize,color);
			} else {
				PrimitiveDrawer::GetInstance()->DrawBox(position_,worldRotation_,scaledSize,color);
			}
		}
		break;

		default:
			break;
		}
	}

	void GpuFxEmitter::Play() {
		isPlaying_ = true;
		hasPreviousPosition_ = false;
		complementDistanceRemainder_ = 0.0f;
	}

	void GpuFxEmitter::Stop() {
		isPlaying_ = false;
	}

	void GpuFxEmitter::Reset() {
		isInitialized = false;
		emitterData_.frequencyTime = 0.0f;
		perFrame_ = {};
		uvElapsedTime_ = 0.0f;
		material_.noiseMaskUv.x = 0.0f;
		material_.noiseMaskUv.y = 0.0f;
		hasPreviousPosition_ = false;
		complementDistanceRemainder_ = 0.0f;
	}

	bool GpuFxEmitter::LoadTextureByGuid(const Guid& g) {
		if(!g.isValid()) return false;
		auto h = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(g);
		if(!h.ptr) return false;
		textureHandle_ = h;
		textureGuid_ = g;
		return true;
	}

	void GpuFxEmitter::SetTextureGuid(const Guid& g) {
		if(!g.isValid()) {
			textureGuid_ = Guid::Empty();
			material_.texturePath = ResolveTexturePath(material_.texturePath);
			textureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(material_.texturePath);
			return;
		}
		LoadTextureByGuid(g);
	}

	void GpuFxEmitter::SetPosition(const CalyxEngine::Vector3& pos) {
		position_ = pos;
		emitterData_.translate = pos;
	}

	// ────────────────────────────────────────────────────────────────
	//  Dispatch: CS でパーティクル更新
	// ────────────────────────────────────────────────────────────────
	void GpuFxEmitter::DispatchInitialize(ID3D12GraphicsCommandList* cmd){
		if (!cmd || isInitialized) return;

		auto* res = particleBuffer_.GetResource().Get();
		auto* counterRes = freeListIndexBuffer_.GetResource().Get();
		auto* listRes = freeListBuffer_.GetResource().Get();

		const D3D12_RESOURCE_STATES beforeState = hasInitializedOnce_
			? (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
			: D3D12_RESOURCE_STATE_COMMON;

		// 1) readable/common -> UAV
		CD3DX12_RESOURCE_BARRIER toUav[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				res,
				beforeState,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
			CD3DX12_RESOURCE_BARRIER::Transition(
				counterRes,
				beforeState,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
			CD3DX12_RESOURCE_BARRIER::Transition(
				listRes,
				beforeState,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		};
		cmd->ResourceBarrier(_countof(toUav), toUav);

		// 2) b0: 定数
		paramBuffer_.SetCompute(cmd, 0);
		cmd->SetComputeRootDescriptorTable(1, particleBuffer_.GetGpuUavHandle());
		cmd->SetComputeRootDescriptorTable(2, freeListIndexBuffer_.GetGpuUavHandle()); // u1: Counter
		cmd->SetComputeRootDescriptorTable(3, freeListBuffer_.GetGpuUavHandle()); // u1: Counter

		cmd->Dispatch(CeilDiv(kMaxParticles, 256u), 1, 1);

		// 5) UAV -> readable state used by later transitions/draw
		CD3DX12_RESOURCE_BARRIER toSrv[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				res,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(
				counterRes,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(
				listRes,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		};
		cmd->ResourceBarrier(_countof(toSrv), toSrv);

		isInitialized = true;
		hasInitializedOnce_ = true;
	}

	void GpuFxEmitter::DispatchEmit(ID3D12GraphicsCommandList* cmd){
		if (!cmd) return;
		if(emitterData_.emit == 0 || emitterData_.count == 0) return;

		auto* res = particleBuffer_.GetResource().Get();
		auto* counterRes = freeListIndexBuffer_.GetResource().Get();
		auto* listRes = freeListBuffer_.GetResource().Get();

		// ── SRV → UAV 遷移 ───────────────────────────────
		CD3DX12_RESOURCE_BARRIER toUav[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				res,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

			CD3DX12_RESOURCE_BARRIER::Transition(
				counterRes,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

			CD3DX12_RESOURCE_BARRIER::Transition(
				listRes,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		};
		cmd->ResourceBarrier(_countof(toUav), toUav);

		// ── Root 設定 ────────────────────────────────────
		emitterParamBuf_.SetCompute(cmd, 0);       // b0: Emitter
		perFrameBuffer_.SetCompute(cmd, 1);        // b1: PerFrame
		cmd->SetComputeRootDescriptorTable(2, particleBuffer_.GetGpuUavHandle());   // u0: Particles
		cmd->SetComputeRootDescriptorTable(3, freeListIndexBuffer_.GetGpuUavHandle()); // u1: listIndex
		cmd->SetComputeRootDescriptorTable(4, freeListBuffer_.GetGpuUavHandle()); // u2: list

		// ── Dispatch（1スレッドでEmit）────────────────────
		cmd->Dispatch(CeilDiv((std::min)(emitterData_.count, kMaxParticles), 1024u), 1, 1);

		// ── UAV → SRV に戻す ─────────────────────────────
		CD3DX12_RESOURCE_BARRIER toSrv[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				res,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

			CD3DX12_RESOURCE_BARRIER::Transition(
				counterRes,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

			CD3DX12_RESOURCE_BARRIER::Transition(
				listRes,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		};
		cmd->ResourceBarrier(_countof(toSrv), toSrv);
	}

	void GpuFxEmitter::DispatchUpdate(ID3D12GraphicsCommandList* cmd){
		if (!cmd) return;

		auto* res = particleBuffer_.GetResource().Get();
		auto* counterRes = freeListIndexBuffer_.GetResource().Get();
		auto* listRes = freeListBuffer_.GetResource().Get();

		// ── SRV → UAV 遷移 ───────────────────────────────
		CD3DX12_RESOURCE_BARRIER toUav[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				res,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

			CD3DX12_RESOURCE_BARRIER::Transition(
				counterRes,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

			CD3DX12_RESOURCE_BARRIER::Transition(
				listRes,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		};

		cmd->ResourceBarrier(_countof(toUav), toUav);

		// ── Root 設定 ────────────────────────────────────
		perFrameBuffer_.SetCompute(cmd, 0);        // b0: PerFrame
		emitterParamBuf_.SetCompute(cmd, 1);       // b1: Emitter/module params
		cmd->SetComputeRootDescriptorTable(2, particleBuffer_.GetGpuUavHandle());		// u0: Particles
		cmd->SetComputeRootDescriptorTable(3, freeListIndexBuffer_.GetGpuUavHandle());	// u1: listIndex
		cmd->SetComputeRootDescriptorTable(4, freeListBuffer_.GetGpuUavHandle());		// u2: list

		// ── Dispatch ────────────────────
		cmd->Dispatch(CeilDiv(kMaxParticles, 1024u), 1, 1);

		// ── UAV → SRV に戻す ─────────────────────────────
		CD3DX12_RESOURCE_BARRIER toSrv[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				res,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

			CD3DX12_RESOURCE_BARRIER::Transition(
				counterRes,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

			CD3DX12_RESOURCE_BARRIER::Transition(
				listRes,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		};
		cmd->ResourceBarrier(_countof(toSrv), toSrv);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GpuFxEmitter::GetParticleSrv() const {
		return particleBuffer_.GetGpuSrvHandle();
	}

	uint32_t GpuFxEmitter::GetDrawInstanceCount() const {
		// Complement emission can allocate an arbitrary number of particles per frame.
		// Until the renderer uses an alive-count indirect argument, the full pool is
		// required so particles outside the normal rate-based estimate stay visible.
		if(complementEnabled_) return kMaxParticles;
		const float frequency = (std::max)(emitterData_.frequency, 0.0001f);
		const float lifeTime = (std::max)(emitterData_.lifeTime, 0.01f);
		const uint64_t burstCount = (std::max)(emitterData_.count, 1u);
		const uint64_t activeBurstEstimate = static_cast<uint64_t>(std::ceil(lifeTime / frequency)) + 1u;
		const uint64_t estimatedInstances = burstCount * activeBurstEstimate;
		const uint64_t clamped = (std::min<uint64_t>)(estimatedInstances, kMaxParticles);
		return static_cast<uint32_t>(clamped);
	}
}
