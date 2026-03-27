#include "LivePPService.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
#ifdef LIVEPP
#include <filesystem>
#include <string>
#endif // LIVEPP
#include <Windows.h>

namespace CalyxEngine {

#ifdef LIVEPP
	////////////////////////////////////////////////////////////////////////////////
	// 初期化: Live++ Agent のロードとモジュール登録
	////////////////////////////////////////////////////////////////////////////////
	void LivePPService::Initialize() {
		lpp::LppLocalPreferences   localPrefs	= lpp::LppCreateDefaultLocalPreferences();
		lpp::LppProjectPreferences projectPrefs = lpp::LppCreateDefaultProjectPreferences();
		// Broker が未起動の場合は自動で起動する（デフォルトで true だが明示）
		projectPrefs.general.spawnBrokerForLocalConnection = true;

		// 実行ファイルのパスからプロジェクトルートを逆算して
		// externals/LivePP へのパスを構築する
		// 実行ファイル: <SolutionDir>/../generated/outputs/<Config>/MyEngine.exe
		// LivePP     : <ProjectDir>/externals/LivePP
		const wchar_t*		  exePath = lpp::LppGetCurrentModulePath();
		std::filesystem::path livePPPath =
			std::filesystem::path(exePath).parent_path() // generated/outputs/<Config>/
			/ L"../../../project/externals/LivePP";
		livePPPath = livePPPath.lexically_normal();

		// デバッグ用に構築したパスを出力ウィンドウに出力
		std::wstring debugMsg = L"[LivePP] EXE Path: " + std::wstring(exePath) + L"\n";
		debugMsg += L"[LivePP] Target LivePP Path: " + livePPPath.wstring() + L"\n";
		OutputDebugStringW(debugMsg.c_str());

		agent_ = lpp::LppCreateSynchronizedAgentWithPreferences(
			&localPrefs, livePPPath.c_str(), &projectPrefs);

		if(lpp::LppIsValidSynchronizedAgent(&agent_)) {
			OutputDebugStringW(L"[LivePP] Agent loaded successfully.\n");
			// 現在の実行モジュール（.exe）をホットリロード対象として登録する
			agent_.EnableModule(
				lpp::LppGetCurrentModulePath(),
				lpp::LPP_MODULES_OPTION_ALL_IMPORT_MODULES,
				nullptr,
				nullptr);
		} else {
			OutputDebugStringW(L"[LivePP] Failed to load Agent.\n");
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	// 更新: ホットリロードの同期処理（毎フレーム呼ぶ）
	////////////////////////////////////////////////////////////////////////////////
	void LivePPService::Update() {
		if(!lpp::LppIsValidSynchronizedAgent(&agent_)) {
			return;
		}

		if(agent_.WantsReload(lpp::LPP_RELOAD_OPTION_SYNCHRONIZE_WITH_RELOAD)) {
			agent_.Reload(lpp::LPP_RELOAD_BEHAVIOUR_WAIT_UNTIL_CHANGES_ARE_APPLIED);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	// 終了: Agent の破棄
	////////////////////////////////////////////////////////////////////////////////
	void LivePPService::Finalize() {
		lpp::LppDestroySynchronizedAgent(&agent_);
	}

#else
	// LIVEPP 未定義時は空実装
	void LivePPService::Initialize() {
		OutputDebugStringW(L"[LivePP] Warning: LIVEPP macro is not defined! Building with a configuration other than Develop_LivePP.\n");
	}
	void LivePPService::Update() {}
	void LivePPService::Finalize() {}
#endif // LIVEPP

} // namespace CalyxEngine