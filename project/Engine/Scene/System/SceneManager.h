#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Scene/Base/BaseScene.h>
#include <Engine/Scene/Fade/BaseSceneTransitionEffect.h>
#include <Engine/Scene/Transitioner/IScenePayload.h>
#include <Engine/Scene/Transitioner/SceneTransitionRequestor.h>

#include <d3d12.h>
#include <filesystem>
#include <memory>
#include <optional>

class SceneContext;
class PipelineService;
class ModelRenderer;
class IRenderTarget;

namespace CalyxEngine {
	class PlaySession;
	class PickingPass;
	class GridRenderer;

	/*-----------------------------------------------------------------------------------------
	 * SceneManager
	 * - シーンの読み込み、更新、描画、切り替えを管理するクラス
	 * - ランタイム用シーンとエディタプレビュー用コンテキストを管理する
	 * - シーン遷移要求を受け取り、適切なタイミングでシーンを切り替える
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief SceneManagerの機能を提供するクラスです。
	 */
	class CALYX_API SceneManager {
	public:
		/**
		 * \brief SceneManagerを生成する
		 * \param dx DirectX 12のデバイスや描画環境を管理するDxCore
		 */
		explicit SceneManager(DxCore* dx);

		/**
		 * \brief SceneManagerを破棄する
		 */
		~SceneManager();

		/**
		 * \brief シーン管理に必要なリソースや描画機能を初期化する
		 */
		void Initialize();

		/**
		 * \brief 指定されたシーンファイルを開き、現在のシーンとして設定する
		 * \param scenePath 読み込むシーンファイルのパス
		 * \return シーンの読み込みに成功した場合はtrue
		 */
		bool OpenScene(const std::filesystem::path& scenePath);

		/**
		 * \brief アセットGUIDからシーンを検索し、現在のシーンとして設定する
		 * \param sceneAssetGuid 読み込むシーンアセットのGUID
		 * \return シーンの読み込みに成功した場合はtrue
		 */
		bool OpenScene(const Guid& sceneAssetGuid);

		/**
		 * \brief 指定されたシーンファイルへの遷移を要求する
		 * \param scenePath 遷移先となるシーンファイルのパス
		 */
		void RequestSceneChange(const std::filesystem::path& scenePath);

		/**
		 * \brief アセットGUIDで指定されたシーンへの遷移を要求する
		 * \param sceneAssetGuid 遷移先となるシーンアセットのGUID
		 */
		void RequestSceneChange(const Guid& sceneAssetGuid);
		void RequestSceneChange(const std::filesystem::path& scenePath, std::unique_ptr<BaseSceneTransitionEffect> effect);
		void RequestSceneChange(const Guid& sceneAssetGuid, std::unique_ptr<BaseSceneTransitionEffect> effect);

		/**
		 * \brief 現在のシーンを更新し、保留中のシーン遷移を処理する
		 * \param dt ゲーム進行に使用するデルタタイム
		 * \param alwaysDt ポーズ状態などに影響されないデルタタイム
		 */
		void Update(float dt, float alwaysDt);

		/**
		 * \brief シーン更新後に必要となる描画前処理を実行する
		 * \param cmd 描画命令を記録するコマンドリスト
		 * \param pso 描画パイプラインを管理するサービス
		 */
		void PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso);

		/**
		 * \brief 現在のシーンを描画する
		 * \param cmd 描画命令を記録するコマンドリスト
		 * \param pso 描画パイプラインを管理するサービス
		 */
		void Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso);

		/**
		 * \brief 指定されたレンダーターゲットへ現在のシーンを描画する
		 * \param rt 描画先となるレンダーターゲット
		 * \param cmd 描画命令を記録するコマンドリスト
		 * \param pso 描画パイプラインを管理するサービス
		 */
		void DrawForRenderTarget(
			IRenderTarget*			   rt,
			ID3D12GraphicsCommandList* cmd,
			PipelineService*		   pso);

		/**
		 * \brief ポストエフェクトの影響を受けないオブジェクトを描画する
		 * \param cmd 描画命令を記録するコマンドリスト
		 * \param pso 描画パイプラインを管理するサービス
		 */
		void DrawNotAffectedFromPE(
			ID3D12GraphicsCommandList* cmd,
			PipelineService*		   pso);

		/**
		 * \brief 再生セッションをSceneManagerへ関連付ける
		 * \param ps 関連付ける再生セッション
		 */
		void BindPlaySession(PlaySession* ps) {
			pPlaySession_ = ps;
		}

		/**
		 * \brief 現在有効なSceneContextを取得する
		 * \return 現在有効なSceneContext。存在しない場合はnullptr
		 */
		SceneContext* ActiveCtx() const;

		/**
		 * \brief 現在のシーンがランタイムとして動作しているか確認する
		 * \return ランタイムとして動作している場合はtrue
		 */
		bool ActiveRuntimeFlag() const;

		/**
		 * \brief 現在のゲームが終了状態であるか確認する
		 * \return ゲームが終了状態の場合はtrue
		 */
		bool GetIsEndGame() const;

		/**
		 * \brief SceneContextの変更を検出し、必要な参照を再設定する
		 */
		void RebindIfContextChanged();

		/**
		 * \brief 現在有効なシーンが存在するか確認する
		 * \return シーンが存在する場合はtrue
		 */
		bool HasScene() const {
			return activeScene_.scene != nullptr;
		}

		/**
		 * \brief エディタプレビューで使用するSceneContextを設定する
		 * \param ctx エディタプレビュー対象のSceneContext
		 */
		void SetEditorPreviewContext(SceneContext* ctx);

		/**
		 * \brief エディタビューポートの追加描画設定を変更する
		 * \param renderDebugView デバッグ表示を描画する場合はtrue
		 * \param renderPicking ピッキング用描画を実行する場合はtrue
		 */
		void SetEditorViewportRenderState(
			bool renderDebugView,
			bool renderPicking);

		/**
		 * \brief 管理しているすべてのSceneContextをクリアする
		 */
		void ClearAllContexts();

		/**
		 * \brief 現在管理しているシーンのSceneContextを取得する
		 * \return 現在のSceneContext。存在しない場合はnullptr
		 */
		SceneContext* GetCurrentSceneContext() const;

		/**
		 * \brief 現在開いているシーンファイルのパスを取得する
		 * \return 現在のシーンファイルのパス
		 */
		std::filesystem::path GetCurrentScenePath() const;

		/**
		 * \brief シーン遷移要求を送信するためのインターフェースを取得する
		 * \return シーン遷移要求インターフェースへの参照
		 */
		ISceneTransitionRequestor& GetTransitionRequestor();

		/**
		 * \brief シーンのピッキング処理を取得する
		 * \return PickingPassへのポインタ
		 */
		PickingPass* GetPickingPass() const {
			return pickingPass_.get();
		}

	private:
		/**
		 * \brief ペイロードを伴うシーンファイルへの遷移要求を登録する
		 * \param scenePath 遷移先となるシーンファイルのパス
		 * \param payload 遷移先シーンへ受け渡すデータ
		 */
		void RequestSceneChangeInternal(
			const std::filesystem::path&   scenePath,
			std::unique_ptr<IScenePayload> payload = nullptr);
		void RequestSceneChangeInternal(
			const std::filesystem::path& scenePath,
			std::unique_ptr<IScenePayload> payload,
			std::unique_ptr<BaseSceneTransitionEffect> effect);

		/**
		 * \brief ペイロードを伴うGUID指定のシーン遷移要求を登録する
		 * \param sceneAssetGuid 遷移先となるシーンアセットのGUID
		 * \param payload 遷移先シーンへ受け渡すデータ
		 */
		void RequestSceneChangeInternal(
			const Guid&					   sceneAssetGuid,
			std::unique_ptr<IScenePayload> payload = nullptr);
		void RequestSceneChangeInternal(
			const Guid& sceneAssetGuid,
			std::unique_ptr<IScenePayload> payload,
			std::unique_ptr<BaseSceneTransitionEffect> effect);

		/**
		 * \brief エディタプレビュー対象のシーンをレンダーターゲットへ描画する
		 * \param rt 描画先となるレンダーターゲット
		 * \param cmd 描画命令を記録するコマンドリスト
		 * \param pso 描画パイプラインを管理するサービス
		 */
		void DrawEditorPreview(
			IRenderTarget*			   rt,
			ID3D12GraphicsCommandList* cmd,
			PipelineService*		   pso);

		/**
		 * \brief デバッグ用プリミティブを指定されたレンダーターゲットへ描画する
		 * \param rt 描画先となるレンダーターゲット
		 * \param cmd 描画命令を記録するコマンドリスト
		 * \param includeDebugViewOnly DebugView専用オブジェクトを含める場合はtrue
		 */
		void RenderDebugPrimitivesToRenderTarget(
			IRenderTarget*			   rt,
			ID3D12GraphicsCommandList* cmd,
			bool					   includeDebugViewOnly);

		/**
		 * \brief ビューポートの座標軸を指定されたレンダーターゲットへ描画する
		 * \param rt 描画先となるレンダーターゲット
		 * \param cmd 描画命令を記録するコマンドリスト
		 */
		void RenderViewportAxisToRenderTarget(
			IRenderTarget*			   rt,
			ID3D12GraphicsCommandList* cmd);

		/**
		 * \brief スプライトを指定されたレンダーターゲットへ描画する
		 * \param rt 描画先となるレンダーターゲット
		 * \param cmd 描画命令を記録するコマンドリスト
		 * \param pso 描画パイプラインを管理するサービス
		 * \param transitionToShaderResource 描画後にShaderResource状態へ遷移する場合はtrue
		 */
		void DrawSpritesToRenderTarget(
			IRenderTarget*			   rt,
			ID3D12GraphicsCommandList* cmd,
			PipelineService*		   pso,
			bool					   transitionToShaderResource);

		/*-----------------------------------------------------------------------------------------
		 * RuntimeScene
		 * - 現在実行しているシーンとSceneContextをまとめて保持する構造体
		 * - シーンに必要なアセットの読み込み状態を管理する
		 *---------------------------------------------------------------------------------------*/
		/**
		 * @brief RuntimeSceneに関するデータを保持する構造体です。
		 */
		struct RuntimeScene {
			/// 現在実行しているシーン
			std::unique_ptr<BaseScene> scene;

			/// 現在のシーンが使用するコンテキスト
			std::unique_ptr<SceneContext> ctx;

			/// シーンで使用するアセットの読み込みが完了しているか
			bool assetsLoaded = false;
		};

		/*-----------------------------------------------------------------------------------------
		 * SceneTransitionService
		 * - ISceneTransitionRequestorを実装し、外部からのシーン遷移要求を受け付けるクラス
		 * - 受け取った遷移要求をSceneManagerの内部処理へ転送する
		 *---------------------------------------------------------------------------------------*/
		/**
		 * @brief SceneTransitionServiceの機能を提供するクラスです。
		 */
		class SceneTransitionService final : public ISceneTransitionRequestor {
		public:
			/**
			 * \brief SceneTransitionServiceを生成する
			 * \param manager シーン遷移を管理するSceneManager
			 */
			explicit SceneTransitionService(SceneManager& manager)
				: manager_(manager) {
			}

			/**
			 * \brief ファイルパスを指定してシーン遷移を要求する
			 * \param path 遷移先となるシーンファイルのパス
			 */
			void RequestSceneChange(
				const std::filesystem::path& path) override {
				manager_.RequestSceneChangeInternal(path);
			}

			/**
			 * \brief ペイロード付きでシーン遷移を要求する
			 * \param path 遷移先となるシーンファイルのパス
			 * \param payload 遷移先シーンへ受け渡すデータ
			 */
			void RequestSceneChange(
				const std::filesystem::path&   path,
				std::unique_ptr<IScenePayload> payload) override {
				manager_.RequestSceneChangeInternal(
					path,
					std::move(payload));
			}

			/**
			 * \brief アセットGUIDを指定してシーン遷移を要求する
			 * \param guid 遷移先となるシーンアセットのGUID
			 */
			void RequestSceneChange(const Guid& guid) override {
				manager_.RequestSceneChangeInternal(guid);
			}

			/**
			 * \brief ペイロード付きでGUID指定のシーン遷移を要求する
			 * \param guid 遷移先となるシーンアセットのGUID
			 * \param payload 遷移先シーンへ受け渡すデータ
			 */
			void RequestSceneChange(
				const Guid&					   guid,
				std::unique_ptr<IScenePayload> payload) override {
				manager_.RequestSceneChangeInternal(
					guid,
					std::move(payload));
			}

			void RequestSceneChange(const std::filesystem::path& path,
				std::unique_ptr<BaseSceneTransitionEffect> effect) override {
				manager_.RequestSceneChangeInternal(path, nullptr, std::move(effect));
			}

			void RequestSceneChange(const Guid& guid,
				std::unique_ptr<BaseSceneTransitionEffect> effect) override {
				manager_.RequestSceneChangeInternal(guid, nullptr, std::move(effect));
			}

		private:
			/// シーン遷移要求の転送先となるSceneManager
			SceneManager& manager_;
		};

		/** \brief シーン遷移 */
		std::unique_ptr<SceneTransitionService> transitionService_;		   //< シーン遷移要求を受け付けるサービス
		RuntimeScene							activeScene_;			   //< 現在実行しているシーンとコンテキストを保持する構造体
		std::optional<std::filesystem::path>	pendingScenePath_;		   //< 保留中のシーン遷移先パス
		std::unique_ptr<IScenePayload>			pendingPayload_;		   //< 保留中のシーン遷移ペイロード
		SceneContext*							lastBoundCtx_	= nullptr; //< 最後にバインドされたSceneContext
		uint64_t								lastRuntimeGen_ = 0;	   //< 最後にバインドされたSceneContextのランタイム世代番号

		/** \brief 描画パス */
		std::unique_ptr<PickingPass>   pickingPass_;				//< ピッキング用描画パス
		std::unique_ptr<GridRenderer>  editorGridRenderer_;			//< エディタグリッド描画用レンダラー
		std::unique_ptr<ModelRenderer> editorPreviewModelRenderer_; //< エディタプレビュー用モデル描画用レンダラー

		/** \brief エディタ関連 */
		PlaySession*  pPlaySession_		= nullptr; //< 現在関連付けられている再生セッション
		SceneContext* editorPreviewCtx_ = nullptr; //< エディタプレビュー用のSceneContext
		bool		  renderDebugView_	= true;	   //< エディタ上でデバッグ表示を描画するか
		bool		  renderPicking_	= true;	   //< エディタ上でピッキング用描画を実行するか

		DxCore* dx_ = nullptr; //< DirectX 12のデバイスや描画環境を管理するDxCore
		std::unique_ptr<BaseSceneTransitionEffect> pendingTransitionEffect_;
		std::unique_ptr<BaseSceneTransitionEffect> activeTransitionEffect_;
		enum class TransitionPhase { None, FadeOut, FadeIn };
		TransitionPhase transitionPhase_ = TransitionPhase::None;
	};
} // namespace CalyxEngine
