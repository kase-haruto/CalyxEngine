#include "FxEmitter.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/FxGuiHelpers.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Module/Factory/ModuleFactory.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/System/AssetDragPayload.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
// externals
#include "Engine/Application/UI/Panels/InspectorPanel.h"
#include "Engine/Assets/Manager/AssetManager.h"
#include "Engine/Assets/Texture/TextureManager.h"
#include "Engine/Foundation/Math/MathUtil.h"
#include "Engine/Foundation/Utility/Converter/EnumConverter.h"
#include "Engine/Graphics/Camera/Manager/CameraManager.h"
#include "../Detail/ParticleDetail.h"

#include <externals/imgui/ImGuiFileDialog.h>
#include <externals/imgui/imgui.h>

#include <algorithm>
#include <cmath>

namespace {
	[[maybe_unused]] void VSeparator(float height = 0.0f,float thickness = 1.0f,float pad = 6.0f) {
		ImVec2 pos = ImGui::GetCursorScreenPos();
		if(height <= 0.0f) height = ImGui::GetTextLineHeightWithSpacing();

		ImU32       col = ImGui::GetColorU32(ImGuiCol_Separator);
		ImDrawList* dl  = ImGui::GetWindowDrawList();
		float       x   = pos.x + pad * 0.5f;
		dl->AddLine(ImVec2(x,pos.y),ImVec2(x,pos.y + height),col,thickness);

		ImGui::Dummy(ImVec2(pad + thickness,height));
		ImGui::SameLine();
	}
}; // namespace

namespace CalyxEngine {
	/////////////////////////////////////////////////////////////////////////////////////////
	// ctor / dtor
	/////////////////////////////////////////////////////////////////////////////////////////
	FxEmitter::FxEmitter() {
		ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();

		// マテリアル
		material_.color = CalyxEngine::Vector4(1,1,1,1);
		materialBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice());
		billboardCB_.Initialize(GraphicsGroup::GetInstance()->GetDevice());
		fadeCB_.Initialize(GraphicsGroup::GetInstance()->GetDevice());

		instanceBuffer_.Initialize(device,kMaxUnits_);
		instanceBuffer_.CreateSrv(device);

		// ビルボード定数バッファ初期化
		billboardParams_.mode = static_cast<uint32_t>(billboardMode_);
		billboardCB_.Initialize(device);
		billboardCB_.TransferData(billboardParams_);
		fadeCB_.TransferData(fadeParams_);
		noiseMaskTextureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture("Textures/white1x1.dds");

		// 各種パラメータ
		velocity_ = FxParam<CalyxEngine::Vector3>::MakeRandom(CalyxEngine::Vector3(-1.0f,0.0f,-1.0f),
															CalyxEngine::Vector3(1.0f,0.0f,1.0f));
		direction_      = FxParam<CalyxEngine::Vector3>::MakeConstant(CalyxEngine::Vector3(0.0f,1.0f,0.0f));
		directionSpeed_ = FxParam<float>::MakeConstant(1.0f);
		lifetime_ = FxParam<float>::MakeRandom(1.0f,3.0f);
		scale_    = FxParam<CalyxEngine::Vector3>::MakeConstant();
		spin_     = FxParam<CalyxEngine::Vector3>::MakeConstant(CalyxEngine::Vector3(0.0f,0.0f,0.0f));

		moduleContainer_ = std::make_unique<CalyxEngine::FxModuleContainer>();
	}

	FxEmitter::~FxEmitter() { instanceBuffer_.ReleaseSrv(); }

	/////////////////////////////////////////////////////////////////////////////////////////
	// Update
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxEmitter::Update(float deltaTime) {
		// ---- プレビュー ----
		if(isOneShot_ && timedPreview_) {
			previewTimer_ += deltaTime;
			if(previewTimer_ >= previewIntervalSec_) {
				previewTimer_ = 0.0f;
				RestartOneShot();
			}
		}

		trailEmitter_.Update(position_,worldRotation_,deltaTime,HasFlag(Playing));
		if(!HasFlag(Playing)) return;
		uvElapsedTime_ += deltaTime;
		material_.uvOffsetTiling = {uvSettings_.offset.x,uvSettings_.offset.y,uvSettings_.tiling.x,uvSettings_.tiling.y};
		material_.uvScrollRotationTime = {uvSettings_.scrollSpeed.x,uvSettings_.scrollSpeed.y,uvSettings_.rotation,uvElapsedTime_};
		material_.noiseMaskUv.x += noiseMaskScrollSpeed_.x * deltaTime;
		material_.noiseMaskUv.y += noiseMaskScrollSpeed_.y * deltaTime;

		position_ = GetWorldPosition();
		elapsedTime_ += deltaTime;

		if(elapsedTime_ < emitDelay_) return;

		// ---- 発生停止判定 ----
		if(emitDuration_ >= 0.0f && elapsedTime_ > emitDelay_ + emitDuration_) { Stop(); }

		// ---- パーティクル発生 ----
		if(isOneShot_) {
			if(!hasEmitted_) {
				for(int i = 0; i < emitCount_ && units_.size() < kMaxUnits_; ++i) { Emit(); }
				hasEmitted_ = true;
			}
		} else {
			if(HasFlag(FirstFrame)) {
				prevPostion_ = position_;
				SetFlag(FirstFrame,false);
			}

			CalyxEngine::Vector3 moveDelta = position_ - prevPostion_;
			float              distance  = moveDelta.Length();

			if(distance > 0.0f && HasFlag(Complement)) {
				// 密度を alphaMultiplier でスケーリング (0.01以下にならないようにガード)
				float spawnInterval = 0.02f / (std::max)(alphaMultiplier_,0.01f);
				int   trailCount    = static_cast<int>(distance / spawnInterval);
				for(int i = 0; i < trailCount; ++i) {
					float              dist     = static_cast<float>(i) * spawnInterval;
					float              t        = dist / distance;
					CalyxEngine::Vector3 spawnPos = CalyxEngine::Vector3::Lerp(prevPostion_,position_,t);
					Emit(GenerateSpawnPosition(spawnPos, randomStream_) + offset_);
				}
			} else {
				emitTimer_ += deltaTime;
				// 発生レートも alphaMultiplier でスケーリング
				const float interval = emitRate_ / (std::max)(alphaMultiplier_,0.01f);
				if(emitTimer_ >= interval && units_.size() < kMaxUnits_) {
					emitTimer_ -= interval;
					Emit();
				}
			}
			prevPostion_ = position_;
		}

		// ---- 各パーティクル更新 ----
		for(auto& fx : units_) {
			if(!fx.alive) continue;
			if(fx.lifetime > 0.0f) {
				float t  = fx.age / fx.lifetime;
				fx.lifeT = std::clamp(t,0.0f,1.0f);
			} else { fx.lifeT = 1.0f; }

			for(auto* module : moduleContainer_->GetUpdateModules()) {
				if(module->IsEnabled()) module->OnUpdate(fx,deltaTime);
			}

			// 追従フラグが立っている場合はエミッタ位置+オフセットに常に合わせる（速度適用は行わない）
			if(fx.followEmitter) { fx.position = position_ + fx.followOffset; } else { fx.position += fx.velocity * deltaTime; }

			// スピン
			fx.rotationEuler += fx.spinSpeed * deltaTime;

			fx.age += deltaTime;
			if(fx.age >= fx.lifetime) fx.alive = false;

		}

		ParticleMaterial renderMat = material_;
		renderMat.color.w *= alphaMultiplier_;
		materialBuffer_.TransferData(renderMat);
		billboardCB_.TransferData(billboardParams_);
		fadeCB_.TransferData(fadeParams_);
		std::erase_if(units_,[](const FxUnit& fx) { return !fx.alive; });

		bool shouldNotify =
			(isOneShot_ && hasEmitted_ && units_.empty() && trailEmitter_.History().Empty()) ||
			(emitDuration_ >= 0.0f && elapsedTime_ > emitDelay_ + emitDuration_ && units_.empty() && trailEmitter_.History().Empty());

		if(shouldNotify && !isFinishedNotified_) {
			isFinishedNotified_ = true;
			Stop();
			if(onFinished_) onFinished_();
		}

		// ---- Billboardモード転送 ----
		billboardParams_.mode = static_cast<uint32_t>(billboardMode_);
		billboardCB_.TransferData(billboardParams_);

	}

	/////////////////////////////////////////////////////////////////////////////////////////
	// Emit / Reset
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxEmitter::TransferParticleDataToGPU() {
		if(units_.empty()) return;

		std::vector<ParticleConstantData> gpuUnits;
		gpuUnits.reserve(units_.size());

		for(const auto& fx : units_) {
			if(!fx.alive) continue;

			ParticleConstantData data{};
			data.position = fx.position;
			data.scale    = fx.scale;
			data.color    = fx.color;
			data.vertexColor = fx.vertexColor;
			data.flipbookScaleOffset = {fx.uvTransform.scale.x,fx.uvTransform.scale.y,fx.uvTransform.translate.x,fx.uvTransform.translate.y};
			data.emissiveColor = fx.emissiveColor;
			data.emissiveIntensity = fx.emissiveIntensity;
			data.rotation = fx.rotationEuler;
			data.alignDirection = fx.alignDirection;
			data.alignToDirection = fx.alignToDirection ? 1u : 0u;

			gpuUnits.push_back(data);
		}

		if(!gpuUnits.empty()) { instanceBuffer_.TransferVectorData(gpuUnits); }
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	// Emit / Reset
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxEmitter::Emit() {
		// 形状が点の時はエミッタの位置、そうでない時は形状に応じた位置を生成して発生させる
		Emit(GenerateSpawnPosition(randomStream_) + offset_);
	}

	void FxEmitter::Emit(const CalyxEngine::Vector3& pos) {
		if(units_.size() >= kMaxUnits_) return;
		FxUnit fx;
		ResetFxUnit(fx);
		fx.position = pos;
		if(isOneShot_ && HasFlag(FollowOneShot)) {
			fx.followEmitter = true;
			fx.followOffset  = fx.position - position_; // エミッタ基準のオフセットを保存
		}
		units_.push_back(fx);
	}

	void FxEmitter::RestartOneShot() {
		units_.clear();
		emitTimer_   = 0.0f;
		elapsedTime_ = 0.0f;
		uvElapsedTime_ = 0.0f;
		SetFlag(FirstFrame,true);
		hasEmitted_         = false;
		isFinishedNotified_ = false;
		particleSequence_ = 0;
		randomStream_.Reset(randomSeed_);
		material_.noiseMaskUv.x = 0.0f;
		material_.noiseMaskUv.y = 0.0f;
		uvElapsedTime_ = 0.0f;
		SetFlag(Playing,true);
	}

	void FxEmitter::ResetFxUnit(FxUnit& fx) {
		// Random値は生成時に確定し、Update中は同じSeedを使用する。
		fx.randomSeed = HashParticleSeed(randomSeed_ + particleSequence_++);
		fx.position     = position_;
		fx.scale        = scale_.Get(randomStream_);
		fx.velocity     = velocity_.Get(randomStream_);
		fx.lifetime     = lifetime_.Get(randomStream_);
		fx.age          = 0.0f;
		fx.initialScale = fx.scale;
		fx.color        = CalyxEngine::Vector4(1,1,1,1);
		fx.vertexColor  = vertexColor_;
		fx.emissiveColor = CalyxEngine::Vector4(1,1,1,1);
		fx.emissiveIntensity = 0.0f;
		fx.alive        = true;
		fx.uvTransform.Initialize();
		fx.spinSpeed = spin_.Get(randomStream_);
		fx.alignDirection = CalyxEngine::Vector3(0.0f,0.0f,0.0f);
		fx.alignToDirection = false;
		fx.rotationEuler = initialRotation_;
		if(HasFlag(RandomSpinEmit)) { fx.rotationEuler.z = randomStream_.NextFloat(-CalyxEngine::kPi,CalyxEngine::kPi); }
		if(useDirection_) ApplyDirectionVelocity(fx);
		for(auto* module : moduleContainer_->GetInitializeModules()) {
			if(module->IsEnabled()) module->OnEmit(fx);
		}
	}

	void FxEmitter::ApplyDirectionVelocity(FxUnit& fx) {
		CalyxEngine::Vector3 dir = direction_.Get(randomStream_);
		if(dir.Length() <= 0.0001f) {
			fx.velocity = CalyxEngine::Vector3(0.0f,0.0f,0.0f);
			return;
		}

		dir = dir.Normalize();
		fx.velocity = dir * directionSpeed_.Get(randomStream_);
		fx.alignDirection = dir;
		fx.alignToDirection = rotateToDirection_;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	// ShowGui
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxEmitter::ShowGui() {
		ImGui::PushID(this);

		// ---- 上部ミニバー：よく触る項目をサッと ----
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Quick Controls");
		ImGui::SameLine();
		ImGui::NewLine();
		if(ImGui::CollapsingHeader("Ribbon Trail")) {
			auto& trail = trailEmitter_.Settings();
			ImGui::Checkbox("Enabled##trail",&trail.enabled);
			ImGui::BeginDisabled(!trail.enabled);
			ImGui::DragFloat("Lifetime##trail",&trail.lifetime,0.01f,0.01f,30.0f);
			ImGui::DragFloat("Base Width##trail",&trail.baseWidth,0.01f,0.0f,100.0f);
			ImGui::DragFloat("Min Sample Distance##trail",&trail.minSampleDistance,0.005f,0.001f,10.0f);
			ImGui::DragFloat("Max Sample Interval##trail",&trail.maxSampleInterval,0.005f,0.001f,1.0f);
			int maxPoints = static_cast<int>(trail.maxPointCount);
			if(ImGui::DragInt("Max Point Count##trail",&maxPoints,1,2,4096)) trail.maxPointCount=static_cast<uint32_t>(std::clamp(maxPoints,2,4096));
			int geometryMode=static_cast<int>(trail.geometryMode);
			if(ImGui::Combo("Geometry Mode##trail",&geometryMode,"Ribbon\0Mesh Extrusion\0Mesh Instances\0")) trail.geometryMode=static_cast<TrailGeometryMode>(geometryMode);
			if(trail.geometryMode!=TrailGeometryMode::Ribbon) {
				ImGui::TextUnformatted("Geometry Model (Drop Model Asset)");
				ImGui::TextDisabled("%s",trail.geometryModelPath.empty()?"Not assigned":trail.geometryModelPath.c_str());
				ImGui::InvisibleButton("##trail_geometry_model",ImVec2(ImGui::GetContentRegionAvail().x,32.0f));
				if(ImGui::BeginDragDropTarget()) {
					if(const ImGuiPayload* p=ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
						const auto payload=*reinterpret_cast<const AssetDragPayload*>(p->Data);
						if(payload.type==AssetType::Model) { trail.geometryModelGuid=payload.guid; if(auto* rec=AssetDatabase::GetInstance()->Get(payload.guid)) trail.geometryModelPath=rec->sourcePath.filename().string(); }
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::DragFloat3("Geometry Scale##trail",&trail.geometryScale.x,0.01f,0.001f,100.0f);
				if(trail.geometryMode==TrailGeometryMode::MeshExtrusion) ImGui::Checkbox("Close Cross Section##trail",&trail.closeCrossSection);
				if(trail.geometryMode==TrailGeometryMode::MeshInstances) ImGui::Checkbox("Align To Tangent##trail",&trail.alignInstancesToTangent);
			}
			int facingMode=static_cast<int>(trail.facingMode);
			if(ImGui::Combo("Facing Mode##trail",&facingMode,"Camera Facing\0Local Axis\0World Axis\0Cross\0")) trail.facingMode=static_cast<TrailFacingMode>(facingMode);
			if(trail.facingMode==TrailFacingMode::LocalAxis) ImGui::DragFloat3("Local Axis##trail",&trail.localAxis.x,0.01f,-1.0f,1.0f);
			if(trail.facingMode==TrailFacingMode::WorldAxis) ImGui::DragFloat3("World Axis##trail",&trail.worldAxis.x,0.01f,-1.0f,1.0f);
			int uvMode=static_cast<int>(trail.uvMode);
			if(ImGui::Combo("UV Mode##trail",&uvMode,"Distance\0Stretch\0")) trail.uvMode=static_cast<TrailUVMode>(uvMode);
			ImGui::DragFloat("UV Tiling##trail",&trail.uvTiling,0.01f);
			ImGui::Checkbox("Use Spline##trail",&trail.useSpline);
			int subdivisions=static_cast<int>(trail.splineSubdivision);
			if(ImGui::DragInt("Spline Subdivision##trail",&subdivisions,1,1,8)) trail.splineSubdivision=static_cast<uint32_t>(std::clamp(subdivisions,1,8));
			CalyxEngine::EnumConverter<BlendMode>::Combo("Trail Blend Mode",trail.blendMode);

			auto textureDrop = [&](const char* label,const char* id,Guid& guid,std::string& path) {
				ImGui::TextUnformatted(label);
				ImGui::InvisibleButton(id,ImVec2(ImGui::GetContentRegionAvail().x,32.0f));
				if(ImGui::BeginDragDropTarget()) {
					if(const ImGuiPayload* p=ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
						const auto payload=*reinterpret_cast<const AssetDragPayload*>(p->Data);
						if(payload.type==AssetType::Texture) { guid=payload.guid; if(auto* rec=AssetDatabase::GetInstance()->Get(guid)) path=rec->sourcePath.generic_string(); }
					}
					ImGui::EndDragDropTarget();
				}
			};
			textureDrop("Base Texture (Drop Asset)","##trail_base_texture",trail.baseTextureGuid,trail.baseTexturePath);
			textureDrop("Noise Texture (Drop Asset)","##trail_noise_texture",trail.noiseTextureGuid,trail.noiseTexturePath);
			ImGui::DragFloat2("Base Tiling##trail",&trail.material.baseTiling.x,0.01f);
			ImGui::DragFloat2("Base Scroll##trail",&trail.material.baseScrollSpeed.x,0.01f);
			ImGui::DragFloat2("Noise Tiling##trail",&trail.material.noiseTiling.x,0.01f);
			ImGui::DragFloat2("Noise Scroll##trail",&trail.material.noiseScrollSpeed.x,0.01f);
			ImGui::ColorEdit4("Color##trail",&trail.material.color.x,ImGuiColorEditFlags_Float|ImGuiColorEditFlags_HDR);
			if(trail.colorOverLifetime.keys.size()<2) trail.colorOverLifetime.keys={{0,{1,1,1,1}},{1,{1,1,1,1}}};
			ImGui::ColorEdit4("Color Start##trail",&trail.colorOverLifetime.keys.front().color.x);
			ImGui::ColorEdit4("Color End##trail",&trail.colorOverLifetime.keys.back().color.x);
			auto drawLifetimeCurve=[&](const char* label,FloatCurve& curve,float minValue,float maxValue) {
				ImGui::PushID(label);
				int mode=static_cast<int>(curve.mode);
				ImGui::Combo("Mode",&mode,"Constant\0Linear\0Curve\0Random Constants\0Random Curves\0");
				curve.mode=static_cast<CurveValueMode>(std::clamp(mode,0,4));
				if(curve.mode==CurveValueMode::Constant) ImGui::DragFloat(label,&curve.constant,0.01f,minValue,maxValue);
				else if(curve.mode==CurveValueMode::Linear || curve.mode==CurveValueMode::Curve) {
					if(curve.keys.size()<2) curve.keys={{0,curve.constant},{1,curve.constant}};
					ImGui::DragFloat("Start",&curve.keys.front().value,0.01f,minValue,maxValue);
					ImGui::DragFloat("End",&curve.keys.back().value,0.01f,minValue,maxValue);
				} else {
					ImGui::DragFloat("Min",&curve.constantMin,0.01f,minValue,maxValue);
					ImGui::DragFloat("Max",&curve.constantMax,0.01f,minValue,maxValue);
				}
				ImGui::PopID();
			};
			drawLifetimeCurve("Width Over Lifetime",trail.widthOverLifetime,0.0f,10.0f);
			drawLifetimeCurve("Alpha Over Lifetime",trail.alphaOverLifetime,0.0f,1.0f);
			drawLifetimeCurve("Emissive Over Lifetime",trail.emissiveOverLifetime,0.0f,10.0f);
			ImGui::DragFloat("Noise Strength##trail",&trail.material.noiseStrength,0.01f,0.0f,1.0f);
			ImGui::DragFloat("Distortion Strength##trail",&trail.material.distortionStrength,0.001f,0.0f,1.0f);
			ImGui::Checkbox("Dissolve##trail",&trail.material.dissolveEnabled);
			ImGui::DragFloat("Dissolve Start##trail",&trail.material.dissolveStart,0.01f,0.0f,1.0f);
			ImGui::DragFloat("Dissolve End##trail",&trail.material.dissolveEnd,0.01f,0.0f,1.0f);
			ImGui::DragFloat("Dissolve Softness##trail",&trail.material.dissolveSoftness,0.01f,0.001f,1.0f);
			ImGui::DragFloat("Dissolve Edge Width##trail",&trail.material.dissolveEdgeWidth,0.01f,0.0f,1.0f);
			ImGui::ColorEdit3("Dissolve Edge Color##trail",&trail.material.dissolveEdgeColor.x,ImGuiColorEditFlags_Float|ImGuiColorEditFlags_HDR);
			ImGui::DragFloat("Dissolve Edge Emissive##trail",&trail.material.dissolveEdgeEmissive,0.01f,0.0f,100.0f);
			ImGui::DragFloat("Head Fade##trail",&trail.material.headFade,0.01f,0.0f,1.0f);
			ImGui::DragFloat("Tail Fade##trail",&trail.material.tailFade,0.01f,0.0f,1.0f);
			ImGui::ColorEdit3("Emissive Color##trail",&trail.material.emissiveColor.x,ImGuiColorEditFlags_Float|ImGuiColorEditFlags_HDR);
			ImGui::DragFloat("Emissive Intensity##trail",&trail.material.emissiveIntensity,0.01f,0.0f,100.0f);
			ImGui::DragFloat("Alpha Clip##trail",&trail.material.alphaClipThreshold,0.001f,0.0f,1.0f);
			ImGui::EndDisabled();
		}
		ImGui::NewLine();

		ImGui::Spacing();
		ImGui::SameLine();

		ImGui::TextUnformatted("Rate");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120);
		GuiCmd::DragFloat("##rate_top",emitRate_,0.01f,0.0f,10.0f);
		ImGui::SameLine();

		ImGui::TextUnformatted("OneShot");
		ImGui::SameLine();
		GuiCmd::CheckBox("##oneshot_top",isOneShot_);

		// ================= Material =================
		if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Material)) {
			if(FxGui::GridScope sec{"Material"}; sec.open) {
				// Color
				FxGui::RowLabel("Color");
				ImGui::ColorEdit4("##color",&material_.color.x);

				FxGui::RowLabel("Vertex Color");
				ImGui::ColorEdit4("##vertex_color",&vertexColor_.x,ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				FxGui::RowLabel("UV Offset");
				ImGui::DragFloat2("##uv_offset",&uvSettings_.offset.x,0.01f);
				FxGui::RowLabel("UV Tiling");
				ImGui::DragFloat2("##uv_tiling",&uvSettings_.tiling.x,0.01f,0.001f,100.0f);
				FxGui::RowLabel("UV Scroll Speed");
				ImGui::DragFloat2("##uv_scroll",&uvSettings_.scrollSpeed.x,0.01f);
				FxGui::RowLabel("UV Rotation");
				float uvRotationDegrees = uvSettings_.rotation * 180.0f / CalyxEngine::kPi;
				if(ImGui::DragFloat("##uv_rotation",&uvRotationDegrees,0.25f,-360.0f,360.0f,"%.1f deg"))
					uvSettings_.rotation = CalyxEngine::ToRadians(uvRotationDegrees);

				// Texture (path表示 + 選択ボタン)
				FxGui::RowLabel("Texture");
				ImGui::BeginGroup();
				// 現在のパスを表示
				// ---- ドラッグ&ドロップでテクスチャ適用 ----
				ImGui::Text("Texture (Drag & Drop from Assets)");
				// ドロップ領域（InvisibleButton で有効アイテム化）
				ImVec2 dropSize(ImGui::GetContentRegionAvail().x,56.0f);
				ImGui::InvisibleButton("##TextureDrop",dropSize);

				// 見た目（枠とテキスト）
				const bool   hovered = ImGui::IsItemHovered();
				const ImVec2 rmin    = ImGui::GetItemRectMin();
				const ImVec2 rmax    = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddRect(
					rmin,rmax,hovered ? IM_COL32(120,180,255,220) : IM_COL32(90,90,90,160),
					8.0f,0,2.0f);
				ImGui::GetWindowDrawList()->AddText(
					ImVec2(rmin.x + 8.0f,rmin.y + 8.0f),
					IM_COL32(230,230,230,255),
					"Drop a Texture here");

				// 受け取り
				if(ImGui::BeginDragDropTarget()) {
					if(const ImGuiPayload* p = ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
						const AssetDragPayload payload =
							*reinterpret_cast<const AssetDragPayload*>(p->Data);
						if(payload.type == AssetType::Texture) { if(LoadTextureByGuid(payload.guid)) { textureGuid_ = payload.guid; } else { ImGui::OpenPopup("TextureDropError"); } }
					}
					ImGui::EndDragDropTarget();
				}

				// 失敗メッセージ（2D 以外の SRV 等）
				if(ImGui::BeginPopup("TextureDropError")) {
					ImGui::TextUnformatted("このテクスチャは適用できません（2D以外/未対応形式）。");
					ImGui::EndPopup();
				}

				// 現在のテクスチャ表示（GUID→ファイル名）
				/* auto labelFromGuid = [](const Guid& g) -> std::string {
					if(!g.isValid()) return "(none)";
					auto* db = AssetDatabase::GetInstance();
					for(auto* r : db->GetView()) {
						if(r && r->type == AssetType::Texture && r->guid == g) {
							return r->sourcePath.filename().string();
						}
					}
					return "(missing)";
				}; */

				ImGui::EndGroup(); // Texture BeginGroup の対応

				FxGui::RowLabel("Noise Mask Texture");
				ImGui::BeginGroup();
				ImGui::TextUnformatted(material_.noiseMaskParams.x > 0.5f ? "Noise mask attached" : "Drop a noise texture here");
				ImGui::InvisibleButton("##NoiseMaskTextureDrop", ImVec2(ImGui::GetContentRegionAvail().x,40.0f));
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
					noiseMaskTextureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture("Textures/white1x1.dds");
					material_.noiseMaskParams.x = 0.0f;
				}
				ImGui::BeginDisabled(material_.noiseMaskParams.x <= 0.5f);
				ImGui::DragFloat("Tiling##noise_mask", &material_.noiseMaskParams.y, 0.05f, 0.01f, 32.0f);
				ImGui::SliderFloat("Strength##noise_mask", &material_.noiseMaskParams.z, 0.0f, 1.0f);
				ImGui::SliderFloat("Threshold##noise_mask", &material_.noiseMaskParams.w, 0.0f, 1.0f);
				ImGui::SliderFloat("Softness##noise_mask", &material_.noiseMaskUv.z, 0.0001f, 1.0f);
				ImGui::DragFloat2("Scroll Speed##noise_mask", &noiseMaskScrollSpeed_.x, 0.01f);
				ImGui::EndDisabled();
				if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
					ImGui::SetTooltip("アタッチしたNoise TextureのRチャンネルでAlphaをマスクします。\nThresholdで表示範囲、Softnessで境界、Scroll Speedで流れを調整します。");
				ImGui::EndGroup();

				// メッシュ
				FxGui::RowLabel("mesh");
				ImGui::BeginGroup();
				// 現在のパスを表示
				// ---- ドラッグ&ドロップでメッシュデータ適用 ----
				ImGui::Text("MeshData (Drag & Drop from Assets)");
				// ドロップ領域（InvisibleButton で有効アイテム化）
				ImGui::InvisibleButton("##MeshDataDrop",dropSize);

				// 見た目（枠とテキスト）
				const bool   m_hovered = ImGui::IsItemHovered();
				const ImVec2 m_rmin    = ImGui::GetItemRectMin();
				const ImVec2 m_rmax    = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddRect(
					m_rmin,m_rmax,m_hovered ? IM_COL32(120,180,255,220) : IM_COL32(90,90,90,160),
					8.0f,0,2.0f);

				std::string modelLabel = modelPath;
				if(modelGuid_.isValid()) { if(auto* rec = AssetDatabase::GetInstance()->Get(modelGuid_)) { modelLabel = rec->sourcePath.filename().string(); } }

				ImGui::GetWindowDrawList()->AddText(
					ImVec2(m_rmin.x + 8.0f,m_rmin.y + 8.0f),
					IM_COL32(230,230,230,255),
					("Model: " + modelLabel).c_str());

				// 受け取り
				if(ImGui::BeginDragDropTarget()) {
					if(const ImGuiPayload* p = ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
						const AssetDragPayload payload =
							*reinterpret_cast<const AssetDragPayload*>(p->Data);
						if(payload.type == AssetType::Model) { LoadModelByGuid(payload.guid); }
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::EndGroup();
			}
			GuiCmd::EndSection();
		}

		// // ================= Billboard =================
		if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Object)) {
			if(FxGui::GridScope sec{"Billboard"}; sec.open) {
				FxGui::RowLabel("Mode");
				static const char* modes[] = {"None","Full","AxisY"};
				int                current = static_cast<int>(billboardMode_);
				if(ImGui::Combo("##billmode",&current,modes, IM_ARRAYSIZE(modes))) {
					billboardMode_        = static_cast<BillboardMode>(current);
					billboardParams_.mode = current;
					billboardCB_.TransferData(billboardParams_);
				}
			}
			GuiCmd::EndSection();
		}

		// ================= ParameterData =================
		if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::ParameterData)) {
			// ================= Emission =================
			if(FxGui::GridScope sec{"Emission"}; sec.open) {

				FxGui::RowLabel("offset");
				GuiCmd::DragFloat3("##offset",offset_,0.01f,-100.0f,100.0f);
				
				// エミッタ形状を選べるようにする
				FxGui::RowLabel("emitter shape");
				CalyxEngine::EnumConverter<EmitterShape>::Combo("Emitter Shape",shape_);

				FxGui::RowLabel("shape radius");
				GuiCmd::DragFloat("##shapeRadius",shapeRadius_,0.01f,0.0f,100.0f);

				if(shape_ == EmitterShape::Box) {
					FxGui::RowLabel("shape size");
					GuiCmd::DragFloat3("##shapeSize",shapeSize_,0.01f,-100.0f,100.0f);
				}

				if(shape_ == EmitterShape::Cone) {
					FxGui::RowLabel("shape angle");
					GuiCmd::DragFloat("##shapeAngle",shapeAngle_,0.1f,0.0f,89.0f);
				}

				// ブレンドモードを選べるようにする
				FxGui::RowLabel("blend mode");
				CalyxEngine::EnumConverter<BlendMode>::Combo("BlendMode",blendMode_);

				FxGui::RowLabel("Alive Count");
				ImGui::Text("%zu",units_.size());

				FxGui::RowLabel("World Position");
				GuiCmd::DragFloat3("##pos",position_);

				FxGui::RowLabel("Emit Rate (sec)");
				GuiCmd::DragFloat("##rate",emitRate_,0.01f,0.0f,10.0f);

				FxGui::RowLabel("Complement Trail");
				{
					bool comp = HasFlag(Complement);
					if(GuiCmd::CheckBox("##comp",comp)) SetFlag(Complement,comp);
				}

				FxGui::RowLabel("random Spin on Emit");
				{
					bool rse = HasFlag(RandomSpinEmit);
					if(GuiCmd::CheckBox("##randspin",rse)) SetFlag(RandomSpinEmit,rse);
				}

				FxGui::RowLabel("Fixed Random Seed");
				GuiCmd::CheckBox("##fixedRandomSeed",fixedRandomSeed_);
				if(fixedRandomSeed_) {
					FxGui::RowLabel("Random Seed");
					int seed = static_cast<int>(randomSeed_);
					if(ImGui::DragInt("##randomSeed",&seed,1,0)) randomSeed_ = static_cast<uint32_t>((std::max)(seed,0));
				}
			}

			// ================= Camera Dither =================
			if(FxGui::GridScope sec{"Camera Dither"}; sec.open) {
				FxGui::RowLabel("Enable");
				bool enabled = IsCameraDitherEnabled();
				if(GuiCmd::CheckBox("##cameraDitherEnable",enabled)) {
					SetCameraFadeEnabled(enabled);
				}

				ImGui::BeginDisabled(!enabled);
				FxGui::RowLabel("Near");
				if(GuiCmd::DragFloat("##cameraDitherNear",fadeParams_.fadeNear,0.01f,0.0f,1000.0f)) {
					if(fadeParams_.fadeFar < fadeParams_.fadeNear) fadeParams_.fadeFar = fadeParams_.fadeNear;
					fadeCB_.TransferData(fadeParams_);
				}

				FxGui::RowLabel("Far");
				if(GuiCmd::DragFloat("##cameraDitherFar",fadeParams_.fadeFar,0.01f,0.0f,1000.0f)) {
					if(fadeParams_.fadeFar < fadeParams_.fadeNear) fadeParams_.fadeNear = fadeParams_.fadeFar;
					fadeCB_.TransferData(fadeParams_);
				}
				ImGui::EndDisabled();
			}

			// ================= Params =================
			if(FxGui::GridScope sec{"Params"}; sec.open) {
				FxGui::DrawParam("Scale",scale_);
				ImGui::BeginDisabled(useDirection_);
				FxGui::DrawParam("Velocity",velocity_);
				ImGui::EndDisabled();
				FxGui::RowLabel("Direction Enable");
				GuiCmd::CheckBox("##directionEnable",useDirection_);

				ImGui::BeginDisabled(!useDirection_);
				FxGui::DrawParam("Direction",direction_);
				FxGui::DrawParam("Direction Speed",directionSpeed_);

				FxGui::RowLabel("Rotate To Direction");
				GuiCmd::CheckBox("##rotateToDirection",rotateToDirection_);
				ImGui::EndDisabled();

				FxGui::DrawParam("Lifetime",lifetime_);
				FxGui::RowLabel("Initial Rotation");
				Vector3 initialRotationDegrees = initialRotation_ * (180.0f / CalyxEngine::kPi);
				if(GuiCmd::DragFloat3("##initialRotation",initialRotationDegrees,0.25f,-360.0f,360.0f))
					initialRotation_ = initialRotationDegrees * (CalyxEngine::kPi / 180.0f);
				FxGui::DrawParam("Spin",spin_);
			}

			// ================= Playback =================
			if(FxGui::GridScope sec{"Playback"}; sec.open) {
				FxGui::RowLabel("Controls");
				ImGui::BeginGroup();
				if(ImGui::Button("Play")) { Play(); }
				ImGui::SameLine();
				if(ImGui::Button("Stop")) { Stop(); }
				ImGui::SameLine();
				if(ImGui::Button("Reset")) { Reset(); }
				ImGui::EndGroup();

				FxGui::RowLabel("Draw Enable");
			}

			// ================= One-Shot =================
			if(FxGui::GridScope sec{"One-Shot"}; sec.open) {
				FxGui::RowLabel("Enable");
				if(GuiCmd::CheckBox("##oneshot",isOneShot_)) {
					if(!isOneShot_) { hasEmitted_ = false; } // OFFに戻した時の自然な継続
				}

				if(isOneShot_) {
					FxGui::RowLabel("Follow Emitter");
					{
						bool fe = HasFlag(FollowOneShot);
						if(GuiCmd::CheckBox("##followoneshot",fe)) SetFlag(FollowOneShot,fe);
					}

					bool tp = GetTimedPreview();
					if(GuiCmd::CheckBox("##timedPrev",tp)) {
						SetTimedPreview(tp);
						// ON にした瞬間に一度流したい場合は以下を有効に
						// if (tp && isOneShot_) RestartOneShot();
					}

					FxGui::RowLabel("Interval (sec)");
					float iv = GetPreviewInterval();
					if(GuiCmd::DragFloat("##prevInt",iv,0.01f,0.05f,10.0f)) { SetPreviewInterval(iv); }

					ImGui::BeginDisabled(!isOneShot_);
					FxGui::RowLabel("Emit Count");
					ImGui::DragInt("##count",&emitCount_,1,1,kMaxUnits_);

					FxGui::RowLabel("Auto Destroy");
					GuiCmd::CheckBox("##autoDestroy",autoDestroy_);

					FxGui::RowLabel("Delay (sec)");
					GuiCmd::DragFloat("##delay",emitDelay_,0.01f,0.0f,10.0f);
					ImGui::EndDisabled();

					ImGui::BeginDisabled(isOneShot_);
					FxGui::RowLabel("Emit Duration (sec)");
					GuiCmd::DragFloat("##duration",emitDuration_,0.01f,-1.0f,60.0f);
					ImGui::EndDisabled();
				}
			}

			// ================= Modules =================
			if(moduleContainer_) {
				if(FxGui::GridScope sec{"Modules"}; sec.open) {
					// ラベル列
					FxGui::RowLabel("Modules");

					ImGui::BeginGroup();
					// 幅を常にその列いっぱいに
					FxGui::FullWidthScope _fullWidth{};
					// 少しだけ余裕を持たせる見た目
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(6,4));
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(6,6));

					// --- 有効モジュール（パラメータをここで全部縦に描く） ---
					moduleContainer_->ShowModulesGui();

					// --- 追加パレット（同じ列のまま下に表示） ---
					moduleContainer_->ShowAvailableModulesGui();

					ImGui::PopStyleVar(2);
					ImGui::EndGroup();
				}
			}
			GuiCmd::EndSection();
		}

		ImGui::Spacing();
		ImGui::PopID();
	}

	void FxEmitter::DrawEmitterShape(const WorldTransform& tf) {
		[[maybe_unused]] CalyxEngine::Vector3 pos = tf.GetWorldPosition();
		DrawEmitterShapeInternal(false);
	}

	void FxEmitter::DrawEmitterShapePreview(const WorldTransform& tf) {
		[[maybe_unused]] CalyxEngine::Vector3 pos = tf.GetWorldPosition();
		DrawEmitterShapeInternal(true);
	}

	void FxEmitter::DrawEmitterShapeInternal(bool effectPreview) {
		const CalyxEngine::Vector3 absScale{
			(std::max)(std::abs(worldScale_.x), 0.0001f),
			(std::max)(std::abs(worldScale_.y), 0.0001f),
			(std::max)(std::abs(worldScale_.z), 0.0001f)};

		CalyxEngine::Vector4 color = CalyxEngine::Vector4(1.0f,0.0f,0.0f,1.0f);
		switch(shape_) {
		case EmitterShape::Circle: {
			const float radiusX = shapeRadius_ * absScale.x;
			const float radiusZ = shapeRadius_ * absScale.z;
			if(effectPreview) {
				PrimitiveDrawer::GetInstance()->DrawEffectPreviewCircle(position_,worldRotation_,radiusX,radiusZ,color);
			} else {
				PrimitiveDrawer::GetInstance()->DrawCircle(position_,worldRotation_,radiusX,radiusZ,color);
			}
		}
		break;

		case EmitterShape::Cone: {
			const float height = (std::max)(shapeRadius_,0.0f) * absScale.y;
			const float angleRad = std::clamp(CalyxEngine::ToRadians(shapeAngle_),0.0f,CalyxEngine::kPi * 0.5f);
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
				PrimitiveDrawer::GetInstance()->DrawEffectPreviewSphere(position_,shapeRadius_ * maxScale,4,color);
			} else {
				PrimitiveDrawer::GetInstance()->DrawSphere(position_,shapeRadius_ * maxScale,4,color);
			}
		}
		break;

		case EmitterShape::Box: {
			const CalyxEngine::Vector3 scaledSize{
				shapeSize_.x * absScale.x,
				shapeSize_.y * absScale.y,
				shapeSize_.z * absScale.z};
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

	/////////////////////////////////////////////////////////////////////////////////////////
	// SetCommand
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxEmitter::SetCommand(ID3D12GraphicsCommandList* cmdList) {

		cmdList->SetGraphicsRootConstantBufferView(1,materialBuffer_.GetResource()->GetGPUVirtualAddress()); // [1] gMaterial (b1)
		cmdList->SetGraphicsRootDescriptorTable(3,GetTextureHandle());                                       // [3] gTexture  (t1)
		cmdList->SetGraphicsRootConstantBufferView(4,billboardCB_.GetResource()->GetGPUVirtualAddress());    // [4] gBillboard (b2)
		cmdList->SetGraphicsRootConstantBufferView(5,fadeCB_.GetResource()->GetGPUVirtualAddress());         // [5] gFade      (b3)
		cmdList->SetGraphicsRootDescriptorTable(6,noiseMaskTextureHandle_);                                  // [6] gNoiseMaskTexture (t2)
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	// Config apply/extract
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxEmitter::ApplyConfigFrom(const EmitterConfig& config) {
		position_       = config.position;
		offset_         = config.offset;
		worldRotation_  = config.rotation;
		worldScale_     = config.worldScale;
		material_.color = config.color;
		trailEmitter_.ApplySettings(config.trail);
		vertexColor_ = config.vertexColor;
		uvSettings_ = config.uvSettings;
		uvElapsedTime_ = 0.0f;
		material_.uvOffsetTiling = {uvSettings_.offset.x,uvSettings_.offset.y,uvSettings_.tiling.x,uvSettings_.tiling.y};
		material_.uvScrollRotationTime = {uvSettings_.scrollSpeed.x,uvSettings_.scrollSpeed.y,uvSettings_.rotation,0.0f};
		noiseMaskTextureGuid_ = config.noiseMaskTextureGuid;
		noiseMaskTexturePath_ = config.noiseMaskTexturePath;
		noiseMaskTextureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture("Textures/white1x1.dds");
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
		velocity_.FromConfig(config.velocity);
		direction_.FromConfig(config.direction.vector);
		directionSpeed_.FromConfig(config.direction.speed);
		useDirection_       = config.direction.enabled;
		rotateToDirection_  = config.direction.rotateToDirection;
		spin_.FromConfig(config.spin);
		initialRotation_ = config.initialRotation;
		lifetime_.FromConfig(config.lifetime);
		scale_.FromConfig(config.scale);
		emitRate_  = config.emitRate;
		modelPath  = config.modelPath;
		modelGuid_ = config.modelGuid;
		if(modelGuid_.isValid()) { LoadModelByGuid(modelGuid_); }

		material_.texturePath = config.texturePath;
		textureGuid_          = config.textureGuid;
		textureHandle_        = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(textureGuid_);
		SetFlag(DrawEnable,config.isDrawEnable);
		SetFlag(Complement,config.isComplement);
		moduleContainer_ = std::make_unique<CalyxEngine::FxModuleContainer>(config.modules);
		isOneShot_       = config.isOneShot;
		autoDestroy_     = config.autoDestroy;
		emitCount_       = config.emitCount;
		emitDelay_       = config.emitDelay;
		emitDuration_    = config.emitDuration;
		billboardMode_   = config.billboardMode;
		SetFlag(RandomSpinEmit,config.randomSpinEmit);
		fixedRandomSeed_ = config.fixedRandomSeed;
		randomSeed_ = fixedRandomSeed_ ? config.randomSeed : HashParticleSeed(config.randomSeed ^ static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this)));
		particleSequence_ = 0;
		randomStream_.Reset(randomSeed_);
		SetFlag(FollowOneShot,config.followOneShot);
		fadeParams_.enabled = config.cameraDitherEnabled ? 1u : 0u;
		fadeParams_.fadeNear = config.cameraDitherNear;
		fadeParams_.fadeFar = config.cameraDitherFar;
		fadeCB_.TransferData(fadeParams_);
		blendMode_ = config.blendMode;
		shape_ = config.emitterShape;
		shapeSize_ = config.shapeSize;
		shapeRadius_ = config.shapeRadius;
		shapeAngle_ = config.shapeAngle;

		SetFlag(FirstFrame,true);
		hasEmitted_  = false;
		elapsedTime_ = 0.0f;
		// Do not auto-play on load; wait for explicit Play/Restart.
		SetFlag(Playing,false);
	}

	void FxEmitter::ExtractConfigTo(EmitterConfig& config) const {
		config.position       = position_;
		config.offset         = offset_;
		config.rotation       = worldRotation_;
		config.worldScale     = worldScale_;
		config.color          = material_.color;
		config.trail          = trailEmitter_.Settings();
		config.vertexColor    = vertexColor_;
		config.uvSettings     = uvSettings_;
		config.velocity       = Vector3ParamConfig{velocity_.ToConfig()};
		config.direction.enabled = useDirection_;
		config.direction.vector = Vector3ParamConfig{direction_.ToConfig()};
		config.direction.speed = FxFloatParamConfig{directionSpeed_.ToConfig()};
		config.direction.rotateToDirection = rotateToDirection_;
		config.spin           = Vector3ParamConfig{spin_.ToConfig()};
		config.initialRotation = initialRotation_;
		config.lifetime       = FxFloatParamConfig{lifetime_.ToConfig()};
		config.scale          = Vector3ParamConfig{scale_.ToConfig()};
		config.emitRate       = emitRate_;
		config.modelPath      = modelPath;
		config.modelGuid      = modelGuid_;
		config.texturePath    = material_.texturePath;
		config.textureGuid    = textureGuid_;
		config.noiseMaskTextureGuid = noiseMaskTextureGuid_;
		config.noiseMaskTexturePath = noiseMaskTexturePath_;
		config.noiseMaskScale = material_.noiseMaskParams.y;
		config.noiseMaskStrength = material_.noiseMaskParams.z;
		config.noiseMaskThreshold = material_.noiseMaskParams.w;
		config.noiseMaskSoftness = material_.noiseMaskUv.z;
		config.noiseMaskScrollSpeed = noiseMaskScrollSpeed_;
		config.isDrawEnable   = HasFlag(DrawEnable);
		config.isComplement   = HasFlag(Complement);
		config.randomSpinEmit = HasFlag(RandomSpinEmit);
		config.fixedRandomSeed = fixedRandomSeed_;
		config.randomSeed = fixedRandomSeed_ ? randomSeed_ : 1u;
		config.followOneShot  = HasFlag(FollowOneShot);
		if(moduleContainer_)
			config.modules = moduleContainer_->ExtractConfigs();
		else
			config.modules.clear();
		config.isOneShot     = isOneShot_;
		config.autoDestroy   = autoDestroy_;
		config.emitCount     = emitCount_;
		config.emitDelay     = emitDelay_;
		config.emitDuration  = emitDuration_;
		config.billboardMode = billboardMode_;
		config.cameraDitherEnabled = IsCameraDitherEnabled();
		config.cameraDitherNear = fadeParams_.fadeNear;
		config.cameraDitherFar = fadeParams_.fadeFar;
		config.blendMode     = blendMode_;
		config.emitterShape  = shape_;
		config.shapeSize     = shapeSize_;
		config.shapeRadius   = shapeRadius_;
		config.shapeAngle    = shapeAngle_;
	}

	void FxEmitter::Play() {
		SetFlag(Playing,true);
		SetFlag(FirstFrame,true);

		if(isOneShot_) {
			// OneShot 時は状態も初期化しておく
			hasEmitted_   = false;
			elapsedTime_  = 0.0f;
			previewTimer_ = 0.0f;
			particleSequence_ = 0;
			randomStream_.Reset(randomSeed_);
		}
	}

	void FxEmitter::Stop() { SetFlag(Playing,false); }

	void FxEmitter::Reset() {
		trailEmitter_.Reset();
		units_.clear();
		emitTimer_   = 0.0f;
		elapsedTime_ = 0.0f;
		uvElapsedTime_ = 0.0f;
		SetFlag(FirstFrame,true);
		hasEmitted_   = false;
		previewTimer_ = 0.0f;
		particleSequence_ = 0;
		randomStream_.Reset(randomSeed_);
	}

	bool FxEmitter::LoadTextureByGuid(const Guid& g) {
		if(!g.isValid()) return false;

		auto h = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(g);
		if(!h.ptr) return false;

		textureHandle_ = h;
		textureGuid_   = g;
		return true;
	}

	void FxEmitter::SetTextureGuid(const Guid& g) {
		if(!g.isValid()) {
			textureGuid_ = Guid::Empty();
			textureHandle_ = AssetManager::GetInstance()->GetTextureManager()->LoadTexture(material_.texturePath.empty() ? "Textures/white1x1.dds" : material_.texturePath);
			return;
		}

		LoadTextureByGuid(g);
	}

	void FxEmitter::SetCameraFade(float nearZ,float farZ) {
		fadeParams_.fadeNear = nearZ;
		fadeParams_.fadeFar  = farZ;
		fadeParams_.enabled  = 1u;
		fadeCB_.TransferData(fadeParams_);
	}

	void FxEmitter::SetCameraFadeEnabled(bool enabled) {
		fadeParams_.enabled = enabled ? 1u : 0u;
		fadeCB_.TransferData(fadeParams_);
	}

	// ---- callback ----
	void FxEmitter::SetOnFinishedCallback(std::function<void()> callback) { onFinished_ = std::move(callback); }
} // namespace CalyxEngine
