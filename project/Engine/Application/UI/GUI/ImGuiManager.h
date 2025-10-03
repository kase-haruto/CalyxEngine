#pragma once



#include<d3d12.h>
#include <wrl.h>

class WinApp;
class DxCore;

class ImGuiManager{
public:

	ImGuiManager() = default;
	~ImGuiManager() = default;

	void Initialize(WinApp* winApp, const DxCore* dxCore);
	void Finalize();
	void Begin();
	void End();
	void Draw();

private:
	void CustomizeImGuiStyle();
private:
	const DxCore* pDxCore_ = nullptr;
};

