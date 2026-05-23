#pragma once

#include <string>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * IEngineUI
	 * - UIパネルインターフェース
	 * - すべてのエディタパネルが実装する描画・表示切替の基盤
	 *---------------------------------------------------------------------------------------*/
	class IEngineUI {
	public:
		//===================================================================*/
		//						public methods
		//===================================================================*/
		IEngineUI(const std::string& name);
		IEngineUI();
		virtual ~IEngineUI() = default;

		virtual void Render() = 0;

		//--------- accessor -----------------------------------------------------//
		virtual const std::string& GetPanelName() const { return panelName_; }
		void					   SetShow(bool isShow) { isShow_ = isShow; }
		bool					   IsShow() const { return isShow_; }

	protected:
		//===================================================================*/
		//						protected methods
		//===================================================================*/
		std::string panelName_;
		bool		isShow_ = true;
	};
} // namespace CalyxEngine