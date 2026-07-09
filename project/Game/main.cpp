#include <CalyxEngine/CalyxEngine.h>
#include <CalyxEngine/Project.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>
#include <Engine/Foundation/Reflection/CalyxGameObjectRegistry.generated.h>
#include <Engine/Foundation/Reflection/CalyxObjectRegistry.generated.h>

#include "GameApplication.h"

#include <filesystem>
#include <memory>
#include <Windows.h>

namespace {

	/*-----------------------------------------------------------------------------------------
	 * GameModule
	 * - ゲーム DLL のロード、アプリケーション生成、エディタ拡張登録関数の取得を担当する
	 * - DLL の所有権を持ち、破棄時に安全にアンロードする
	 *---------------------------------------------------------------------------------------*/
	class GameModule {
	public:
		using CreateApplication = Calyx::Application* (*)();		 //< ゲームアプリケーション生成関数
		using DestroyApplication = void (*)(Calyx::Application*); //< ゲームアプリケーション破棄関数
		using RegisterEditorTools = CalyxEditor::RegisterEditorToolsFn; //< ゲーム側エディタ拡張登録関数

		////////////////////////////////////////////////////////////////////////////////////////////
		//		デストラクタ
		////////////////////////////////////////////////////////////////////////////////////////////
		~GameModule() {
			Unload();
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		ゲーム DLL ロード
		////////////////////////////////////////////////////////////////////////////////////////////
		bool Load(const std::filesystem::path& modulePath) {
			// 既に読み込んでいる DLL があれば先に解放する
			Unload();

			// DLL パスが無効な場合はロードしない
			if(modulePath.empty() || !std::filesystem::exists(modulePath)) {
				return false;
			}

			// Windows API でゲーム DLL をロードする
			handle_ = ::LoadLibraryW(modulePath.wstring().c_str());
			if(!handle_) {
				return false;
			}

			// ゲームアプリケーションの生成/破棄関数を取得する
			create_ = reinterpret_cast<CreateApplication>(::GetProcAddress(handle_, "CreateCalyxApplication"));
			destroy_ = reinterpret_cast<DestroyApplication>(::GetProcAddress(handle_, "DestroyCalyxApplication"));
			if(!create_ || !destroy_) {
				Unload();
				return false;
			}

			return true;
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		ゲームアプリケーション生成
		////////////////////////////////////////////////////////////////////////////////////////////
		Calyx::Application* Create() const {
			return create_ ? create_() : nullptr;
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		ゲームアプリケーション破棄
		////////////////////////////////////////////////////////////////////////////////////////////
		void Destroy(Calyx::Application* application) const {
			if(destroy_ && application) {
				destroy_(application);
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		DLL ハンドル取得
		////////////////////////////////////////////////////////////////////////////////////////////
		void* GetHandle() const { return handle_; }

		////////////////////////////////////////////////////////////////////////////////////////////
		//		エディタ拡張登録関数取得
		////////////////////////////////////////////////////////////////////////////////////////////
		RegisterEditorTools GetEditorRegistration() const {
			// エディタ拡張が存在しないゲーム DLL では nullptr を返す
			return handle_ ? reinterpret_cast<RegisterEditorTools>(
				::GetProcAddress(handle_, "RegisterCalyxEditorTools")) : nullptr;
		}

	private:
		////////////////////////////////////////////////////////////////////////////////////////////
		//		ゲーム DLL アンロード
		////////////////////////////////////////////////////////////////////////////////////////////
		void Unload() {
			// DLL がロード済みであれば解放する
			if(handle_) {
				::FreeLibrary(handle_);
				handle_ = nullptr;
			}

			// DLL 内関数ポインタを無効化する
			create_ = nullptr;
			destroy_ = nullptr;
		}

		HMODULE handle_ = nullptr;				 //< ゲーム DLL ハンドル
		CreateApplication create_ = nullptr;	 //< ゲームアプリケーション生成関数
		DestroyApplication destroy_ = nullptr; //< ゲームアプリケーション破棄関数
	};

	////////////////////////////////////////////////////////////////////////////////////////////
	//		ゲーム DLL パス選択
	////////////////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path SelectGameModulePath(const Calyx::ProjectInfo& project) {
		// 起動構成が未指定の場合は Develop DLL を使用する
		const std::string config = project.launchConfiguration.empty() ? "Develop" : project.launchConfiguration;

		// 構成別 DLL パスが指定されていれば優先する
		if(config == "Debug" && !project.gameModuleDebug.empty()) return Calyx::ResolveProjectPath(project, project.gameModuleDebug);
		if(config == "Develop" && !project.gameModuleDevelop.empty()) return Calyx::ResolveProjectPath(project, project.gameModuleDevelop);
		if(config == "Release" && !project.gameModuleRelease.empty()) return Calyx::ResolveProjectPath(project, project.gameModuleRelease);

		// 構成別指定が無い場合は共通の gameModule を使用する
		return Calyx::ResolveProjectPath(project, project.gameModule);
	}

	/*-----------------------------------------------------------------------------------------
	 * GameHostApplication
	 * - CalyxGame.exe 側のホストアプリケーション
	 * - プロジェクトに指定されたゲーム DLL をロードし、実際のゲーム Application へ処理を委譲する
	 * - Engine UI が利用可能な場合はゲーム側エディタ拡張も登録する
	 *---------------------------------------------------------------------------------------*/
	class GameHostApplication : public Calyx::Application {
	public:
		////////////////////////////////////////////////////////////////////////////////////////////
		//		デストラクタ
		////////////////////////////////////////////////////////////////////////////////////////////
		~GameHostApplication() override {
			UnloadGameApplication();
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		プロジェクトロード通知
		////////////////////////////////////////////////////////////////////////////////////////////
		void OnProjectLoaded(const Calyx::ProjectInfo& project) override {
			// プロジェクト情報を保持してゲーム DLL の選択に使用する
			project_ = project;

			// プロジェクトに紐づくゲーム DLL をロードする
			LoadGameApplication();

			// 実ゲームアプリケーションへプロジェクトロードを通知する
			ActiveApplication().OnProjectLoaded(project);
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		SceneManager 準備完了通知
		////////////////////////////////////////////////////////////////////////////////////////////
		void OnSceneManagerReady(CalyxEngine::SceneManager& sceneManager) override {
			ActiveApplication().OnSceneManagerReady(sceneManager);
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		Engine UI 準備完了通知
		////////////////////////////////////////////////////////////////////////////////////////////
		void OnEngineUiReady(CalyxEngine::EngineUICore& engineUi) override {
			// エディタ拡張登録に使用する Engine UI を保持する
			engineUi_ = &engineUi;

			// ゲーム DLL が既にロードされていればエディタ拡張を登録する
			RegisterGameEditorTools();

			// 実ゲームアプリケーションへ Engine UI 準備完了を通知する
			ActiveApplication().OnEngineUiReady(engineUi);
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		初期化
		////////////////////////////////////////////////////////////////////////////////////////////
		void OnInitialize() override {
			ActiveApplication().OnInitialize();
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		更新
		////////////////////////////////////////////////////////////////////////////////////////////
		void OnUpdate() override {
			ActiveApplication().OnUpdate();
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		描画
		////////////////////////////////////////////////////////////////////////////////////////////
		void OnRender() override {
			ActiveApplication().OnRender();
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		終了処理
		////////////////////////////////////////////////////////////////////////////////////////////
		void OnFinalize() override {
			// ゲーム DLL 解放前にエディタ拡張を解除する
			UnregisterGameEditorTools();

			// 実ゲームアプリケーションの終了処理を実行する
			ActiveApplication().OnFinalize();

			// ゲームアプリケーションと DLL を解放する
			UnloadGameApplication();
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		Engine UI 描画判定
		////////////////////////////////////////////////////////////////////////////////////////////
		bool ShouldRenderEngineUi() const override {
			return ActiveApplication().ShouldRenderEngineUi();
		}

	private:
		////////////////////////////////////////////////////////////////////////////////////////////
		//		アクティブアプリケーション取得
		////////////////////////////////////////////////////////////////////////////////////////////
		Calyx::Application& ActiveApplication() const {
			// ゲーム DLL が有効な場合はゲームアプリケーションを使用する
			return gameApplication_ ? *gameApplication_ : fallbackApplication_;
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		ゲームアプリケーションロード
		////////////////////////////////////////////////////////////////////////////////////////////
		void LoadGameApplication() {
			// 既存のゲームアプリケーションを解放してからロードする
			UnloadGameApplication();

			// プロジェクト設定から対象 DLL を選択してロードする
			if(!gameModule_.Load(SelectGameModulePath(project_))) {
				return;
			}

			// DLL 内のファクトリ関数からゲームアプリケーションを生成する
			gameApplication_ = gameModule_.Create();

			// Engine UI が準備済みならゲーム側エディタ拡張を登録する
			RegisterGameEditorTools();
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		ゲームアプリケーションアンロード
		////////////////////////////////////////////////////////////////////////////////////////////
		void UnloadGameApplication() {
			// DLL を解放する前にエディタ拡張を解除する
			UnregisterGameEditorTools();

			// 生成済みゲームアプリケーションを DLL 側の破棄関数で破棄する
			if(gameApplication_) {
				gameModule_.Destroy(gameApplication_);
				gameApplication_ = nullptr;
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		ゲーム側エディタ拡張登録
		////////////////////////////////////////////////////////////////////////////////////////////
		void RegisterGameEditorTools() {
			// 登録済み、UI 未準備、DLL 未ロードの場合は何もしない
			if(gameEditorRegistered_ || !engineUi_ || !gameModule_.GetHandle()) return;

			// ゲーム DLL がエディタ拡張登録関数を公開していれば登録する
			if(auto entryPoint = gameModule_.GetEditorRegistration()) {
				gameEditorRegistered_ = engineUi_->RegisterEditorModule(gameModule_.GetHandle(), entryPoint, &project_);
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////
		//		ゲーム側エディタ拡張解除
		////////////////////////////////////////////////////////////////////////////////////////////
		void UnregisterGameEditorTools() {
			// 登録元 DLL のハンドルを使ってエディタ拡張を解除する
			if(engineUi_ && gameModule_.GetHandle()) {
				engineUi_->UnregisterEditorModule(gameModule_.GetHandle());
			}

			// 再ロード時に再登録できるよう状態を戻す
			gameEditorRegistered_ = false;
		}

		Calyx::ProjectInfo project_;						 //< 現在ロード中のプロジェクト情報
		mutable GameApplication fallbackApplication_;		 //< DLL 未ロード時に使用するフォールバックアプリケーション
		GameModule gameModule_;							 //< ゲーム DLL 管理
		CalyxEngine::EngineUICore* engineUi_ = nullptr;	 //< Engine UI 参照
		Calyx::Application* gameApplication_ = nullptr;	 //< DLL から生成したゲームアプリケーション
		bool gameEditorRegistered_ = false;				 //< ゲーム側エディタ拡張の登録状態
	};

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int){
	// エンジン側の生成済み SceneObject 型を登録する
	CalyxEngine::RegisterGeneratedSceneObjects();

	// ゲームホスト側に含まれる生成済み SceneObject 型を登録する
	CalyxEngine::RegisterGeneratedGameSceneObjects();

	// ゲームホストアプリケーションを起動する
	GameHostApplication application;
	return Calyx::Run(hInstance, application, commandLine);
}
