#pragma once

#include <Engine/System/Command/Manager/CommandManager.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/SetValueCommand/SetValueCommand.h>

#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

#include <functional>
#include <cmath>

#include <externals/imgui/imgui.h>

namespace GuiCmdInternal{

	template<typename T>
	class GuiCmdSetValueComputer{
	public:
		void Begin(const T& initial);
		std::unique_ptr<ICommand> End(const T& current,
									  std::function<void(const T&)> setter,
									  const std::string& label);

	private:
		bool IsNotEqual(const Vector2& a, const Vector2& b) const;
		bool IsNotEqual(const Vector3& a, const Vector3& b) const;
		bool IsNotEqual(const Vector4& a, const Vector4& b) const;
		bool IsNotEqual(bool a, bool b) const;
		bool IsNotEqual(float a, float b) const;
		bool IsNotEqual(int a, int b) const;

	private:
		T before_;
		bool dragging_ = false;


	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//		記録
	/////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline void GuiCmdSetValueComputer<T>::Begin(const T& initial){
		before_ = initial;
		dragging_ = true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		コマンド発行
	/////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::unique_ptr<ICommand> GuiCmdSetValueComputer<T>::End(const T& current,
															 std::function<void(const T&)> setter,
															 const std::string& label){
		dragging_ = false;

		if (!IsNotEqual(before_, current))
			return nullptr;

		return std::make_unique<SetValueCommand<T>>(label, before_, current, setter);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		違いがあるかチェック
	/////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline bool GuiCmdSetValueComputer<T>::IsNotEqual(const Vector2& a, const Vector2& b) const{
		const float eps = 0.0001f;
		return std::fabs(a.x - b.x) > eps || std::fabs(a.y - b.y) > eps;
	}

	template<typename T>
	inline bool GuiCmdSetValueComputer<T>::IsNotEqual(const Vector3& a, const Vector3& b) const{
		const float eps = 0.0001f;
		return std::fabs(a.x - b.x) > eps || std::fabs(a.y - b.y) > eps || std::fabs(a.z - b.z) > eps;
	}

	template<typename T>
	inline bool GuiCmdSetValueComputer<T>::IsNotEqual(const Vector4& a, const Vector4& b) const{
		const float eps = 0.0001f;
		return std::fabs(a.x - b.x) > eps || std::fabs(a.y - b.y) > eps ||
			std::fabs(a.z - b.z) > eps || std::fabs(a.w - b.w) > eps;
	}

	template<typename T>
	inline bool GuiCmdSetValueComputer<T>::IsNotEqual(bool a, bool b) const{
		return a != b;
	}

	template<typename T>
	inline bool GuiCmdSetValueComputer<T>::IsNotEqual(float a, float b) const{
		const float eps = 0.0001f;
		return std::fabs(a - b) > eps;
	}

	template<typename T>
	inline bool GuiCmdSetValueComputer<T>::IsNotEqual(int a, int b) const{
		return a != b;
	}

}