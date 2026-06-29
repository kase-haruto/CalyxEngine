#pragma once

#include <CalyxEngine/Application.h>

class GameApplication : public Calyx::Application {
public:
	void OnInitialize() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnFinalize() override;
};
