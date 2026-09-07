#include "GameApplication.h"
#include <Engine/Renderer/Text/TextService.h>

void GameApplication::OnInitialize() {
#if defined(_DEBUG) || defined(DEVELOP)
	textValidationFont_ = CalyxEngine::TextService::GetInstance()->LoadFont("fonts/NotoSerifJP.ttf");
#endif
}

void GameApplication::OnUpdate() {}

void GameApplication::OnRender() {
#if defined(_DEBUG) || defined(DEVELOP)
	if(!textValidationFont_) return;
	CalyxEngine::TextStyle style{};
	style.fontSize_ = 30.0f;
	style.color_ = {1.0f, 1.0f, 1.0f, 1.0f};
	auto* text = CalyxEngine::TextService::GetInstance();
	text->Draw(textValidationFont_, u8"Hello NexusEngine", {40.0f, 60.0f}, style);
	text->Draw(textValidationFont_, u8"こんにちは NexusEngine", {40.0f, 105.0f}, style);
	text->Draw(textValidationFont_, u8"これは\n複数行の\nテキストです。", {40.0f, 150.0f}, style);
#endif
}

void GameApplication::OnFinalize() {}
