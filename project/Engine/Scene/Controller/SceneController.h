#pragma once

class SceneContext;

class SceneController{
public:
	virtual ~SceneController() = default;
	virtual void OnEnter(SceneContext&, const struct SceneAsset&){};
	virtual void OnUpdate(SceneContext&,[[maybe_unused]] float dt){};
	virtual void OnExit(SceneContext&){};
};
