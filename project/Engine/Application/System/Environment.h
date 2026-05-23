#pragma once
#include <stdint.h>
#include <string>

#include <Engine/Foundation/Math/Vector2.h>

static const std::string windowTitle = "Engine";

static const uint32_t kGameWidth = 1280;
static const uint32_t kGameHeight = 720;
static const CalyxEngine::Vector2 kGameSize = CalyxEngine::Vector2(
	static_cast< float >(kGameWidth), static_cast< float >(kGameHeight));

//フルhdとhdの中間1280x720と1920x1080,1600x900
static const uint32_t kWindowWidth	= 1280;
static const uint32_t kWindowHeight = 720;

static const CalyxEngine::Vector2 kWindowSize = CalyxEngine::Vector2(
	static_cast<float>(kWindowWidth), static_cast< float >(kWindowHeight));

//imGUiの実行ウィンドウサイズ736x414:16:9
static const uint32_t kGuiWindowWidth = 1024;
static const uint32_t kGuiWindowHeight = 576;

static const CalyxEngine::Vector2 kExecuteWindowSize = CalyxEngine::Vector2(
	static_cast< float >(kGuiWindowWidth), static_cast< float >(kGuiWindowHeight));