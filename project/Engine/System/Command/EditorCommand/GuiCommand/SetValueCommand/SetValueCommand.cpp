#include "SetValueCommand.h"
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>


template<typename T>
void SetValueCommand<T>::UpdateName(){
	// 特殊化されていない型でもUndo履歴へ表示可能なFallback名を設定する。
	name_ = "Set Value"; // fallback
}

template<>
 void SetValueCommand<float>::UpdateName(){
	name_ = "Set Float: \"" + label_ + "\" from " + std::to_string(before_) + " -> " + std::to_string(after_);
}

template<>
 void SetValueCommand<CalyxEngine::Vector2>::UpdateName(){
	// 変更前後の各成分を履歴名へ含め、Inspector操作の内容を識別可能にする。
	auto toStr = [] (const CalyxEngine::Vector2& v){
		return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + ")";
		};
	name_ = "Set CalyxEngine::Vector2: \"" + label_ + "\" from " + toStr(before_) + " -> " + toStr(after_);
}

template<>
 void SetValueCommand<CalyxEngine::Vector3>::UpdateName(){
	// Vector値は一つのCommandとして表現し、軸ごとの変更履歴へ分割しない。
	auto toStr = [] (const CalyxEngine::Vector3& v){
		return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + ")";
		};
	name_ = "Set CalyxEngine::Vector3: \"" + label_ + "\" from " + toStr(before_) + " -> " + toStr(after_);
}

template<>
 void SetValueCommand<CalyxEngine::Vector4>::UpdateName(){
	// ColorなどVector4用途でも全成分の変更前後をUndo履歴へ残す。
	auto toStr = [] (const CalyxEngine::Vector4& v){
		return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + "," + std::to_string(v.w) + ")";
		};
	name_ = "Set CalyxEngine::Vector4: \"" + label_ + "\" from " + toStr(before_) + " -> " + toStr(after_);
}

template<>
 void SetValueCommand<bool>::UpdateName(){
	name_ = "Set Bool: \"" + label_ + "\" from " + (before_ ? "true" : "false") + " -> " + (after_ ? "true" : "false");
}

template<>
 void SetValueCommand<int>::UpdateName(){
	name_ = "Set Int: \"" + label_ + "\" from " + std::to_string(before_) + " -> " + std::to_string(after_);
}
