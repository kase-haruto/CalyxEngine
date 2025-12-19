#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Camera/Viewport/ViewportDetail.h>
#include <Engine/Foundation/Math/Vector2.h>

// engine
#include <Engine/Application/UI/EngineUI/IOnViewportTool.h>
#include <Engine/Application/UI/EngineUI/IEngineUI.h>

// c++
#include <string>

// externals
#include <externals/imgui/imgui.h>

// forward declaration
struct CxMath::Vector2;
class BaseCamera;


class Viewport:
public IEngineUI{
public:
	//===================================================================*/
	//                    methods
	//===================================================================*/
	Viewport(ViewportType type, const std::string& windowName);

	void Update();								//< ビューポートの更新処理
	void Render(const ImTextureID& tex);		//< ImGui上への描画処理
	void Render() {}

	void AddTool(IOnViewportTool* tool);		//< ビューポートツールの追加

	//--------- accessor -----------------------------------------------------
	bool IsHovered() const;
	bool IsClicked() const;
	CxMath::Vector2 GetSize() const;
	CxMath::Vector2 GetPosition() const;				//< ビューポートの位置
	ViewportType GetType() const;
	void SetCamera(BaseCamera* camera);

private:
	ImVec2 CalcToolPosition(IOnViewportTool* tool,
							const ImVec2& viewportPos,
							const ImVec2& viewportSize);
private:
	std::vector<IOnViewportTool*> tools_;
	ViewportType type_ = ViewportType::VIEWPORT_NONE;	//< ビューポートの種類
	std::string windowName_;							//< ビューポートのウィンドウ名

	ImTextureID textureID_ = nullptr;

	BaseCamera* camera_ = nullptr;						//< ビューポートに関連付けられたカメラ
	CxMath::Vector2 size_ {};
	CxMath::Vector2 viewOrigin_;								//< ImGui上での描画開始位置
	bool isHovered_ = false;
	bool isClicked_ = false;
};

