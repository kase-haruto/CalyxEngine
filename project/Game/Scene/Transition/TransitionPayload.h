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
class TransitionPayload final
	: public CalyxEngine::IScenePayload {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/** \brief コンストラクタ*/
	TransitionPayload();
	~TransitionPayload() override;

public:
	SceneType type = SceneType::TITLE;
};