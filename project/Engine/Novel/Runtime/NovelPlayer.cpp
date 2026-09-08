#include "NovelPlayer.h"

#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Objects/2D/Object2d/SpriteObject2d.h>
#include <Engine/Objects/2D/Object2d/TextSceneObject2d.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <algorithm>
#include <filesystem>

namespace CalyxEngine {
	void NovelPlayer::SetScene(const NovelSceneAsset* scene) noexcept { if(scene_!=scene){Stop();scene_=scene;} }
	void NovelPlayer::Play(){if(!scene_)return;ClearPresentation();currentEventIndex_=0;eventStarted_=false;finished_=false;playing_=true;}
	void NovelPlayer::Pause()noexcept{playing_=false;if(dialogueText_)dialogueText_->Pause();}
	void NovelPlayer::Stop(){playing_=false;finished_=false;currentEventIndex_=0;eventStarted_=false;ClearPresentation();}
	std::string NovelPlayer::ResolveAssetPath(const Guid& guid,AssetType type)const{const auto* db=AssetDatabase::GetInstance();const auto* r=guid.isValid()?db->Get(guid):nullptr;if(!r||r->type!=type)return{};std::error_code e;auto p=std::filesystem::relative(r->sourcePath,db->GetRoot(),e);return e?r->sourcePath.generic_string():p.generic_string();}
	void NovelPlayer::ClearPresentation(){if(dialogueText_)SceneAPI::RemoveObject(dialogueText_);if(speakerText_)SceneAPI::RemoveObject(speakerText_);if(background_)SceneAPI::RemoveObject(background_);for(auto&[_,v]:images_)if(v)SceneAPI::RemoveObject(v);dialogueText_.reset();speakerText_.reset();background_.reset();images_.clear();}
	void NovelPlayer::Advance(){eventStarted_=false;eventElapsedTime_=0;autoAdvanceElapsed_=0;if(++currentEventIndex_>=scene_->GetEvents().size()){playing_=false;finished_=true;}}
	void NovelPlayer::StartCurrentEvent(){if(!scene_||currentEventIndex_>=scene_->GetEvents().size()){playing_=false;finished_=true;return;}eventStarted_=true;const auto& e=scene_->GetEvents()[currentEventIndex_];
		switch(e.type_){
		case NovelEventType::Dialogue:{const auto& d=std::get<NovelDialogueEvent>(e.data_);const auto* r=AssetDatabase::GetInstance()->Get(d.dialogueAssetGuid_);if(!r||r->type!=AssetType::Dialogue||!dialogueAsset_.Load(r->sourcePath)){Advance();return;}const auto* line=dialogueAsset_.FindLine(d.dialogueLineGuid_);if(!line){Advance();return;}if(!dialogueText_){dialogueText_=SceneAPI::Instantiate<TextSceneObject2d>();dialogueText_->GetWorldTransform().translation={80,760,0};dialogueText_->GetWorldTransform().scale={1760,220,1};speakerText_=SceneAPI::Instantiate<TextSceneObject2d>();speakerText_->GetWorldTransform().translation={80,700,0};speakerText_->GetWorldTransform().scale={600,60,1};}speakerText_->SetTypewriter(false);speakerText_->SetText(line->speaker_);dialogueText_->SetTypewriter(true);dialogueText_->SetCharactersPerSecond(line->charactersPerSecond_);dialogueText_->SetText(line->text_);dialogueText_->Restart();break;}
		case NovelEventType::ShowImage:{const auto& d=std::get<NovelShowImageEvent>(e.data_);auto p=ResolveAssetPath(d.textureGuid_,AssetType::Texture);if(!p.empty()){auto s=SceneAPI::Instantiate<SpriteObject2d>();s->Initialize(p);s->SetPosition(d.position_);s->SetScale(d.scale_);images_[e.guid_]=s;}Advance();break;}
		case NovelEventType::HideImage:{auto target=std::get<NovelHideImageEvent>(e.data_).targetGuid_;auto it=images_.find(target);if(it!=images_.end()){SceneAPI::RemoveObject(it->second);images_.erase(it);}Advance();break;}
		case NovelEventType::ChangeBackground:{auto p=ResolveAssetPath(std::get<NovelBackgroundEvent>(e.data_).textureGuid_,AssetType::Texture);if(background_)SceneAPI::RemoveObject(background_);background_.reset();if(!p.empty()){background_=SceneAPI::Instantiate<SpriteObject2d>();background_->Initialize(p);background_->SetPosition({960,540});}Advance();break;}
		case NovelEventType::Wait:break;}}
	void NovelPlayer::Update(float dt){if(!playing_||finished_)return;int guard=0;while(playing_&&!eventStarted_&&guard++<64)StartCurrentEvent();if(!playing_||!eventStarted_)return;dt=(std::max)(0.0f,dt);const auto&e=scene_->GetEvents()[currentEventIndex_];if(e.type_==NovelEventType::Wait){eventElapsedTime_+=dt;if(eventElapsedTime_>=std::get<NovelWaitEvent>(e.data_).duration_)Advance();}else if(e.type_==NovelEventType::Dialogue&&dialogueText_&&dialogueText_->IsCompleted()){const auto& d=std::get<NovelDialogueEvent>(e.data_);const auto* line=dialogueAsset_.FindLine(d.dialogueLineGuid_);if(line&&line->autoAdvance_){autoAdvanceElapsed_+=dt;if(autoAdvanceElapsed_>=line->autoAdvanceDelay_)Advance();}}}
	void NovelPlayer::Next(){if(!playing_||!eventStarted_)return;const auto&e=scene_->GetEvents()[currentEventIndex_];if(e.type_==NovelEventType::Dialogue&&dialogueText_&&!dialogueText_->IsCompleted()){dialogueText_->Complete();autoAdvanceElapsed_=0;return;}if(e.type_==NovelEventType::Dialogue||e.type_==NovelEventType::Wait)Advance();}
}
