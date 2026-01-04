#pragma once
class IRuntimeBehaviour {
public:
	virtual ~IRuntimeBehaviour() = default;
	virtual void Start() {}                // Awake 全完了後に一度
};