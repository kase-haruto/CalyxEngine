#pragma once

#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Graphics/Camera/Frustum/Frustum.h>


class Camera3d
	: public BaseCamera{
public:
	//==================================================================*//
	//			public functions
	//==================================================================*//
	Camera3d();
	~Camera3d() = default;

	void Update() override;  //< 更新
	void ShowGui() override; //< ImGuiによるGUI表示
	bool IsVisible(const class AABB& aabb) const;
private:
	//==================================================================*//
	//			private functions
	//==================================================================*//
	Frustum frustum_;	//視推台
};
