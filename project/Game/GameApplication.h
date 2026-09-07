#pragma once

#include <CalyxEngine/Application.h>
#include <Engine/Assets/Font/FontTypes.h>

/**
 * @brief GameApplicationの機能を提供するクラスです。
 */
class GameApplication : public Calyx::Application {
public:
	void OnInitialize() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnFinalize() override;

private:
#if defined(_DEBUG) || defined(DEVELOP)
	CalyxEngine::FontHandle textValidationFont_{};
#endif
};
