#pragma once

/*-----------------------------------------------------------------------------------------
 * IRenderableDeathListener
 * - 描画対象の破棄通知を受け取るObserverインターフェース
 * - Rendererが保持する非所有参照を安全に解除するための通知契約を定義する
 * - 通知対象となるRenderableの所有権は持たない
 *---------------------------------------------------------------------------------------*/
class IRenderableDeathListener {
public:
	virtual void OnRenderableDestroyed(class IMeshRenderable* renderable) = 0;
};
