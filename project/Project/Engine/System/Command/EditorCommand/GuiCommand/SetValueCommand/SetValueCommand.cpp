#include "SetValueCommand.h"
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>


template<typename T>
void SetValueCommand<T>::UpdateName(){
	name_ = "Set Value"; // fallback
}

template<>
 void SetValueCommand<float>::UpdateName(){
	name_ = "Set Float: \"" + label_ + "\" from " + std::to_string(before_) + " -> " + std::to_string(after_);
}

template<>
 void SetValueCommand<Vector2>::UpdateName(){
	auto toStr = [] (const Vector2& v){
		return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + ")";
		};
	name_ = "Set Vector2: \"" + label_ + "\" from " + toStr(before_) + " -> " + toStr(after_);
}

template<>
 void SetValueCommand<Vector3>::UpdateName(){
	auto toStr = [] (const Vector3& v){
		return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + ")";
		};
	name_ = "Set Vector3: \"" + label_ + "\" from " + toStr(before_) + " -> " + toStr(after_);
}

template<>
 void SetValueCommand<Vector4>::UpdateName(){
	auto toStr = [] (const Vector4& v){
		return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + "," + std::to_string(v.w) + ")";
		};
	name_ = "Set Vector4: \"" + label_ + "\" from " + toStr(before_) + " -> " + toStr(after_);
}

template<>
 void SetValueCommand<bool>::UpdateName(){
	name_ = "Set Bool: \"" + label_ + "\" from " + (before_ ? "true" : "false") + " -> " + (after_ ? "true" : "false");
}

template<>
 void SetValueCommand<int>::UpdateName(){
	name_ = "Set Int: \"" + label_ + "\" from " + std::to_string(before_) + " -> " + std::to_string(after_);
}