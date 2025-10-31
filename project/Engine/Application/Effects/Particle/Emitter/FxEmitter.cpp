#include "FxEmitter.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/FxGuiHelpers.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Module/Factory/ModuleFactory.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// externals
#include <externals/imgui/ImGuiFileDialog.h>
#include <externals/imgui/imgui.h>

#include "Engine/Foundation/Utility/Func/CxUtils.h"

namespace {
void VSeparator(float height = 0.0f, float thickness = 1.0f, float pad = 6.0f) {
	ImVec2 pos = ImGui::GetCursorScreenPos();
	if(height <= 0.0f) height = ImGui::GetTextLineHeightWithSpacing();

	// 線の色は ImGuiCol_Separator を流用
	ImU32		col = ImGui::GetColorU32(ImGuiCol_Separator);
	ImDrawList* dl	= ImGui::GetWindowDrawList();
	float		x	= pos.x + pad * 0.5f; // ちょい内側に
	dl->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + height), col, thickness);

	// レイアウトを前へ送る（幅 = pad + thickness）
	ImGui::Dummy(ImVec2(pad + thickness, height));
	ImGui::SameLine();
}
}; // namespace

FxEmitter::FxEmitter() {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	// マテリアルの初期化
	material_.color = Vector4(1, 1, 1, 1);
	materialBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice());

	instanceBuffer_.Initialize(device, kMaxUnits_);
	instanceBuffer_.CreateSrv(device);

	velocity_ = FxParam<Vector3>::MakeRandom(
		Vector3(-1.0f, 0.0f, -1.0f),
		Vector3(1.0f, 0.0f, 1.0f));

	lifetime_ = FxParam<float>::MakeRandom(1.0f, 3.0f);
	scale_	  = FxParam<Vector3>::MakeConstant();

	// モジュールの初期化
	moduleContainer_ = std::make_unique<FxModuleContainer>();
}

FxEmitter::~FxEmitter() {
	instanceBuffer_.ReleaseSrv();
}

/////////////////////////////////////////////////////////////////////////////////////////
//			更新
/////////////////////////////////////////////////////////////////////////////////////////
void FxEmitter::Update(float deltaTime) {
	if(!isPlaying_) return;

	// ワールド座標に追従
	position_ = GetWorldPosition();

	elapsedTime_ += deltaTime;

	if(elapsedTime_ < emitDelay_) return;

	if(emitDuration_ >= 0.0f && elapsedTime_ > emitDelay_ + emitDuration_) {
		Stop();
	}

	if(isOneShot_) {
		if(!hasEmitted_) {
			for(int i = 0; i < emitCount_ && units_.size() < kMaxUnits_; ++i) {
				Emit(); // 自動的に GetWorldPosition() 使用
			}
			hasEmitted_ = true;
		}
	} else {
		if(isFirstFrame_) {
			prevPostion_  = position_;
			isFirstFrame_ = false;
		}

		Vector3 moveDelta = position_ - prevPostion_;
		float	distance  = moveDelta.Length();

		if(distance > 0.0f && isComplement_) {
			float spawnInterval = 0.02f;
			int	  trailCount	= static_cast<int>(distance / spawnInterval);
			for(int i = 0; i < trailCount; ++i) {
				float	dist	 = i * spawnInterval;
				float	t		 = dist / distance;
				Vector3 spawnPos = Vector3::Lerp(prevPostion_, position_, t);
				Emit(spawnPos); // 明示的に座標指定
			}
		} else {
			emitTimer_ += deltaTime;
			const float interval = emitRate_;
			if(emitTimer_ >= interval && units_.size() < kMaxUnits_) {
				emitTimer_ -= interval;
				Emit(); // 自動でワールド座標
			}
		}
		prevPostion_ = position_;
	}

	// パーティクル更新処理
	for(auto& fx : units_) {
		if(!fx.alive) continue;

		if(fx.lifetime > 0.0f) {
			float t	 = fx.age / fx.lifetime;
			fx.lifeT = (t < 0.f) ? 0.f : (t > 1.f ? 1.f : t);
		} else {
			fx.lifeT = 1.0f;
		}

		for(auto& m : moduleContainer_->GetModules()) {
			if(m->IsEnabled()) m->OnUpdate(fx, deltaTime);
		}

		if(!isStatic_) fx.position += fx.velocity * deltaTime;

		fx.age += deltaTime;
		if(fx.age >= fx.lifetime) fx.alive = false;

		Matrix4x4 uvTransformMatrix = Cx::Math::MakeScaleMatrix(Vector3(fx.uvTransform.scale.x, fx.uvTransform.scale.y, 1.0f));
		uvTransformMatrix			= Matrix4x4::Multiply(uvTransformMatrix, Cx::Math::MakeRotateZMatrix(fx.uvTransform.rotate));
		uvTransformMatrix			= Matrix4x4::Multiply(uvTransformMatrix, Cx::Math::MakeTranslateMatrix(Vector3(fx.uvTransform.translate.x, fx.uvTransform.translate.y, 0.0f)));
		material_.uvTransform		= uvTransformMatrix;
	}

	materialBuffer_.TransferData(material_);
	std::erase_if(units_, [](const FxUnit& fx) { return !fx.alive; });

	bool shouldNotify =
		(isOneShot_ && hasEmitted_ && units_.empty()) ||
		(emitDuration_ >= 0.0f && elapsedTime_ > emitDelay_ + emitDuration_ && units_.empty());

	if(shouldNotify && !isFinishedNotified_) {
		isFinishedNotified_ = true;
		Stop();
		if(onFinished_) {
			onFinished_();
		}
	}
}

void FxEmitter::SetOnFinishedCallback(std::function<void()> callback) {
	onFinished_ = std::move(callback);
}

/////////////////////////////////////////////////////////////////////////////////////////
//			発生
/////////////////////////////////////////////////////////////////////////////////////////
void FxEmitter::Emit() {
	Emit(GetWorldPosition());
}

void FxEmitter::Emit(const Vector3& pos) {
	if(units_.size() >= kMaxUnits_) return;

	FxUnit fx;
	ResetFxUnit(fx);
	fx.position = pos; // 引数位置で初期化
	units_.push_back(fx);
}

/////////////////////////////////////////////////////////////////////////////////////////
//			リセット
/////////////////////////////////////////////////////////////////////////////////////////
void FxEmitter::ResetFxUnit(FxUnit& fx) {
	fx.position		= position_;
	fx.scale		= scale_.Get();
	fx.velocity		= velocity_.Get();
	fx.lifetime		= lifetime_.Get();
	fx.age			= 0.0f;
	fx.initialScale = fx.scale; // 初期スケールを設定
	fx.color		= Vector4(1, 1, 1, 1);
	fx.alive		= true;
	fx.uvTransform.Initialize();
}

/////////////////////////////////////////////////////////////////////////////////////////
//			gui表示
/////////////////////////////////////////////////////////////////////////////////////////
void FxEmitter::ShowGui() {
	ImGui::PushID(this);

	// ---- 上部ミニバー：よく触る項目をサッと ----
	if(ImGui::BeginChild("mini", ImVec2(0, 48), false, ImGuiWindowFlags_NoScrollbar)) {
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Quick Controls");
		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		if(ImGui::Button("Play")) {
			Play();
			ImGui::SameLine();
		}
		if(ImGui::Button("Stop")) {
			Stop();
			ImGui::SameLine();
		}
		if(ImGui::Button("Reset")) {
			Reset();
		}

		ImGui::SameLine();
		VSeparator();
		ImGui::SameLine();

		ImGui::TextUnformatted("Rate");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120);
		GuiCmd::DragFloat("##rate_top", emitRate_, 0.01f, 0.0f, 10.0f);
		ImGui::SameLine();

		ImGui::TextUnformatted("OneShot");
		ImGui::SameLine();
		GuiCmd::CheckBox("##oneshot_top", isOneShot_);

		ImGui::SameLine();
		ImGui::TextUnformatted("Draw");
		ImGui::SameLine();
		GuiCmd::CheckBox("##draw_top", isDrawEnable_);
	}
	ImGui::EndChild();

	// ================= Material =================
	if(FxGui::GridScope sec{"Material"}; sec.open) {
		// Color
		FxGui::RowLabel("Color");
		ImGui::ColorEdit4("##color", &material_.color.x);

		// Texture (path表示 + 選択ボタン)
		FxGui::RowLabel("Texture");
		ImGui::BeginGroup();
		ImGui::TextUnformatted(material_.texturePath.c_str());
		ImGui::SameLine();
		if(ImGui::Button("Select...")) {
			IGFD::FileDialogConfig c;
			c.path = "Resources/Assets/Textures/";
			ImGuiFileDialog::Instance()->OpenDialog("ChooseTex", "Select Texture", ".png,.jpg,.dds", c);
		}
		if(ImGuiFileDialog::Instance()->Display("ChooseTex")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				material_.texturePath = ImGuiFileDialog::Instance()->GetCurrentFileName();
			}
			ImGuiFileDialog::Instance()->Close();
		}
		ImGui::EndGroup();
	}

	// ================= Emission =================
	if(FxGui::GridScope sec{"Emission"}; sec.open) {
		FxGui::RowLabel("Alive Count");
		ImGui::Text("%zu", units_.size());

		FxGui::RowLabel("World Position");
		GuiCmd::DragFloat3("##pos", position_);

		FxGui::RowLabel("Emit Rate (sec)");
		GuiCmd::DragFloat("##rate", emitRate_, 0.01f, 0.0f, 10.0f);

		FxGui::RowLabel("Complement Trail");
		GuiCmd::CheckBox("##comp", isComplement_);

		FxGui::RowLabel("Static");
		GuiCmd::CheckBox("##static", isStatic_);
	}

	// ================= Params =================
	if(FxGui::GridScope sec{"Params"}; sec.open) {
		FxGui::DrawParam("Scale", scale_);
		FxGui::DrawParam("Velocity", velocity_);
		FxGui::DrawParam("Lifetime", lifetime_);
	}

	// ================= Playback =================
	if(FxGui::GridScope sec{"Playback"}; sec.open) {
		FxGui::RowLabel("Controls");
		ImGui::BeginGroup();
		if(ImGui::Button("Play")) {
			Play();
		}
		ImGui::SameLine();
		if(ImGui::Button("Stop")) {
			Stop();
		}
		ImGui::SameLine();
		if(ImGui::Button("Reset")) {
			Reset();
		}
		ImGui::EndGroup();

		FxGui::RowLabel("Draw Enable");
		GuiCmd::CheckBox("##draw", isDrawEnable_);
	}

	// ================= One-Shot =================
	if(FxGui::GridScope sec{"One-Shot"}; sec.open) {
		FxGui::RowLabel("Enable");
		if(GuiCmd::CheckBox("##oneshot", isOneShot_)) {
			if(!isOneShot_) {
				hasEmitted_ = false;
			} // OFFに戻した時の自然な継続
		}

		ImGui::BeginDisabled(!isOneShot_);
		FxGui::RowLabel("Emit Count");
		ImGui::DragInt("##count", &emitCount_, 1, 1, kMaxUnits_);

		FxGui::RowLabel("Auto Destroy");
		GuiCmd::CheckBox("##autoDestroy", autoDestroy_);

		FxGui::RowLabel("Delay (sec)");
		GuiCmd::DragFloat("##delay", emitDelay_, 0.01f, 0.0f, 10.0f);
		ImGui::EndDisabled();

		ImGui::BeginDisabled(isOneShot_);
		FxGui::RowLabel("Emit Duration (sec)");
		GuiCmd::DragFloat("##duration", emitDuration_, 0.01f, -1.0f, 60.0f);
		ImGui::EndDisabled();
	}

	// ================= Modules =================
	if(moduleContainer_) {
		if(FxGui::GridScope sec{"Modules"}; sec.open) {
			// 左: 現在のモジュール一覧 / 右: 追加パレット
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::BeginChild("mod_list", ImVec2(0, 200), true);
			moduleContainer_->ShowModulesGui(); // 有効/無効・順序など
			ImGui::EndChild();

			ImGui::TableSetColumnIndex(1);
			ImGui::BeginChild("mod_add", ImVec2(0, 200), true);
			moduleContainer_->ShowAvailableModulesGui(); // 追加用
			ImGui::EndChild();
		}
	}

	ImGui::Spacing();
	ImGui::PopID();
}

/////////////////////////////////////////////////////////////////////////////////////////
//			コンフィグの適用
/////////////////////////////////////////////////////////////////////////////////////////
void FxEmitter::ApplyConfigFrom(const EmitterConfig& config) {
	position_		= config.position;
	material_.color = config.color;
	velocity_.FromConfig(config.velocity);
	lifetime_.FromConfig(config.lifetime);
	scale_.FromConfig(config.scale);
	emitRate_			  = config.emitRate;
	modelPath			  = config.modelPath;
	material_.texturePath = config.texturePath;
	isDrawEnable_		  = config.isDrawEnable;
	isComplement_		  = config.isComplement;
	isStatic_			  = config.isStatic;

	moduleContainer_ = std::make_unique<FxModuleContainer>(config.modules);

	isOneShot_	  = config.isOneShot;
	autoDestroy_  = config.autoDestroy;
	emitCount_	  = config.emitCount;
	emitDelay_	  = config.emitDelay;
	emitDuration_ = config.emitDuration;

	// 再生状態を初期化
	isFirstFrame_ = true;
	hasEmitted_	  = false;
	elapsedTime_  = 0.0f;
	isPlaying_	  = true;
}

void FxEmitter::ExtractConfigTo(EmitterConfig& config) const {
	config.position		= position_;
	config.color		= material_.color;
	config.velocity		= FxVector3ParamConfig{velocity_.ToConfig()};
	config.lifetime		= FxFloatParamConfig{lifetime_.ToConfig()};
	config.scale		= FxVector3ParamConfig{scale_.ToConfig()};
	config.emitRate		= emitRate_;
	config.modelPath	= modelPath;
	config.texturePath	= material_.texturePath;
	config.isDrawEnable = isDrawEnable_;
	config.isComplement = isComplement_;
	config.isStatic		= isStatic_;

	// モジュール情報を保存
	if(moduleContainer_) {
		config.modules = moduleContainer_->ExtractConfigs();
	} else {
		config.modules.clear();
	}

	config.isOneShot	= isOneShot_;
	config.autoDestroy	= autoDestroy_;
	config.emitCount	= emitCount_;
	config.emitDelay	= emitDelay_;
	config.emitDuration = emitDuration_;
}

void FxEmitter::Play() {
	isPlaying_	  = true;
	isFirstFrame_ = true;

	if(isOneShot_) {
		// OneShot 時は状態も初期化しておく
		hasEmitted_	 = false;
		elapsedTime_ = 0.0f;
	}
}

void FxEmitter::Stop() {
	isPlaying_ = false;
}

void FxEmitter::Reset() {
	units_.clear();
	emitTimer_	  = 0.0f;
	elapsedTime_  = 0.0f;
	isFirstFrame_ = true;
	hasEmitted_	  = false;
}