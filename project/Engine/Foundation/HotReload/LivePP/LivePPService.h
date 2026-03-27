#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#ifdef LIVEPP
#include <LivePP/API/x64/LPP_API_x64_CPP.h>
#endif // LIVEPP

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * LivePPService
	 * - Live++ ホットリロードの管理サービス
	 * - LIVEPP マクロが定義されているビルドのみで有効化される
	 *---------------------------------------------------------------------------------------*/
	class LivePPService {
	public:
		LivePPService()	 = default;
		~LivePPService() = default;

		void Initialize();
		void Update();
		void Finalize();

	private:
#ifdef LIVEPP
		lpp::LppSynchronizedAgent agent_{};
#endif // LIVEPP
	};
} // namespace CalyxEngine
