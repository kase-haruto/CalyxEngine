#include "EnemyEngagementInstaller.h"

#include <Game/Runtime/Engagement/EnemyEngagementService.h>

// engine
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>

// game
#include <Game/3dObject/Actor/Player/Player.h>

namespace Installers {

	std::unique_ptr<EnemyEngagementService>
		InstallEnemyEngagement(SceneContext& ctx, const EnemyEngagementParams& p) {
		auto svc = std::make_unique<EnemyEngagementService>();

		// パラメータ反映
		svc->SetOnScreenPad(p.ndcPad);
		svc->SetMinExposeTime(p.minExposeSec);
		svc->SetMaxEngageDistance(p.maxEngageDist);
		svc->EnableLineOfSight(p.useLOS);

		// --- プレイヤー判定（LOS用：hit.hitObject が Player か？） ---
		svc->SetLineOfSightPredicate([](void* obj) -> bool {
			auto* so = static_cast<SceneObject*>(obj);
			return (so && so->GetObjectTypeName() == "Player");
			// 列挙/タグ/レイヤがあるなら、そちらでの判定に置き換えてOK
		});

		// --- レイキャスト候補（環境 + プレイヤー）---
		svc->SetRaycastCandidatesProvider(
			[&ctx](std::vector<SceneObject*>& out, const SceneObject* ignore) {
			out.clear();
			out.reserve(256);

			// ① 環境（地形/壁など）= BaseGameObject を列挙して追加
			if (auto* lib = ctx.GetObjectLibrary()) {
				auto env = lib->FindByType<BaseGameObject>();
				for (auto& sp : env) {
					auto* bg = sp.get();
					if (!bg || bg == ignore) continue;

					const std::string tn = bg->GetObjectTypeName();
					if (tn == "Player" || tn == "Enemy" || tn == "Bullet") continue;

					if (bg->IsEnableRaycast()) {
						out.push_back(bg);
					}
				}
			}

			if (auto player = ctx.FindFirst<Player>()) {
				SceneObject* pso = player.get();
				if (pso && pso != ignore) {
					out.push_back(pso);
				}
			}
		}
		);

		// 依存解決
		svc->OnSceneLoaded(ctx);
		return svc;
	}

} // namespace Installers
