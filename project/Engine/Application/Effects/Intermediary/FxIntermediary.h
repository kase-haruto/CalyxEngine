#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>

#include <memory>

class SceneContext;

/* ========================================================================
/* 作成したパーティクルをfxSystemに登録させる橋渡し
/* ===================================================================== */
class FxIntermediary {
public:
	//===================================================================*/
	//						singleton methods
	//===================================================================*/
	static FxIntermediary* GetInstance();

public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	void Attach(const std::shared_ptr<FxEmitter>& fxEmitter);
	void Detach(const std::shared_ptr<FxEmitter>& fxEmitter);

	//--------- accessor -----------------------------------------------------
	void SetSceneContext(SceneContext*);

public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	SceneContext* pSceneContext_;
};

