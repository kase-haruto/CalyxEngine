#pragma once
/* ========================================================================
	include space
===================================================================== */
#include <Engine/Foundation/Export/CalyxAPI.h>
#if defined(_DEBUG) || defined(DEVELOP)
#include <Externals/LivePP/API/x64/LPP_API_x64_CPP.h>
#endif // defined(_DEBUG) || defined(DEVELOP)

#include <functional>
#include <string>
#include <vector>

namespace CalyxEngine {

	enum class LivePPStatus {
		Idle,
		Compiling,
		Patching,
		Success,
		Error
	};

	/*-----------------------------------------------------------------------------------------
	 * LivePPService
	 * - Live++ ホットリロードの管理サービス
	 * - LIVEPP マクロが定義されているビルドのみで有効化される
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief LivePPServiceの機能を提供するクラスです。
	 */
	class LivePPService {
	public:
		static CALYX_API LivePPService* GetInstance();

		using HookCallback = std::function<void()>;

		CALYX_API LivePPService();
		~LivePPService() = default;

		void AddPrePatchListener(HookCallback cb) { prePatchListeners_.push_back(std::move(cb)); }
		void AddPostPatchListener(HookCallback cb) { postPatchListeners_.push_back(std::move(cb)); }

		CALYX_API void Initialize();
		CALYX_API void Update();
		CALYX_API void Finalize();

		// Trigger hot reload manually
		CALYX_API void TriggerReload();

		// Status access
		LivePPStatus		GetStatus() const { return status_; }
		const std::string&	GetLastCompilerOutput() const { return lastCompilerOutput_; }
		const std::wstring& GetLastRecompiledModule() const { return lastRecompiledModule_; }
		const std::wstring& GetLastRecompiledSource() const { return lastRecompiledSource_; }

		// internal: used by hooks
		CALYX_API void OnPrePatch();
		CALYX_API void OnCompileStart(const wchar_t* modulePath, const wchar_t* sourcePath);
		CALYX_API void OnCompileSuccess(const wchar_t* modulePath, const wchar_t* sourcePath);
		CALYX_API void OnCompileError(const wchar_t* modulePath, const wchar_t* sourcePath, const wchar_t* compilerOutput);
		CALYX_API void OnPostPatch(const wchar_t* modulePath);

	private:
		static LivePPService* instance_;

#if defined(_DEBUG) || defined(DEVELOP)
		lpp::LppSynchronizedAgent agent_{};
#endif // defined(_DEBUG) || defined(DEVELOP)

		float lastPollTime_ = 0.0f;

		LivePPStatus status_	   = LivePPStatus::Idle;
		float		 successTimer_ = 0.0f;
		std::string	 lastCompilerOutput_;
		std::wstring lastRecompiledModule_;
		std::wstring lastRecompiledSource_;

		std::vector<HookCallback> prePatchListeners_;
		std::vector<HookCallback> postPatchListeners_;
	};
} // namespace CalyxEngine
