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
#include "Engine/Assets/Texture/TextureManager.h"
#include "Engine/Foundation/Math/MathUtil.h"
#include "Engine/Foundation/Utility/Converter/EnumConverter.h"
#include "Engine/Graphics/Camera/Manager/CameraManager.h"

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

namespace CalyxEffect {
	/////////////////////////////////////////////////////////////////////////////////////////
	// ctor / dtor
	/////////////////////////////////////////////////////////////////////////////////////////
	FxEmitter::FxEmitter() {
		ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();

		// マテリアル
		material_.color = CalyxMath::Vector4(1,1,1,1);
		materialBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice());
		billboardCB_.Initialize(GraphicsGroup::GetInstance()->GetDevice());
		fadeCB_.Initialize(GraphicsGroup::GetInstance()->GetDevice());

		instanceBuffer_.Initialize(device,kMaxUnits_);
		instanceBuffer_.CreateSrv(device);

		// ビルボード定数バッファ初期化
		billboardParams_.mode = static_cast<uint32_t>(billboardMode_);
		billboardCB_.Initialize(device);
		billboardCB_.TransferData(billboardParams_);

		// 各種パラメータ
		velocity_ = FxParam<CalyxMath::Vector3>::MakeRandom(CalyxMath::Vector3(-1.0f,0.0f,-1.0f),
															CalyxMath::Vector3(1.0f,0.0f,1.0f));
		lifetime_ = FxParam<float>::MakeRandom(1.0f,3.0f);
		scale_    = FxParam<CalyxMath::Vector3>::MakeConstant();

		moduleContainer_ = std::make_unique<CalyxEffect::FxModuleContainer>();
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

		if(!HasFlag(Playing)) return;

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

			CalyxMath::Vector3 moveDelta = position_ - prevPostion_;
			float              distance  = moveDelta.Length();

			if(distance > 0.0f && HasFlag(Complement)) {
				// 密度を alphaMultiplier でスケーリング (0.01以下にならないようにガード)
				float spawnInterval = 0.02f / (std::max)(alphaMultiplier_,0.01f);
				int   trailCount    = static_cast<int>(distance / spawnInterval);
				for(int i = 0; i < trailCount; ++i) {
					float              dist     = static_cast<float>(i) * spawnInterval;
					float              t        = dist / distance;
					CalyxMath::Vector3 spawnPos = CalyxMath::Vector3::Lerp(prevPostion_,position_,t);
					Emit(spawnPos);
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

			for(auto& m : moduleContainer_->GetModules()) { if(m->IsEnabled()) m->OnUpdate(fx,deltaTime); }

			// 追従フラグが立っている場合はエミッタ位置+オフセットに常に合わせる（速度適用は行わない）
			if(fx.followEmitter) { fx.position = position_ + fx.followOffset; } else { fx.position += fx.velocity * deltaTime; }

			// スピン
			fx.rotationEuler.z += fx.spinSpeed * deltaTime;

			fx.age += deltaTime;
			if(fx.age >= fx.lifetime) fx.alive = false;

			CalyxMath::Matrix4x4 uvTransformMatrix =
				CalyxMath::MakeScaleMatrix(CalyxMath::Vector3(fx.uvTransform.scale.x,fx.uvTransform.scale.y,1.0f));
			uvTransformMatrix = CalyxMath::Matrix4x4::Multiply(uvTransformMatrix,CalyxMath::MakeRotateZMatrix(fx.uvTransform.rotate));
			uvTransformMatrix = CalyxMath::Matrix4x4::Multiply(uvTransformMatrix,
															   CalyxMath::MakeTranslateMatrix(CalyxMath::Vector3(fx.uvTransform.translate.x,fx.uvTransform.translate.y,0.0f)));
			material_.uvTransform = uvTransformMatrix;
		}

		ParticleMaterial renderMat = material_;
		renderMat.color.w *= alphaMultiplier_;
		materialBuffer_.TransferData(material_);
		billboardCB_.TransferData(billboardParams_);
		fadeCB_.TransferData(fadeParams_);
		std::erase_if(units_,[](const FxUnit& fx) { return !fx.alive; });

		bool shouldNotify =
			(isOneShot_ && hasEmitted_ && units_.empty()) ||
			(emitDuration_ >= 0.0f && elapsedTime_ > emitDelay_ + emitDuration_ && units_.empty());

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
			data.rotation = fx.rotationEuler.z;

			gpuUnits.push_back(data);
		}

		if(!gpuUnits.empty()) { instanceBuffer_.TransferVectorData(gpuUnits); }
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	// Emit / Reset
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxEmitter::Emit() {
		// 形状が点の時はエミッタの位置、そうでない時は形状に応じた位置を生成して発生させる
		Emit(GenerateSpawnPosition());
	}

	void FxEmitter::Emit(const CalyxMath::Vector3& pos) {
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
		SetFlag(FirstFrame,true);
		hasEmitted_         = false;
		isFinishedNotified_ = false;
		SetFlag(Playing,true);
	}

	void FxEmitter::ResetFxUnit(FxUnit& fx) {
		fx.position     = position_;
		fx.scale        = scale_.Get();
		fx.velocity     = velocity_.Get();
		fx.lifetime     = lifetime_.Get();
		fx.age          = 0.0f;
		fx.initialScale = fx.scale;
		fx.color        = CalyxMath::Vector4(1,1,1,1);
		fx.alive        = true;
		fx.uvTransform.Initialize();
		fx.spinSpeed = spin_.Get();
		if(HasFlag(RandomSpinEmit)) { fx.rotationEuler.z = Random::Generate<float>(-CalyxMath::kPi,CalyxMath::kPi); } else { fx.rotationEuler.z = 0.0f; }
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
		if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::Material)) {
			if(FxGui::GridScope sec{"Material"}; sec.open) {
				// Color
				FxGui::RowLabel("Color");
				ImGui::ColorEdit4("##color",&material_.color.x);

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
		if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::Object)) {
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
		if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::ParameterData)) {
			// ================= Emission =================
			if(FxGui::GridScope sec{"Emission"}; sec.open) {
				// エミッタ形状を選べるようにする
				FxGui::RowLabel("emitter shape");
				CalyxUtil::EnumConverter<EmitterShape>::Combo("Emitter Shape",shape_);

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
				CalyxUtil::EnumConverter<BlendMode>::Combo("BlendMode",blendMode_);

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
			}

			// ================= Params =================
			if(FxGui::GridScope sec{"Params"}; sec.open) {
				FxGui::DrawParam("Scale",scale_);
				FxGui::DrawParam("Velocity",velocity_);
				FxGui::DrawParam("Lifetime",lifetime_);
				FxGui::DrawParam("spin",spin_);
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
		[[maybe_unused]] CalyxMath::Vector3 pos = tf.GetWorldPosition();

		const CalyxMath::Vector3 absScale{
			(std::max)(std::abs(worldScale_.x), 0.0001f),
			(std::max)(std::abs(worldScale_.y), 0.0001f),
			(std::max)(std::abs(worldScale_.z), 0.0001f)};

		CalyxMath::Vector4 color = CalyxMath::Vector4(1.0f,0.0f,0.0f,1.0f);
		switch(shape_) {
		case EmitterShape::Sphere: {
			const float maxScale = (std::max)((std::max)(absScale.x,absScale.y),absScale.z);
			PrimitiveDrawer::GetInstance()->DrawSphere(position_,shapeRadius_ * maxScale,4,color);
		}
		break;

		case EmitterShape::Box: {
			const CalyxMath::Vector3 scaledSize{
				shapeSize_.x * absScale.x,
				shapeSize_.y * absScale.y,
				shapeSize_.z * absScale.z};
			PrimitiveDrawer::GetInstance()->DrawBox(position_,worldRotation_,scaledSize,color);
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
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	// Config apply/extract
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxEmitter::ApplyConfigFrom(const EmitterConfig& config) {
		position_       = config.position;
		material_.color = config.color;
		velocity_.FromConfig(config.velocity);
		lifetime_.FromConfig(config.lifetime);
		scale_.FromConfig(config.scale);
		emitRate_  = config.emitRate;
		modelPath  = config.modelPath;
		modelGuid_ = config.modelGuid;
		if(modelGuid_.isValid()) { LoadModelByGuid(modelGuid_); }

		material_.texturePath = config.texturePath;
		textureGuid_          = config.textureGuid;
		textureHandle_        = TextureManager::GetInstance()->LoadTexture(textureGuid_);
		SetFlag(DrawEnable,config.isDrawEnable);
		SetFlag(Complement,config.isComplement);
		moduleContainer_ = std::make_unique<CalyxEffect::FxModuleContainer>(config.modules);
		isOneShot_       = config.isOneShot;
		autoDestroy_     = config.autoDestroy;
		emitCount_       = config.emitCount;
		emitDelay_       = config.emitDelay;
		emitDuration_    = config.emitDuration;
		billboardMode_   = config.billboardMode;
		SetFlag(RandomSpinEmit,config.randomSpinEmit);
		SetFlag(FollowOneShot,config.followOneShot);
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
		config.color          = material_.color;
		config.velocity       = Vector3ParamConfig{velocity_.ToConfig()};
		config.lifetime       = FxFloatParamConfig{lifetime_.ToConfig()};
		config.scale          = Vector3ParamConfig{scale_.ToConfig()};
		config.emitRate       = emitRate_;
		config.modelPath      = modelPath;
		config.modelGuid      = modelGuid_;
		config.texturePath    = material_.texturePath;
		config.textureGuid    = textureGuid_;
		config.isDrawEnable   = HasFlag(DrawEnable);
		config.isComplement   = HasFlag(Complement);
		config.randomSpinEmit = HasFlag(RandomSpinEmit);
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
		}
	}

	void FxEmitter::Stop() { SetFlag(Playing,false); }

	void FxEmitter::Reset() {
		units_.clear();
		emitTimer_   = 0.0f;
		elapsedTime_ = 0.0f;
		SetFlag(FirstFrame,true);
		hasEmitted_   = false;
		previewTimer_ = 0.0f;
	}

	bool FxEmitter::LoadTextureByGuid(const Guid& g) {
		if(!g.isValid()) return false;

		auto h = TextureManager::GetInstance()->LoadTexture(g);
		if(!h.ptr) return false;

		textureHandle_ = h;
		textureGuid_   = g;
		return true;
	}

	void FxEmitter::SetCameraFade(float nearZ,float farZ) {
		fadeParams_.fadeNear = nearZ;
		fadeParams_.fadeFar  = farZ;
	}

	// ---- callback ----
	void FxEmitter::SetOnFinishedCallback(std::function<void()> callback) { onFinished_ = std::move(callback); }
} // namespace CalyxEffect