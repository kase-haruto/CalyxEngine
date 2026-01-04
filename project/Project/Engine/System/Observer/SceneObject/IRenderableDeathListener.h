#pragma once

class IRenderableDeathListener {
public:
	virtual void OnRenderableDestroyed(class IMeshRenderable* renderable) = 0;
};