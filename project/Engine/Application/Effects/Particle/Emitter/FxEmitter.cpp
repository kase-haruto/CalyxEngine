#include "FxEmitter.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Module/Factory/ModuleFactory.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>

// externals
#include <externals/imgui/imgui.h>
#include <externals/imgui/ImGuiFileDialog.h>

#include "Engine/Foundation/Utility/Func/CxUtils.h"

FxEmitter::FxEmitter() {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	// マテリアルの初期化
	material_.color = Vector4(1, 1, 1, 1);
	materialBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice());

	instanceBuffer_.Initialize(device, kMaxUnits_);
	instanceBuffer_.CreateSrv(device);

	velocity_ = FxParam<Vector3>::MakeRandom(
		Vector3(-1.0f, 0.0f, -1.0f),
		Vector3(1.0f, 0.0f, 1.0f)
	);

	lifetime_ = FxParam<float>::MakeRandom(1.0f, 3.0f);
	scale_ = FxParam<Vector3>::MakeConstant();

	//モジュールの初期化
	moduleContainer_ = std::make_unique<FxModuleContainer>();
}

FxEmitter::~FxEmitter() {
	instanceBuffer_.ReleaseSrv();
}

/////////////////////////////////////////////////////////////////////////////////////////
//			更新
/////////////////////////////////////////////////////////////////////////////////////////
void FxEmitter::Update(float deltaTime) {
	if (!isPlaying_) return;

	// ワールド座標に追従
	position_ = GetWorldPosition();

	elapsedTime_ += deltaTime;

	if (elapsedTime_ < emitDelay_) return;

	if (emitDuration_ >= 0.0f && elapsedTime_ > emitDelay_ + emitDuration_) {
		Stop();
	}

	if (isOneShot_) {
		if (!hasEmitted_) {
			for (int i = 0; i < emitCount_ && units_.size() < kMaxUnits_; ++i) {
				Emit();  // 自動的に GetWorldPosition() 使用
			}
			hasEmitted_ = true;
		}
	} else {
		if (isFirstFrame_) {
			prevPostion_ = position_;
			isFirstFrame_ = false;
		}

		Vector3 moveDelta = position_ - prevPostion_;
		float distance = moveDelta.Length();

		if (distance > 0.0f && isComplement_) {
			float spawnInterval = 0.02f;
			int trailCount = static_cast<int>(distance / spawnInterval);
			for (int i = 0; i < trailCount; ++i) {
				float dist = i * spawnInterval;
				float t = dist / distance;
				Vector3 spawnPos = Vector3::Lerp(prevPostion_, position_, t);
				Emit(spawnPos);  // 明示的に座標指定
			}
		} else {
			emitTimer_ += deltaTime;
			const float interval = emitRate_;
			if (emitTimer_ >= interval && units_.size() < kMaxUnits_) {
				emitTimer_ -= interval;
				Emit();  // 自動でワールド座標
			}
		}
		prevPostion_ = position_;
	}

	// パーティクル更新処理（省略なしで元のまま）
	for (auto& fx : units_) {
		if (!fx.alive) continue;

		for (auto& m : moduleContainer_->GetModules()) {
			if (m->IsEnabled()) m->OnUpdate(fx, deltaTime);
		}

		if (!isStatic_) fx.position += fx.velocity * deltaTime;

		fx.age += deltaTime;
		if (fx.age >= fx.lifetime) fx.alive = false;

		Matrix4x4 uvTransformMatrix = Cx::Math::MakeScaleMatrix(Vector3(fx.uvTransform.scale.x, fx.uvTransform.scale.y, 1.0f));
		uvTransformMatrix = Matrix4x4::Multiply(uvTransformMatrix, Cx::Math::MakeRotateZMatrix(fx.uvTransform.rotate));
		uvTransformMatrix = Matrix4x4::Multiply(uvTransformMatrix, Cx::Math::MakeTranslateMatrix(Vector3(fx.uvTransform.translate.x, fx.uvTransform.translate.y, 0.0f)));
		material_.uvTransform = uvTransformMatrix;
	}

	materialBuffer_.TransferData(material_);
	std::erase_if(units_, [](const FxUnit& fx) { return !fx.alive; });

	bool shouldNotify =
		(isOneShot_ && hasEmitted_ && units_.empty()) ||
		(emitDuration_ >= 0.0f && elapsedTime_ > emitDelay_ + emitDuration_ && units_.empty());

	if (shouldNotify && !isFinishedNotified_) {
		isFinishedNotified_ = true;
		Stop();
		if (onFinished_) {
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
	if (units_.size() >= kMaxUnits_) return;

	FxUnit fx;
	ResetFxUnit(fx);
	fx.position = pos; // ← 引数位置で初期化
	units_.push_back(fx);
}

/////////////////////////////////////////////////////////////////////////////////////////
//			リセット
/////////////////////////////////////////////////////////////////////////////////////////
void FxEmitter::ResetFxUnit(FxUnit& fx) {
	fx.position = position_;
	fx.scale = scale_.Get();
	fx.velocity = velocity_.Get();
	fx.lifetime = lifetime_.Get();
	fx.age = 0.0f;
	fx.initialScale = fx.scale; // 初期スケールを設定
	fx.color = Vector4(1, 1, 1, 1);
	fx.alive = true;
	fx.uvTransform.Initialize();
}

/////////////////////////////////////////////////////////////////////////////////////////
//			gui表示
/////////////////////////////////////////////////////////////////////////////////////////
void FxEmitter::ShowGui() {
	ImGui::PushID(this);

	// =============================
	// マテリアルセクション
	// =============================
	ImGui::SeparatorText("Material");
	ImGui::ColorEdit4("Color", &material_.color.x);

	// 現在のパスを表示
	ImGui::Text("Texture: %s", material_.texturePath.c_str());

	// ファイル選択ボタン
	if (ImGui::Button("Select Texture")) {
		IGFD::FileDialogConfig config;
		config.path = "Resources/Assets/Textures/";
		ImGuiFileDialog::Instance()->OpenDialog(
			"ChooseTex",
			"Select Texture",
			".png",
			config
		);
	}

	// ダイアログの描画と選択結果処理
	if (ImGuiFileDialog::Instance()->Display("ChooseTex")) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			// ファイル名だけを取得
			std::string fileNameOnly = ImGuiFileDialog::Instance()->GetCurrentFileName();
			material_.texturePath = fileNameOnly;
		}
		ImGuiFileDialog::Instance()->Close();
	}

	// =============================
	// Emit設定
	// =============================
	ImGui::SeparatorText("Emit");
	ImGui::Text("emitCount: %d", units_.size());
	GuiCmd::DragFloat3("position", position_);
	GuiCmd::DragFloat("emitRate", emitRate_, 0.01f, 0.0f, 10.0f);

	GuiCmd::CheckBox("isComplement", isComplement_);
	GuiCmd::CheckBox("isStatic", isStatic_);

	ImGuiHelpers::DrawFxParamGui("Scale", scale_);
	ImGuiHelpers::DrawFxParamGui("Velocity", velocity_);
	ImGuiHelpers::DrawFxParamGui("Lifetime", lifetime_);

	// =============================
	// 再生制御
	// =============================
	ImGui::Spacing();
	ImGui::SeparatorText("Emitter Controls");
	if (ImGui::Button("Play")) { Play(); }
	ImGui::SameLine();
	if (ImGui::Button("Stop")) { Stop(); }
	ImGui::SameLine();
	if (ImGui::Button("Reset")) { Reset(); }

	// =============================
	// OneShot
	// =============================
	ImGui::Spacing();
	ImGui::SeparatorText("OneShot Settings");
	GuiCmd::CheckBox("OneShot", isOneShot_);
	if (isOneShot_) {
		ImGui::DragInt("Emit Count", &emitCount_, 1, 1, kMaxUnits_);
		GuiCmd::CheckBox("Auto Destroy", autoDestroy_);
		GuiCmd::DragFloat("Emit Delay", emitDelay_, 0.01f, 0.0f, 10.0f);
	} else {
		GuiCmd::DragFloat("Emit Duration", emitDuration_, 0.01f, -1.0f, 60.0f);
	}

	// =============================
	// モジュール
	// =============================
	if (moduleContainer_) {
		moduleContainer_->ShowModulesGui();
		moduleContainer_->ShowAvailableModulesGui();
	}

	ImGui::PopID();
}

/////////////////////////////////////////////////////////////////////////////////////////
//			コンフィグの適用
/////////////////////////////////////////////////////////////////////////////////////////
void FxEmitter::ApplyConfigFrom(const EmitterConfig& config) {
	position_ = config.position;
	material_.color = config.color;
	velocity_.FromConfig(config.velocity);
	lifetime_.FromConfig(config.lifetime);
	scale_.FromConfig(config.scale);
	emitRate_ = config.emitRate;
	modelPath = config.modelPath;
	material_.texturePath = config.texturePath;
	isDrawEnable_ = config.isDrawEnable;
	isComplement_ = config.isComplement;
	isStatic_ = config.isStatic;

	moduleContainer_ = std::make_unique<FxModuleContainer>(config.modules);

	isOneShot_ = config.isOneShot;
	autoDestroy_ = config.autoDestroy;
	emitCount_ = config.emitCount;
	emitDelay_ = config.emitDelay;
	emitDuration_ = config.emitDuration;

	// 再生状態を初期化
	isFirstFrame_ = true;
	hasEmitted_ = false;
	elapsedTime_ = 0.0f;
	isPlaying_ = true;
}

void FxEmitter::ExtractConfigTo(EmitterConfig& config) const {
	config.position = position_;
	config.color = material_.color;
	config.velocity = FxVector3ParamConfig{ velocity_.ToConfig() };
	config.lifetime = FxFloatParamConfig{ lifetime_.ToConfig() };
	config.scale = FxVector3ParamConfig{ scale_.ToConfig() };
	config.emitRate = emitRate_;
	config.modelPath = modelPath;
	config.texturePath = material_.texturePath;
	config.isDrawEnable = isDrawEnable_;
	config.isComplement = isComplement_;
	config.isStatic = isStatic_;

	// モジュール情報を保存
	if (moduleContainer_) {
		config.modules = moduleContainer_->ExtractConfigs();
	} else {
		config.modules.clear();
	}

	config.isOneShot = isOneShot_;
	config.autoDestroy = autoDestroy_;
	config.emitCount = emitCount_;
	config.emitDelay = emitDelay_;
	config.emitDuration = emitDuration_;
}


void FxEmitter::Play() {
	isPlaying_ = true;
	isFirstFrame_ = true;

	if (isOneShot_) {
		// OneShot 時は状態も初期化しておく
		hasEmitted_ = false;
		elapsedTime_ = 0.0f;
	}
}


void FxEmitter::Stop() {
	isPlaying_ = false;
}

void FxEmitter::Reset() {
	units_.clear();
	emitTimer_ = 0.0f;
	elapsedTime_ = 0.0f;
	isFirstFrame_ = true;
	hasEmitted_ = false;
}