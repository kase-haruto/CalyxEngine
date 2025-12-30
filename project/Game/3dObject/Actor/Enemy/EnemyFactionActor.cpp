#include "EnemyFactionActor.h"

#include "Engine/System/Event/EventBus.h"
#include "Game/Battle/Shooting/Score/GainScore.h"

namespace {
	const char* EnemyKindToTypeTag(EnemyKind k) {
		switch (k) {
		case EnemyKind::Normal: return "enemyType:normal";
		case EnemyKind::Boss:   return "enemyType:boss";
		default:
			assert(false && "Unhandled EnemyKind");
			return "enemyType:unknown";
		}
	}

	const char* EnemyKindToIconTag(EnemyKind k) {
		switch (k) {
		case EnemyKind::Normal: return "icon:enemy_normal";
		case EnemyKind::Boss:   return "icon:enemy_boss";
		default:
			assert(false && "Unhandled EnemyKind");
			return "icon:enemy_unknown";
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ / デストラクタ
///////////////////////////////////////////////////////////////////////////////////////////////
EnemyFactionActor::EnemyFactionActor() = default;
EnemyFactionActor::EnemyFactionActor(const std::string& modelName, const std::string& objName) 
	: Actor(modelName, objName) {
}
EnemyFactionActor::~EnemyFactionActor() = default;


///////////////////////////////////////////////////////////////////////////////////////////////
//	スコア送信
///////////////////////////////////////////////////////////////////////////////////////////////
void EnemyFactionActor::PublishKillScore() const {
	GainScore e{};
	e.amount = killScore_;

	if (GetGuid().isValid()) {
		e.id = GetGuid();
	}

	e.reason = ScoreReason::EnemyKill;
	e.tag = {
		"src:enemy",
		"bucket:kill",
		EnemyKindToTypeTag(kind_),
		EnemyKindToIconTag(kind_)
	};

	EventBus::Publish(e);
}

CalyxEngine::ParamPath EnemyFactionActor::GetParamPath() const {
	return {
		CalyxEngine::ParamDomain::Game,
		(kind_ == EnemyKind::Boss) ? "Boss" : "Enemy"
	};
}

///////////////////////////////////////////////////////////////////////////////////////////////
//	accessore
///////////////////////////////////////////////////////////////////////////////////////////////
/** \brief 敵種別設定 */
void EnemyFactionActor::SetEnemyKind(EnemyKind kind) { kind_ = kind; }
/** \brief 敵種別取得 */
EnemyKind EnemyFactionActor::GetEnemyKind() const { return kind_; }
/** \brief キルスコア取得 */
int32_t	  EnemyFactionActor::GetKillScore() const{ return killScore_; }


