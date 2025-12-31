#pragma once
/*===========================================================================
 *	include space
 * ========================================================================*/
#include <Engine/Scene/Transitioner/IScenePayload.h>
#include <Game/3dObject/Actor/Enemy/Details/EnemyKind.h>
#include <cstdint>
#include <vector>

/*-----------------------------------------------------------------------------------------
 *  結果シーン遷移ペイロードクラス
 *  - 結果シーンへの遷移時に使用するペイロード
 *---------------------------------------------------------------------------------------*/
class ResultTransitionPayload final
	:public CalyxScene::IScenePayload{
public:
	//===================================================================*/
	//			structs
	//===================================================================*/
	struct ResultEntry {
		EnemyKind kind;		//< 敵種別
		int32_t   count;	//< 撃破数
		int32_t   score;	//< 獲得スコア
	};
	
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/** \brief コンストラクタ*/
	ResultTransitionPayload();
	~ResultTransitionPayload() override;

public:
	int32_t score = 0;					//< 最終スコア
	std::vector<ResultEntry> results;	//< 敵撃破内訳
};
