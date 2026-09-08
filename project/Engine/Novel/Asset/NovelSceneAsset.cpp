#include "NovelSceneAsset.h"

#include <externals/nlohmann/json.hpp>
#include <algorithm>
#include <fstream>

namespace CalyxEngine {
	namespace {
		const char* TypeName(NovelEventType type) {
			switch(type) { case NovelEventType::Dialogue:return "Dialogue"; case NovelEventType::ShowImage:return "ShowImage"; case NovelEventType::HideImage:return "HideImage"; case NovelEventType::ChangeBackground:return "ChangeBackground"; case NovelEventType::Wait:return "Wait"; }
			return "Dialogue";
		}
		NovelEventType ParseType(const std::string& value) {
			if(value=="ShowImage") return NovelEventType::ShowImage; if(value=="HideImage") return NovelEventType::HideImage; if(value=="ChangeBackground") return NovelEventType::ChangeBackground; if(value=="Wait") return NovelEventType::Wait; return NovelEventType::Dialogue;
		}
		nlohmann::json Serialize(const NovelEvent& event) {
			nlohmann::json data;
			std::visit([&](const auto& value) {
				using T=std::decay_t<decltype(value)>;
				if constexpr(std::is_same_v<T,NovelDialogueEvent>) data={{"dialogueAssetGuid",value.dialogueAssetGuid_},{"dialogueLineGuid",value.dialogueLineGuid_}};
				else if constexpr(std::is_same_v<T,NovelShowImageEvent>) data={{"textureGuid",value.textureGuid_},{"position",value.position_},{"scale",value.scale_}};
				else if constexpr(std::is_same_v<T,NovelHideImageEvent>) data={{"targetGuid",value.targetGuid_}};
				else if constexpr(std::is_same_v<T,NovelBackgroundEvent>) data={{"textureGuid",value.textureGuid_}};
				else data={{"duration",value.duration_}};
			},event.data_);
			return {{"guid",event.guid_},{"type",TypeName(event.type_)},{"data",std::move(data)}};
		}
		NovelEvent Deserialize(const nlohmann::json& json) {
			NovelEvent event; event.guid_=json.value("guid",Guid::New()); if(!event.guid_.isValid()) event.guid_=Guid::New(); event.type_=ParseType(json.value("type",std::string{"Dialogue"})); const auto& d=json.value("data",nlohmann::json::object());
			switch(event.type_) {
			case NovelEventType::Dialogue:event.data_=NovelDialogueEvent{d.value("dialogueAssetGuid",Guid{}),d.value("dialogueLineGuid",Guid{})};break;
			case NovelEventType::ShowImage:event.data_=NovelShowImageEvent{d.value("textureGuid",Guid{}),d.value("position",Vector2{}),d.value("scale",Vector2{1,1})};break;
			case NovelEventType::HideImage:event.data_=NovelHideImageEvent{d.value("targetGuid",Guid{})};break;
			case NovelEventType::ChangeBackground:event.data_=NovelBackgroundEvent{d.value("textureGuid",Guid{})};break;
			case NovelEventType::Wait:event.data_=NovelWaitEvent{(std::max)(0.0f,d.value("duration",1.0f))};break;
			} return event;
		}
	}
	NovelEvent* NovelSceneAsset::FindEvent(const Guid& guid) noexcept { auto it=std::ranges::find(events_,guid,&NovelEvent::guid_); return it==events_.end()?nullptr:&*it; }
	const NovelEvent* NovelSceneAsset::FindEvent(const Guid& guid) const noexcept { auto it=std::ranges::find(events_,guid,&NovelEvent::guid_); return it==events_.end()?nullptr:&*it; }
	NovelEvent& NovelSceneAsset::AddEvent(NovelEventType type) { return events_.emplace_back(NovelEvent{Guid::New(),type,MakeNovelEventData(type)}); }
	bool NovelSceneAsset::RemoveEvent(const Guid& guid) { auto it=std::ranges::find(events_,guid,&NovelEvent::guid_);if(it==events_.end())return false;events_.erase(it);return true; }
	bool NovelSceneAsset::MoveEvent(const Guid& guid,int offset){auto it=std::ranges::find(events_,guid,&NovelEvent::guid_);if(it==events_.end())return false;auto i=std::distance(events_.begin(),it);auto t=i+offset;if(t<0||t>=static_cast<std::ptrdiff_t>(events_.size()))return false;std::iter_swap(events_.begin()+i,events_.begin()+t);return true;}
	bool NovelSceneAsset::Load(const std::filesystem::path& path){try{std::ifstream s(path);if(!s)return false;nlohmann::json j;s>>j;std::vector<NovelEvent> loaded;for(const auto& v:j.value("events",nlohmann::json::array()))loaded.push_back(Deserialize(v));events_=std::move(loaded);return true;}catch(...){return false;}}
	bool NovelSceneAsset::Save(const std::filesystem::path& path)const{try{if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());nlohmann::json a=nlohmann::json::array();for(const auto& e:events_)a.push_back(Serialize(e));nlohmann::json j={{"version",1},{"events",std::move(a)}};std::ofstream s(path);if(!s)return false;s<<j.dump(2);return bool(s);}catch(...){return false;}}
}
