#include "LivePPService.h"
/* ========================================================================
	include space
===================================================================== */
#ifdef LIVEPP
#include <filesystem>
#include <string>
#endif // LIVEPP
#include "../../Clock/ClockManager.h"
#include "../../Input/Input.h"
#include <Windows.h>

namespace CalyxEngine {

#ifdef LIVEPP
	LivePPService* LivePPService::GetInstance() {
		return instance_;
	}

	LivePPService::LivePPService() {
		instance_ = this;
	}

	// --- Hooks ---
	void MyCompileStartHook(lpp::LppCompileStartHookId, const wchar_t* const recompiledModulePath, const wchar_t* const recompiledSourcePath) {
		if(auto* service = LivePPService::GetInstance()) {
			service->OnCompileStart(recompiledModulePath, recompiledSourcePath);
		}
	}
	LPP_COMPILE_START_HOOK(MyCompileStartHook);

	void MyCompileSuccessHook(lpp::LppCompileSuccessHookId, const wchar_t* const recompiledModulePath, const wchar_t* const recompiledSourcePath) {
		if(auto* service = LivePPService::GetInstance()) {
			service->OnCompileSuccess(recompiledModulePath, recompiledSourcePath);
		}
	}
	LPP_COMPILE_SUCCESS_HOOK(MyCompileSuccessHook);

	void MyCompileErrorHook(lpp::LppCompileErrorHookId, const wchar_t* const recompiledModulePath, const wchar_t* const recompiledSourcePath, const wchar_t* const compilerOutput) {
		if(auto* service = LivePPService::GetInstance()) {
			service->OnCompileError(recompiledModulePath, recompiledSourcePath, compilerOutput);
		}
	}
	LPP_COMPILE_ERROR_HOOK(MyCompileErrorHook);

	void MyPostPatchHook(lpp::LppHotReloadPostpatchHookId, const wchar_t* const recompiledModulePath, const wchar_t* const* const, unsigned int, const wchar_t* const* const, unsigned int) {
		if(auto* service = LivePPService::GetInstance()) {
			service->OnPostPatch(recompiledModulePath);
		}
	}
	LPP_HOTRELOAD_POSTPATCH_HOOK(MyPostPatchHook);

	void MyPrePatchHook(lpp::LppHotReloadPrepatchHookId, const wchar_t* const, const wchar_t* const* const, unsigned int, const wchar_t* const* const, unsigned int) {
		if(auto* service = LivePPService::GetInstance()) {
			service->OnPrePatch();
		}
	}
	LPP_HOTRELOAD_PREPATCH_HOOK(MyPrePatchHook);

	////////////////////////////////////////////////////////////////////////////////
	// 初期化: Live++ Agent のロードとモジュール登録
	////////////////////////////////////////////////////////////////////////////////
	void LivePPService::Initialize() {
		lpp::LppLocalPreferences   localPrefs			   = lpp::LppCreateDefaultLocalPreferences();
		lpp::LppProjectPreferences projectPrefs			   = lpp::LppCreateDefaultProjectPreferences();
		projectPrefs.general.spawnBrokerForLocalConnection = true;

		const wchar_t*		  exePath = lpp::LppGetCurrentModulePath();
		std::filesystem::path livePPPath =
			std::filesystem::path(exePath).parent_path() / L"../../../project/externals/LivePP";
		livePPPath = livePPPath.lexically_normal();

		agent_ = lpp::LppCreateSynchronizedAgentWithPreferences(&localPrefs, livePPPath.c_str(), &projectPrefs);

		if(lpp::LppIsValidSynchronizedAgent(&agent_)) {
			OutputDebugStringW(L"[LivePP] Synchronized Agent loaded successfully.\n");
			agent_.EnableModule(lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_ALL_IMPORT_MODULES, nullptr, nullptr);
		}
	}

	void LivePPService::Update() {
		if(!lpp::LppIsValidSynchronizedAgent(&agent_)) return;

		using namespace CalyxFoundation;
		bool isCtrlPressed	= Input::PushKey(DIK_LCONTROL) || Input::PushKey(DIK_RCONTROL);
		bool isAltPressed	= Input::PushKey(DIK_LMENU) || Input::PushKey(DIK_RMENU);
		bool isF11Triggered = Input::TriggerKey(DIK_F11);

		if(isCtrlPressed && isAltPressed && isF11Triggered) {
			TriggerReload();
		}

		// リロードチェック（FPS低下を防ぐため、0.2秒ごとにポーリング）
		lastPollTime_ += ClockManager::GetInstance()->GetDeltaTime();
		if(lastPollTime_ >= 0.2f) {
			lastPollTime_ = 0.0f;
			if(agent_.WantsReload(lpp::LPP_RELOAD_OPTION_SYNCHRONIZE_WITH_RELOAD)) {
				// ここで同期リロードを実行
				// この呼び出しの中で PrePatch/PostPatch フックが（メインスレッド上で）同期的に呼ばれる
				agent_.Reload(lpp::LPP_RELOAD_BEHAVIOUR_WAIT_UNTIL_CHANGES_ARE_APPLIED);
			}
		}

		// Auto-reset Success status after 2 seconds
		if(status_ == LivePPStatus::Success) {
			successTimer_ += ClockManager::GetInstance()->GetDeltaTime();
			if(successTimer_ > 2.0f) {
				status_		  = LivePPStatus::Idle;
				successTimer_ = 0.0f;
			}
		}
	}

	void LivePPService::TriggerReload() {
#ifdef LIVEPP
		if(lpp::LppIsValidSynchronizedAgent(&agent_)) {
			OutputDebugStringW(L"[LivePP] Scheduling reload...\n");
			agent_.ScheduleReload();
		}
#endif
	}

	void LivePPService::Finalize() {
		lpp::LppDestroySynchronizedAgent(&agent_);
	}

	void LivePPService::OnPrePatch() {
		for(auto& cb : prePatchListeners_) {
			if(cb) cb();
		}
		OutputDebugStringW(L"[LivePP] Pre-patch hook executed.\n");
	}

	void LivePPService::OnCompileStart(const wchar_t* modulePath, const wchar_t* sourcePath) {
		status_				  = LivePPStatus::Compiling;
		lastRecompiledModule_ = modulePath;
		lastRecompiledSource_ = sourcePath;
		lastCompilerOutput_	  = "";
		OutputDebugStringW(L"[LivePP] Compile started.\n");
	}

	void LivePPService::OnCompileSuccess(const wchar_t*, const wchar_t*) {
		status_ = LivePPStatus::Patching;
		OutputDebugStringW(L"[LivePP] Compile success. Patching...\n");
	}

	void LivePPService::OnCompileError(const wchar_t*, const wchar_t*, const wchar_t* compilerOutput) {
		status_ = LivePPStatus::Error;
		// Convert wchar_t compiler output to std::string
		int			size = WideCharToMultiByte(CP_UTF8, 0, compilerOutput, -1, nullptr, 0, nullptr, nullptr);
		std::string message(size, '\0');
		WideCharToMultiByte(CP_UTF8, 0, compilerOutput, -1, &message[0], size, nullptr, nullptr);
		lastCompilerOutput_ = message;
		OutputDebugStringW(L"[LivePP] Compile error.\n");
	}

	void LivePPService::OnPostPatch(const wchar_t*) {
		status_		  = LivePPStatus::Success;
		successTimer_ = 0.0f;

		for(auto& cb : postPatchListeners_) {
			if(cb) cb();
		}

		OutputDebugStringW(L"[LivePP] Patch applied successfully.\n");
	}

#else
	LivePPService* LivePPService::GetInstance() { return instance_; }
	LivePPService::LivePPService() { instance_ = this; }
	void LivePPService::Initialize() {}
	void LivePPService::Update() {}
	void LivePPService::TriggerReload() {}
	void LivePPService::Finalize() {}
	void LivePPService::OnCompileStart(const wchar_t*, const wchar_t*) {}
	void LivePPService::OnCompileSuccess(const wchar_t*, const wchar_t*) {}
	void LivePPService::OnCompileError(const wchar_t*, const wchar_t*, const wchar_t*) {}
	void LivePPService::OnPrePatch() {}
	void LivePPService::OnPostPatch(const wchar_t*) {}
#endif // LIVEPP

} // namespace CalyxEngine
