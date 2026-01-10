#include "PostEffectManager.h"

// engine
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>

// externals
#include <externals/imgui/imgui.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// c++
#include "Engine/Foundation/Utility/Ease/CxEase.h"

#include <unordered_set>

PostEffectManager* PostEffectManager::Get(){
	static PostEffectManager instance;
	return &instance;
}

void PostEffectManager::Initialize(PipelineService* service,bool enableAll){
	assert(service);
	collection_.Initialize(service);
	collection_.BuildInitialSlots(enableAll);

	// CopyImage は最終コピー専用。常にOFF固定にしておく
	for (auto& s : collection_.GetSlots()){ if (s.name == kCopyImageName) s.enabled = false; }

	dirty_ = true;
	initialized_ = true;
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

// ---------- Update / Execute ----------
void PostEffectManager::RebuildGraphIfDirty(){
	if (!dirty_) return;
	graph_.SetPassesFromList(collection_.GetSlots()); // enabled だけ反映
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
		const float k = CalyxEase::ApplyEase(tw.ease, r);
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
								CalyxGraphics::DxCore* dxCore){
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
								   CalyxEase::EaseType ease,
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
		if (ImGui::BeginPopup("pp_param")){
			if (s.pass) s.pass->ShowImGui();
			ImGui::EndPopup();
		}

		if (isCopy) ImGui::EndDisabled();
		ImGui::PopID();
		ImGui::Separator();
	}
}