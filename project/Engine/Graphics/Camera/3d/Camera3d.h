#pragma once

#include <Engine/Graphics/Camera/Base/BaseCamera.h>


class Camera3d
	: public BaseCamera{
public:
	//==================================================================*//
	//			public functions
	//==================================================================*//
	Camera3d();
	~Camera3d() = default;

	// 視錐台を描画（デバッグ用）
	void DrawFrustum();

	void Update() override;  //< 更新
	void ShowGui() override; //< ImGuiによるGUI表示

};
