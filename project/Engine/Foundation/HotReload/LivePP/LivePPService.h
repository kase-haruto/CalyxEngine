#pragma once
/* ========================================================================
	include space
===================================================================== */
#ifdef LIVEPP
#include <LivePP/API/x64/LPP_API_x64_CPP.h>
#endif // LIVEPP

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
	class LivePPService {
	public:
		static LivePPService* GetInstance();

		using HookCallback = std::function<void()>;

		LivePPService();
		~LivePPService() = default;

		void AddPrePatchListener(HookCallback cb) { prePatchListeners_.push_back(std::move(cb)); }
		void AddPostPatchListener(HookCallback cb) { postPatchListeners_.push_back(std::move(cb)); }

		void Initialize();
		void Update();
		void Finalize();

		// Trigger hot reload manually
		void TriggerReload();

		// Status access
		LivePPStatus		GetStatus() const { return status_; }
		const std::string&	GetLastCompilerOutput() const { return lastCompilerOutput_; }
		const std::wstring& GetLastRecompiledModule() const { return lastRecompiledModule_; }
		const std::wstring& GetLastRecompiledSource() const { return lastRecompiledSource_; }

		// internal: used by hooks
		void OnPrePatch();
		void OnCompileStart(const wchar_t* modulePath, const wchar_t* sourcePath);
		void OnCompileSuccess(const wchar_t* modulePath, const wchar_t* sourcePath);
		void OnCompileError(const wchar_t* modulePath, const wchar_t* sourcePath, const wchar_t* compilerOutput);
		void OnPostPatch(const wchar_t* modulePath);

	private:
		inline static LivePPService* instance_ = nullptr;

#ifdef LIVEPP
		lpp::LppSynchronizedAgent agent_{};
#endif // LIVEPP

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
