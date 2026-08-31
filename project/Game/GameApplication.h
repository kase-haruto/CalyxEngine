#pragma once

#include <CalyxEngine/Application.h>

/**
 * @brief GameApplicationの機能を提供するクラスです。
 */
class GameApplication : public Calyx::Application {
public:
	void OnInitialize() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnFinalize() override;
};
