#pragma once
/* ========================================================================
	include space
===================================================================== */
#ifdef LIVEPP
#include <LivePP/API/x64/LPP_API_x64_CPP.h>
#endif // LIVEPP

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

		LivePPService();
		~LivePPService() = default;

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
		void OnCompileStart(const wchar_t* modulePath, const wchar_t* sourcePath);
		void OnCompileSuccess(const wchar_t* modulePath, const wchar_t* sourcePath);
		void OnCompileError(const wchar_t* modulePath, const wchar_t* sourcePath, const wchar_t* compilerOutput);
		void OnPostPatch(const wchar_t* modulePath);

	private:
		inline static LivePPService* instance_ = nullptr;

#ifdef LIVEPP
		lpp::LppDefaultAgent agent_{};
#endif // LIVEPP

		LivePPStatus status_	   = LivePPStatus::Idle;
		float		 successTimer_ = 0.0f;
		std::string	 lastCompilerOutput_;
		std::wstring lastRecompiledModule_;
		std::wstring lastRecompiledSource_;
	};
} // namespace CalyxEngine
