#pragma once

class ObjectBase;

/* =========================================================
   汎用コンポーネント基底
 ========================================================= */
class IComponent {
public:
	virtual ~IComponent() = default;

	ObjectBase* owner = nullptr;
};
