#pragma once
/*===========================================================================
 *	include space
 * ========================================================================*/
#include <Engine/Scene/Transitioner/IScenePayload.h>
#include <Game/Scene/Details/SceneType.h>
#include <cstdint>
#include <vector>

/*-----------------------------------------------------------------------------------------
 *  シーン遷移ペイロードクラス
 *  - シーンへの遷移時に使用するペイロード
 *---------------------------------------------------------------------------------------*/
class GameTransitionPayload final
	: public CalyxEngine::IScenePayload {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/** \brief コンストラクタ*/
	GameTransitionPayload();
	~GameTransitionPayload() override;

public:
	int stageNum_ = 0;
	SceneType type_		= SceneType::SELECT;
};