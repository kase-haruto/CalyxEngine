#pragma once
#include <memory>

class SceneContext;
class EnemyEngagementService;

// セットアップ用パラメータ
struct EnemyEngagementParams {
    float ndcPad = 0.05f;   // 画面端の余白（NDC）
    float minExposeSec = 0.20f;   // 露出時間しきい値
    float maxEngageDist = 120.0f;  // 射程
    bool  useLOS = true;    // ライン・オブ・サイト使用
};

namespace Installers {

    // シーン依存のセットアップを1か所に集約
    std::unique_ptr<EnemyEngagementService>
        InstallEnemyEngagement(SceneContext& ctx, const EnemyEngagementParams& p = {});

}