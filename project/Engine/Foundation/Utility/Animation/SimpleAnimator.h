#pragma once

#include <unordered_map>
#include <string>
#include <memory>

#include "SimpleAnimChannel.h"

namespace CalyxUtil {

	/* ----------------------------------------------------------------------------
	/* SimpleAnimator class
	/* -複数のSimpleAnimChannelを管理するアニメーター
	/* ----------------------------------------------------------------------------*/
	class SimpleAnimator {
	public:
		//----------------------------
		// 追加
		//----------------------------
		template<typename T>
		SimpleAnimChannel<T>& Add(const std::string& name) {
			auto ch = std::make_unique<SimpleAnimChannel<T>>();
			auto& ref = *ch;
			GetMap<T>()[name] = std::move(ch);
			return ref;
		}

		//----------------------------
		// 更新
		//----------------------------
		void Update(float dt) {
			UpdateMap<float>(dt);
			UpdateMap<CalyxMath::Vector2>(dt);
			UpdateMap<CalyxMath::Vector3>(dt);
			UpdateMap<CalyxMath::Vector4>(dt);
		}

		void ShowGui(bool isLoop = true) {
			ShowGuiMap<float>("Float", isLoop);
			ShowGuiMap<CalyxMath::Vector2>("Vector2", isLoop);
			ShowGuiMap<CalyxMath::Vector3>("Vector3", isLoop);
			ShowGuiMap<CalyxMath::Vector4>("Vector4", isLoop);
		}

		//----------------------------
		// 取得
		//----------------------------
		template<typename T>
		const T& Get(const std::string& name) const {
			return GetMap<T>().at(name)->GetValue();
		}

		template<typename T>
		bool Has(const std::string& name) const {
			return GetMap<T>().contains(name);
		}

	private:
		std::unordered_map<std::string, std::unique_ptr<SimpleAnimChannel<float>>>            floatAnims_;
		std::unordered_map<std::string, std::unique_ptr<SimpleAnimChannel<CalyxMath::Vector2>>> vec2Anims_;
		std::unordered_map<std::string, std::unique_ptr<SimpleAnimChannel<CalyxMath::Vector3>>> vec3Anims_;
		std::unordered_map<std::string, std::unique_ptr<SimpleAnimChannel<CalyxMath::Vector4>>> vec4Anims_;

		template<typename T>
		auto& GetMap();

		template<typename T>
		auto& GetMap() const;

		template<typename T>
		void UpdateMap(float dt) {
			for(auto& [_, ch] : GetMap<T>()) {
				ch->Update(dt);
			}
		}

		//====================================
		// ImGui helper
		//====================================
		template<typename T>
		void ShowGuiMap(const char* typeLabel, bool isLoop) {
			auto& map = GetMap<T>();
			if(map.empty()) {
				return;
			}

			if(ImGui::CollapsingHeader(typeLabel, ImGuiTreeNodeFlags_DefaultOpen)) {
				for(auto& [name, ch] : map) {
					ImGui::PushID(name.c_str());
					ch->ShowGui(name, isLoop);
					ImGui::PopID();
				}
			}
		}
	};

	// float
	template<>
	inline auto& SimpleAnimator::GetMap<float>() { return floatAnims_; }
	template<>
	inline auto& SimpleAnimator::GetMap<float>() const { return floatAnims_; }

	// Vector2
	template<>
	inline auto& SimpleAnimator::GetMap<CalyxMath::Vector2>() { return vec2Anims_; }
	template<>
	inline auto& SimpleAnimator::GetMap<CalyxMath::Vector2>() const { return vec2Anims_; }

	// Vector3
	template<>
	inline auto& SimpleAnimator::GetMap<CalyxMath::Vector3>() { return vec3Anims_; }
	template<>
	inline auto& SimpleAnimator::GetMap<CalyxMath::Vector3>() const { return vec3Anims_; }

	// Vector4
	template<>
	inline auto& SimpleAnimator::GetMap<CalyxMath::Vector4>() { return vec4Anims_; }
	template<>
	inline auto& SimpleAnimator::GetMap<CalyxMath::Vector4>() const { return vec4Anims_; }


} // namespace CalyxUtil
