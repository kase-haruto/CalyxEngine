#pragma once
/* ========================================================================
/*      include space
/* ===================================================================== */
// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Foundation/Math/Vector2.h>

// c++
#include <d3d12.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// fwd
class SceneContext;
class SceneObject;

/// <summary>
/// オブジェクト配置エディタ
/// </summary>
class PlaceToolPanel
	: public IEngineUI {
public:
	//===================================================================*/
	//					enums
	//===================================================================*/
	enum class ShapeObjType {
		Plane,
		Cube,
		Sphere,
		Cylinder,
		Cone,
		Torus,
		Count
	};

	enum class PlaceItemCategory {
		Shape,		  //< 単純図形オブジェクト
		Light,		  //< ライト
		Particle,	  //< パーティクル
		InGameObject, //< インゲームのオブジェクト
		Model,		  //< モデル
		Event,		  //< イベント
		Count
	};

private:
	//===================================================================*/
	//					private structs
	//===================================================================*/
	struct PlaceItem {
		PlaceItemCategory			category;
		std::string					name;
		D3D12_GPU_DESCRIPTOR_HANDLE texture;
		Vector2						iconSize{64.0f, 64.0f};
		std::function<void()>		createFunc;
	};

public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	PlaceToolPanel();
	~PlaceToolPanel() override = default;

	// editor描画
	void Render() override;

	// getter
	const std::string& GetPanelName() const override { return panelName_; }

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	void RegisterPlaceItems();
	void RenderCategoryItems();

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	std::unordered_map<PlaceItemCategory, std::vector<PlaceItem>> categoryItems_;
	std::string panelName_ = "PlaceToolPanel";
};