#pragma once

class ObjectBase;

/*-----------------------------------------------------------------------------------------
 * IComponent
 * - コンポーネントインターフェース
 * - オブジェクトにアタッチ可能なコンポーネントの基底クラス
 *---------------------------------------------------------------------------------------*/
/**
 * @brief IComponentの機能を提供するクラスです。
 */
class IComponent {
public:
	virtual ~IComponent() = default;

	ObjectBase* owner = nullptr;
};
