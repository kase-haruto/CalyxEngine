#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>

namespace Calyx {

	struct ProjectInfo;
} // namespace Calyx

namespace CalyxEngine {
	class EngineUICore;
	class SceneManager;
}

namespace Calyx {




	/*-----------------------------------------------------------------------------------------
	 * Application
	 * - ゲームアプリケーションからエンジンへ処理を接続する基底クラス
	 * - プロジェクト読込、初期化、更新、描画、終了のライフサイクル通知を提供
	 * - Engine、SceneManager、Editor UIの所有権は管理しない
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief Applicationの機能を提供するクラスです。
	 */
	class CALYX_API Application {
	public:
		/**
		 * \brief アプリケーション基底クラスを破棄する
		 */
		virtual ~Application() = default;

		/**
		 * \brief プロジェクト情報の読込完了をアプリケーションへ通知する
		 * \param project 読み込まれたプロジェクト情報
		 */
		virtual void OnProjectLoaded(const ProjectInfo& project) { (void)project; }

		/**
		 * \brief シーン管理機能の利用準備完了をアプリケーションへ通知する
		 * \param sceneManager エンジンが所有するシーン管理機能
		 */
		virtual void OnSceneManagerReady(CalyxEngine::SceneManager& sceneManager) { (void)sceneManager; }

		/**
		 * \brief Editor UIの利用準備完了をアプリケーションへ通知する
		 * \param engineUi エンジンが所有するEditor UI
		 */
		virtual void OnEngineUiReady(CalyxEngine::EngineUICore& engineUi) { (void)engineUi; }

		/** \brief ゲーム固有機能を初期化する */
		virtual void OnInitialize() {}

		/** \brief ゲーム固有のフレーム更新を実行する */
		virtual void OnUpdate() {}

		/** \brief ゲーム固有の描画命令を登録する */
		virtual void OnRender() {}

		/** \brief ゲーム固有リソースを終了順序に従って解放する */
		virtual void OnFinalize() {}

		/**
		 * \brief Engine UIを描画するか判定する
		 * \return Engine UIを描画する場合はtrue
		 */
		virtual bool ShouldRenderEngineUi() const { return true; }

		// Game applications normally discover a nearby project when launched
		// without arguments. Editors can opt out to show their project browser.
		virtual bool ShouldAutoDiscoverProject() const { return true; }
	};

} // namespace Calyx
