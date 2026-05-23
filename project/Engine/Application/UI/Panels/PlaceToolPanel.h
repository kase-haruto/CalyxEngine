#pragma once
/* ========================================================================
/*      include space
/* ===================================================================== */
// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>

// c++
#include "imgui/imgui.h"

#include <d3d12.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class SceneContext;
class SceneObject;

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * PlaceToolPanel
	 * - オブジェクト配置ツールパネルクラス
	 * - Shape・Light・Particle等のオブジェクトをシーンに配置する機能を提供
	 *---------------------------------------------------------------------------------------*/
	class PlaceToolPanel
		: public IEngineUI {
	public:
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
			Shape,			 //< 単純図形オブジェクト
			Light,			 //< ライト
			Particle,		 //< パーティクル
			Event,			 //< イベント
			Count
		};

	public:
		struct PlaceItem {
			PlaceItemCategory							   category;
			std::string									   name;
			D3D12_GPU_DESCRIPTOR_HANDLE					   texture;
			CalyxEngine::Vector2							   iconSize{64.0f, 64.0f};
			std::function<void(const CalyxEngine::Vector3&)> createFunc;
			std::function<std::shared_ptr<SceneObject>()>  ghostFactory;
		};

	public:
		PlaceToolPanel();
		~PlaceToolPanel() override = default;

		void Render() override;

		const std::string& GetPanelName() const override { return panelName_; }

	private:
		void RegisterPlaceItems();
		void RenderSidebar();
		void RenderContent();

		std::unordered_map<PlaceItemCategory, std::vector<PlaceItem>> categoryItems_;
		std::string													  panelName_ = "Place Actors";

		// Selection & Filter
		PlaceItemCategory selectedCategory_ = PlaceItemCategory::Shape;
		ImGuiTextFilter	  filter_;
	};
} // namespace CalyxEngine