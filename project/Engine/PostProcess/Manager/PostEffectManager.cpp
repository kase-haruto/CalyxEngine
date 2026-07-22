#include "PostEffectManager.h"
#include <CalyxEngine/Project.h>
#include <Engine/Foundation/Debug/CxAssert.h>

// engine
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Foundation/Utility/FileSystem/FileSystemHelper.h>

// externals
#include <externals/imgui/imgui.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <externals/nlohmann/json.hpp>

// c++
#include "Engine/Foundation/Utility/Ease/CxEase.h"

#include <filesystem>
#include <fstream>
#include <unordered_set>

PostEffectManager* PostEffectManager::Get(){
	static PostEffectManager instance;
	return &instance;
}

namespace {
	constexpr const char* kApplyAlways = "Always";
	constexpr const char* kApplyTriggered = "Triggered";

	const char* ToString(PostEffectApplyMode mode) {
		return mode == PostEffectApplyMode::Triggered ? kApplyTriggered : kApplyAlways;
	}

	PostEffectApplyMode ApplyModeFromString(const std::string& value) {
		return value == kApplyTriggered ? PostEffectApplyMode::Triggered : PostEffectApplyMode::Always;
	}

	const char* EaseToString(CalyxEngine::EaseType ease) {
		switch(ease) {
		case CalyxEngine::EaseType::Linear: return "Linear";
		case CalyxEngine::EaseType::EaseInQuad: return "EaseInQuad";
		case CalyxEngine::EaseType::EaseOutQuad: return "EaseOutQuad";
		case CalyxEngine::EaseType::EaseInOutQuad: return "EaseInOutQuad";
		case CalyxEngine::EaseType::EaseInCubic: return "EaseInCubic";
		case CalyxEngine::EaseType::EaseOutCubic: return "EaseOutCubic";
		case CalyxEngine::EaseType::EaseInOutCubic: return "EaseInOutCubic";
		case CalyxEngine::EaseType::EaseInSine: return "EaseInSine";
		case CalyxEngine::EaseType::EaseOutSine: return "EaseOutSine";
		case CalyxEngine::EaseType::EaseInOutSine: return "EaseInOutSine";
		case CalyxEngine::EaseType::EaseInExpo: return "EaseInExpo";
		case CalyxEngine::EaseType::EaseOutExpo: return "EaseOutExpo";
		case CalyxEngine::EaseType::EaseInOutExpo: return "EaseInOutExpo";
		case CalyxEngine::EaseType::EaseInBack: return "EaseInBack";
		case CalyxEngine::EaseType::EaseOutBack: return "EaseOutBack";
		case CalyxEngine::EaseType::EaseInOutBack: return "EaseInOutBack";
		default: return "Linear";
		}
	}

	CalyxEngine::EaseType EaseFromString(const std::string& value) {
		for(int32_t i = 0; i < static_cast<int32_t>(CalyxEngine::EaseType::Count); ++i) {
			auto ease = static_cast<CalyxEngine::EaseType>(i);
			if(value == EaseToString(ease)) return ease;
		}
		return CalyxEngine::EaseType::Linear;
	}

	nlohmann::json FloatAnimationsToJson(const std::vector<PostEffectFloatAnimation>& animations) {
		nlohmann::json root = nlohmann::json::array();
		for(const auto& anim : animations) {
			root.push_back({
				{"parameter", anim.parameter},
				{"from", anim.from},
				{"to", anim.to},
				{"useCurrentAsFrom", anim.useCurrentAsFrom}
			});
		}
		return root;
	}

	std::vector<PostEffectFloatAnimation> FloatAnimationsFromJson(const nlohmann::json& root) {
		std::vector<PostEffectFloatAnimation> animations;
		if(!root.is_array()) return animations;
		for(const auto& item : root) {
			PostEffectFloatAnimation anim;
			anim.parameter = item.value("parameter", "");
			anim.from = item.value("from", 0.0f);
			anim.to = item.value("to", 0.0f);
			anim.useCurrentAsFrom = item.value("useCurrentAsFrom", false);
			if(!anim.parameter.empty()) animations.push_back(std::move(anim));
		}
		return animations;
	}
}

void PostEffectManager::Initialize(PipelineService* service,bool enableAll){
	CX_CHECK(service, "Assertion failed");
	collection_.Initialize(service);
	collection_.BuildInitialSlots(enableAll);

	// CopyImage と Blend はグラフ内の専用ノードとして扱う
	for (auto& s : collection_.GetSlots()){ if (s.name == kCopyImageName || s.name == kBlendName) s.enabled = false; }

	//defaultのeffectを使用する


	dirty_ = true;
	initialized_ = true;

	LoadPreset(kDefaultPaht);
}

// ---------- Toggle ----------
int PostEffectManager::IndexOf(const std::string& name) const{
	const auto& slots = collection_.GetSlots();
	for (int i = 0; i < (int)slots.size(); ++i){ if (slots[i].name == name) return i; }
	return -1;
}

void PostEffectManager::Enable(const std::string& name, bool enabled){
	if (!initialized_) return;
	const int idx = IndexOf(name);
	if (idx < 0) return;
	if (collection_.GetSlots()[idx].name == kCopyImageName) return;
	collection_.GetSlots()[idx].enabled = enabled;
	MarkDirty();
}

void PostEffectManager::Toggle(const std::string& name){
	if (!initialized_) return;
	const int idx = IndexOf(name);
	if (idx < 0) return;
	if (collection_.GetSlots()[idx].name == kCopyImageName) return;
	auto& s = collection_.GetSlots()[idx];
	s.enabled = !s.enabled;
	MarkDirty();
}

bool PostEffectManager::IsEnabled(const std::string& name) const{
	const int idx = IndexOf(name);
	if (idx < 0) return false;
	return collection_.GetSlots()[idx].enabled;
}

void PostEffectManager::EnableOnly(std::initializer_list<std::string> names){
	if (!initialized_) return;
	std::unordered_set<std::string> pick(names.begin(),names.end());
	for (auto& s : collection_.GetSlots()){
		if (s.name == kCopyImageName){
			s.enabled = false;
			continue;
		}
		if (s.name == kBlendName){
			s.enabled = false;
			continue;
		}
		s.enabled = (pick.find(s.name) != pick.end());
	}
	MarkDirty();
}

void PostEffectManager::EnableAll(){
	if (!initialized_) return;
	for (auto& s : collection_.GetSlots()){
		if (s.name == kCopyImageName){
			s.enabled = false;
			continue;
		}
		if (s.name == kBlendName){
			s.enabled = false;
			continue;
		}
		s.enabled = true;
	}
	MarkDirty();
}

void PostEffectManager::DisableAll(){
	if (!initialized_) return;
	for (auto& s : collection_.GetSlots()) s.enabled = false;
	MarkDirty();
}

// ---------- Order ----------
bool PostEffectManager::MoveUp(const std::string& name){
	auto& slots = collection_.GetSlots();
	const int i = IndexOf(name);
	if (i <= 0) return false;
	if (slots[i].name == kCopyImageName) return false;
	std::swap(slots[i - 1],slots[i]);
	MarkDirty();
	return true;
}

bool PostEffectManager::MoveDown(const std::string& name){
	auto& slots = collection_.GetSlots();
	const int i = IndexOf(name);
	if (i < 0 || i >= (int)slots.size() - 1) return false;
	if (slots[i].name == kCopyImageName) return false;
	std::swap(slots[i],slots[i + 1]);
	MarkDirty();
	return true;
}

void PostEffectManager::SetOrder(const std::vector<std::string>& orderedNames){
	auto old = collection_.GetSlots();
	std::vector<PostEffectSlot> re;
	re.reserve(old.size());

	auto pick = [&](const std::string& n){
		auto it = std::find_if(old.begin(),old.end(),
							   [&](const PostEffectSlot& s){ return s.name == n; });
		if (it != old.end()){
			re.push_back(*it);
			old.erase(it);
		}
	};
	for (auto& n : orderedNames) pick(n);
	for (auto& s : old) re.push_back(s);

	collection_.GetSlots() = std::move(re);
	MarkDirty();
}

bool PostEffectManager::SavePreset(const std::string& filePath, const std::string& presetName) const{
	if(!initialized_) return false;

	nlohmann::json root;
	root["type"] = "PostEffectPreset";
	root["name"] = presetName;
	root["version"] = 1;
	root["outline"] = {
		{"enabled", outlineEnabled_}
	};
	root["nodes"] = nlohmann::json::array();

	int32_t nodeId = 1;
	for(const auto& slot : collection_.GetSlots()){
		if(slot.name == kCopyImageName) continue;
		nlohmann::json node;
		node["id"] = nodeId++;
		node["type"] = slot.name;
		node["title"] = slot.name;
		node["enabled"] = slot.enabled;
		node["applyMode"] = ToString(slot.applyMode);
		node["duration"] = slot.duration;
		node["ease"] = EaseToString(slot.ease);
		node["autoDisable"] = slot.autoDisable;
		node["parameters"] = slot.pass ? slot.pass->SaveParameters() : nlohmann::json::object();
		node["floatAnimations"] = FloatAnimationsToJson(slot.floatAnimations);
		root["nodes"].push_back(std::move(node));
	}

	try{
		std::filesystem::path path(Calyx::ResolveAssetPath(filePath));
		FileSystemHelper::CreateDirectoryPath(path.parent_path().string());
		std::ofstream ofs(path);
		if(!ofs) return false;
		ofs << root.dump(2);
		return true;
	}catch(...){
		return false;
	}
}

bool PostEffectManager::LoadPreset(const std::string& filePath){
	if(!initialized_) return false;

	nlohmann::json root;
	try{
		std::ifstream ifs(Calyx::ResolveAssetPath(filePath));
		if(!ifs) return false;
		ifs >> root;
	}catch(...){
		return false;
	}

	if(!root.contains("nodes") || !root["nodes"].is_array()) return false;

	// 別プリセットの再生状態を新しいシーンへ持ち越さない。
	// 古いTweenが完了して、新しく読み込んだ同名パスを無効化する競合も防ぐ。
	floatTweens_.clear();
	loadedPreset_ = root;
	hasLoadedGraph_ = root.contains("graph") && root["graph"].is_object();
	if(root.contains("outline") && root["outline"].is_object()){
		outlineEnabled_ = root["outline"].value("enabled", outlineEnabled_);
	}

	auto old = collection_.GetSlots();
	std::vector<PostEffectSlot> loaded;
	loaded.reserve(old.size());

	auto takeSlot = [&](const std::string& name) -> std::optional<PostEffectSlot>{
		auto it = std::find_if(old.begin(), old.end(), [&](const PostEffectSlot& slot){ return slot.name == name; });
		if(it == old.end()) return std::nullopt;
		PostEffectSlot slot = *it;
		old.erase(it);
		return slot;
	};

	for(const auto& node : root["nodes"]){
		const std::string type = node.value("type", "");
		auto slotOpt = takeSlot(type);
		if(!slotOpt.has_value()) continue;

		auto slot = std::move(*slotOpt);
		slot.enabled = node.value("enabled", slot.enabled);
		slot.applyMode = ApplyModeFromString(node.value("applyMode", std::string(kApplyAlways)));
		slot.duration = (std::max)(0.0f, node.value("duration", slot.duration));
		if(node.contains("easeIndex") && node["easeIndex"].is_number_integer()){
			const int easeIndex = node["easeIndex"].get<int>();
			if(easeIndex >= 0 && easeIndex < static_cast<int>(CalyxEngine::EaseType::Count)){
				slot.ease = static_cast<CalyxEngine::EaseType>(easeIndex);
			}
		}else{
			slot.ease = EaseFromString(node.value("ease", std::string(EaseToString(slot.ease))));
		}
		slot.autoDisable = node.value("autoDisable", slot.autoDisable);
		slot.floatAnimations = node.contains("floatAnimations")
			? FloatAnimationsFromJson(node["floatAnimations"])
			: std::vector<PostEffectFloatAnimation>{};

		if(slot.name == kBlendName){
			slot.enabled = true;
		}
		if(slot.applyMode == PostEffectApplyMode::Triggered){
			slot.enabled = false;
		}
		if(slot.pass && node.contains("parameters") && node["parameters"].is_object()){
			slot.pass->LoadParameters(node["parameters"]);
		}

		loaded.push_back(std::move(slot));
	}

	for(auto& slot : old){
		if(slot.name == kCopyImageName || slot.name == kBlendName) slot.enabled = false;
		loaded.push_back(std::move(slot));
	}

	collection_.GetSlots() = std::move(loaded);
	MarkDirty();
	return true;
}

void PostEffectManager::PlayTriggeredEffects(){
	if(!initialized_) return;
	for(const auto& slot : collection_.GetSlots()){
		if(slot.applyMode == PostEffectApplyMode::Triggered){
			PlayTriggeredEffect(slot.name);
		}
	}
}

void PostEffectManager::PlayTriggeredEffect(const std::string& name){
	if(!initialized_) return;
	const int idx = IndexOf(name);
	if(idx < 0) return;
	auto& slot = collection_.GetSlots()[idx];
	if(slot.name == kCopyImageName || slot.applyMode != PostEffectApplyMode::Triggered || !slot.pass) return;

	// 同じエフェクトが連続発生した場合、前回の完了処理で今回の再生が
	// 途中停止しないよう、対象パスのTweenを今回の再生へ置き換える。
	std::erase_if(floatTweens_, [&name](const FloatTween& tween) {
		return tween.passName == name;
	});

	Enable(slot.name, true);

	for(const auto& anim : slot.floatAnimations){
		float current = 0.0f;
		const bool hasCurrent = slot.pass->GetFloatParameter(anim.parameter, current);
		const float from = anim.useCurrentAsFrom && hasCurrent ? current : anim.from;
		TweenFloat(slot.name,
				   [pass = slot.pass, param = anim.parameter]() {
					   float value = 0.0f;
					   pass->GetFloatParameter(param, value);
					   return value;
				   },
				   [pass = slot.pass, param = anim.parameter](float value) {
					   pass->SetFloatParameter(param, value);
				   },
				   from,
				   anim.to,
				   slot.duration,
				   slot.ease,
				   slot.autoDisable,
				   nullptr);
	}

	if(slot.floatAnimations.empty() && slot.autoDisable){
		TweenFloat(slot.name,
				   []() { return 1.0f; },
				   [](float) {},
				   1.0f,
				   0.0f,
				   slot.duration,
				   slot.ease,
				   true,
				   nullptr);
	}
}

// ---------- Update / Execute ----------
void PostEffectManager::RebuildGraphIfDirty(){
	if (!dirty_) return;
	if(hasLoadedGraph_){
		graph_.SetGraphFromJson(loadedPreset_, collection_.GetSlots());
	}else{
		graph_.SetPassesFromList(collection_.GetSlots()); // enabled だけ反映
	}
	dirty_ = false;
}

void PostEffectManager::Update(float dt){
	if (!initialized_) return;

	std::vector<int> toRemove;                      // 削除する tween のインデックス
	std::vector<std::string> toDisablePass;         // 後で Enable(false) するパス名
	std::vector<std::function<void()>> completions; // 後で実行する onComplete

	for (size_t i = 0; i < floatTweens_.size(); ++i){
		auto& tw = floatTweens_[i];

		tw.t += dt;
		const float r = std::clamp(tw.t / ( std::max ) (0.0001f, tw.dur), 0.f, 1.f);
		const float k = CalyxEngine::ApplyEase(tw.ease, r);
		const float v = std::lerp(tw.start, tw.end, k);

		if (tw.setter) tw.setter(v);

		const bool finished = (r >= 1.f);
		if (finished){
			if (tw.autoDisableIfZero && std::fabs(tw.end) <= 1e-4f){
				toDisablePass.push_back(tw.passName); // 今は無効化しない（後でまとめて）
			}
			if (tw.onComplete){
				completions.push_back(std::move(tw.onComplete)); // 今は呼ばない（後で）
			}
			toRemove.push_back(static_cast< int >(i)); // 今は消さない（後で）
		}
	}

	// さくじょ
	for (int j = static_cast< int >(toRemove.size()) - 1; j >= 0; --j){
		const int idx = toRemove[j];
		if (idx >= 0 && idx < static_cast< int >(floatTweens_.size())){
			floatTweens_.erase(floatTweens_.begin() + idx);
		}
	}

	// Enable(false) もループ後に
	for (const auto& name : toDisablePass){
		Enable(name, false);
	}

	for (auto& fn : completions){
		if (fn) fn();
	}

	// 各パスの Tick
	for (auto& s : collection_.GetSlots()){
		if (s.pass) s.pass->Tick(dt);
	}
}


void PostEffectManager::Execute(ID3D12GraphicsCommandList* cmd,
								DxGpuResource* input,
								IRenderTarget* finalTarget,
								CalyxEngine::DxCore* dxCore){
	if (!initialized_) return;
	RebuildGraphIfDirty();
	graph_.Execute(cmd,input,finalTarget,dxCore);
}

// ---------- TweenFloat ----------
void PostEffectManager::TweenFloat(const std::string& passName,
								   std::function<float()> getter,
								   std::function<void(float)> setter,
								   std::optional<float> from,
								   float to,
								   float durationSec,
								   CalyxEngine::EaseType ease,
								   bool autoDisableIfZero,
								   std::function<void()> onComplete){
	FloatTween tw;
	tw.passName = passName;
	tw.getter = std::move(getter);
	tw.setter = std::move(setter);
	tw.start = from.has_value() ? *from : (tw.getter ? tw.getter() : 0.f);
	tw.end = to;
	tw.t = 0.f;
	tw.dur = (std::max)(0.0001f,durationSec);
	tw.ease = ease;
	tw.autoDisableIfZero = autoDisableIfZero;
	tw.onComplete = std::move(onComplete);

	// 直ちに有効化
	Enable(passName,true);
	// 初期値を即時反映（
	if (tw.setter) tw.setter(tw.start);

	floatTweens_.push_back(std::move(tw));
}

// ---------- GetPass ----------
IPostEffectPass* PostEffectManager::GetPass(const std::string& name){
	const int idx = IndexOf(name);
	if (idx < 0) return nullptr;
	return collection_.GetSlots()[idx].pass;
}

// ---------- UI ----------
void PostEffectManager::DrawImGui(){
	if (!initialized_) return;
	auto& slots = collection_.GetSlots();

	static char presetPath[256] = "PostEffects/Default.postfx";
	ImGui::SetNextItemWidth(360.0f);
	ImGui::InputText("Preset Path", presetPath, sizeof(presetPath));
	if(ImGui::Button("Save Preset")){ SavePreset(presetPath, "PostEffectPreset"); }
	ImGui::SameLine();
	if(ImGui::Button("Load Preset")){ LoadPreset(presetPath); }
	ImGui::SameLine();
	if(ImGui::Button("Play Triggered")){ PlayTriggeredEffects(); }
	ImGui::Separator();

	ImGui::Checkbox("Outline", &outlineEnabled_);
	ImGui::Separator();

	if (ImGui::Button("Enable All")){ EnableAll(); }
	ImGui::SameLine();
	if (ImGui::Button("Disable All")){ DisableAll(); }
	ImGui::Separator();

	for (int i = 0; i < (int)slots.size(); ++i){
		auto& s = slots[i];
		const bool isCopy = (s.name == kCopyImageName);

		ImGui::PushID(i);
		if (isCopy) ImGui::BeginDisabled();

		bool on = s.enabled;
		if (GuiCmd::CheckBox("##on",on)){
			s.enabled = on;
			MarkDirty();
		}
		ImGui::SameLine();
		ImGui::TextUnformatted(s.name.c_str());
		ImGui::SameLine(260);
		if (ImGui::SmallButton("Up")){ MoveUp(s.name); }
		ImGui::SameLine();
		if (ImGui::SmallButton("Down")){ MoveDown(s.name); }
		ImGui::SameLine();
		if (ImGui::SmallButton("Reset")){ if (s.pass) s.pass->ResetParameters(); }
		ImGui::SameLine();
		if (ImGui::SmallButton("Play")){ PlayTriggeredEffect(s.name); }
		ImGui::SameLine();
		if (ImGui::BeginPopup("pp_param")){
			if (s.pass) s.pass->ShowImGui();
			ImGui::EndPopup();
		}

		if(ImGui::TreeNode("Runtime")){
			int mode = s.applyMode == PostEffectApplyMode::Triggered ? 1 : 0;
			if(ImGui::Combo("Apply Mode", &mode, "Always\0Triggered\0")){
				s.applyMode = mode == 1 ? PostEffectApplyMode::Triggered : PostEffectApplyMode::Always;
				if(s.applyMode == PostEffectApplyMode::Triggered){
					s.enabled = false;
				}
				MarkDirty();
			}
			ImGui::DragFloat("Duration", &s.duration, 0.01f, 0.0f, 60.0f, "%.2f sec");
			int ease = static_cast<int>(s.ease);
			if(CalyxEngine::SelectEaseInt("Ease", ease)){
				s.ease = static_cast<CalyxEngine::EaseType>(ease);
			}
			ImGui::Checkbox("Auto Disable", &s.autoDisable);

			if(s.pass && ImGui::TreeNode("Float Animations")){
				const char* candidates[] = {"width", "intensity", "threshold", "softKnee", "strength", "radius", "center.x", "center.y", "tint.r", "tint.g", "tint.b"};
				if(ImGui::BeginCombo("Add Parameter", "select")){
					for(const char* param : candidates){
						float tmp = 0.0f;
						if(!s.pass->GetFloatParameter(param, tmp)) continue;
						if(ImGui::Selectable(param)){
							s.floatAnimations.push_back({param, tmp, 0.0f, false});
						}
					}
					ImGui::EndCombo();
				}
				for(int animIndex = 0; animIndex < static_cast<int>(s.floatAnimations.size()); ++animIndex){
					auto& anim = s.floatAnimations[animIndex];
					ImGui::PushID(animIndex);
					ImGui::TextUnformatted(anim.parameter.c_str());
					ImGui::SameLine();
					if(ImGui::SmallButton("Remove")){
						s.floatAnimations.erase(s.floatAnimations.begin() + animIndex);
						ImGui::PopID();
						--animIndex;
						continue;
					}
					ImGui::Checkbox("Use Current As From", &anim.useCurrentAsFrom);
					if(!anim.useCurrentAsFrom){
						ImGui::DragFloat("From", &anim.from, 0.01f);
					}
					ImGui::DragFloat("To", &anim.to, 0.01f);
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		if (isCopy) ImGui::EndDisabled();
		ImGui::PopID();
		ImGui::Separator();
	}
}
