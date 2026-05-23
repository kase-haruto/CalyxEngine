#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "SimpleAnimChannel.h"

namespace CalyxEngine {

	/* ----------------------------------------------------------------------------
	/* SimpleAnimator class
	/* -複数のSimpleAnimChannelを管理するアニメーター
	/* ----------------------------------------------------------------------------*/
	class SimpleAnimator {
	public:
		//===================================================================*/
		//                    private methods
		//===================================================================*/
		/**
		 * \brief 指定した型のアニメーションチャンネルを追加
		 * \tparam T アニメーションの型
		 * \param name アニメーション名
		 * \return 追加したアニメーションチャンネルの参照
		 */
		template <typename T>
		SimpleAnimChannel<T>& Add(const std::string& name) {
			auto  ch		  = std::make_unique<SimpleAnimChannel<T>>();
			auto& ref		  = *ch;
			GetMap<T>()[name] = std::move(ch);
			return ref;
		}
		/**
		 * \brief 更新処理
		 * \param dt デルタタイム
		 */
		void Update(float dt) {
			UpdateMap<float>(dt);
			UpdateMap<CalyxEngine::Vector2>(dt);
			UpdateMap<CalyxEngine::Vector3>(dt);
			UpdateMap<CalyxEngine::Vector4>(dt);
		}
		/**
		 * \brief GUI表示
		 * \param isLoop ループ設定を表示するか
		 */
		void ShowGui(bool isLoop = true) {
			ShowGuiMap<float>("Float", isLoop);
			ShowGuiMap<CalyxEngine::Vector2>("Vector2", isLoop);
			ShowGuiMap<CalyxEngine::Vector3>("Vector3", isLoop);
			ShowGuiMap<CalyxEngine::Vector4>("Vector4", isLoop);
		}
		/**
		 * \brief 指定した名前のアニメーションの値を取得
		 * \tparam T アニメーションの型
		 * \param name アニメーション名
		 * \return アニメーションの値の参照
		 */
		template <typename T>
		const T& Get(const std::string& name) const {
			return GetMap<T>().at(name)->GetValue();
		}
		/**
		 * \brief 指定した名前のアニメーションが存在するか
		 * \tparam T アニメーションの型
		 * \param name アニメーション名
		 * \return 存在する場合true
		 */
		template <typename T>
		bool Has(const std::string& name) const {
			return GetMap<T>().contains(name);
		}

		template <typename T>
		SimpleAnimChannel<T>& GetChannel(const std::string& name) {
			return *GetMap<T>().at(name);
		}

		template <typename T>
		const SimpleAnimChannel<T>& GetChannel(const std::string& name) const {
			return *GetMap<T>().at(name);
		}

		void Reset();

	private:
		//===================================================================*/
		//                    private methods
		//===================================================================*/
		/**
		 * \brief 非const版GetMap
		 * \tparam T
		 * \return
		 */
		template <typename T>
		auto& GetMap();
		/**
		 * \brief const版GetMap
		 * \tparam T
		 * \return
		 */
		template <typename T>
		auto& GetMap() const;
		/**
		 * \brief アニメーション更新
		 * \tparam T
		 * \param dt
		 */
		template <typename T>
		void UpdateMap(float dt) {
			for(auto& [_, ch] : GetMap<T>()) {
				ch->Update(dt);
			}
		}
		/**
		 * \brief GUI表示
		 * \tparam T
		 * \param typeLabel
		 * \param isLoop
		 */
		template <typename T>
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

	private:
		//===================================================================*/
		//                    public methods
		//===================================================================*/
		std::unordered_map<std::string, std::unique_ptr<SimpleAnimChannel<float>>>				floatAnims_; //< floatアニメーション
		std::unordered_map<std::string, std::unique_ptr<SimpleAnimChannel<CalyxEngine::Vector2>>> vec2Anims_;	 //< Vector2アニメーション
		std::unordered_map<std::string, std::unique_ptr<SimpleAnimChannel<CalyxEngine::Vector3>>> vec3Anims_;	 //< Vector3アニメーション
		std::unordered_map<std::string, std::unique_ptr<SimpleAnimChannel<CalyxEngine::Vector4>>> vec4Anims_;	 //< Vector4アニメーション
	};

	// float
	template <>
	inline auto& SimpleAnimator::GetMap<float>() { return floatAnims_; }
	template <>
	inline auto& SimpleAnimator::GetMap<float>() const { return floatAnims_; }

	// Vector2
	template <>
	inline auto& SimpleAnimator::GetMap<CalyxEngine::Vector2>() { return vec2Anims_; }
	template <>
	inline auto& SimpleAnimator::GetMap<CalyxEngine::Vector2>() const { return vec2Anims_; }

	// Vector3
	template <>
	inline auto& SimpleAnimator::GetMap<CalyxEngine::Vector3>() { return vec3Anims_; }
	template <>
	inline auto& SimpleAnimator::GetMap<CalyxEngine::Vector3>() const { return vec3Anims_; }

	// Vector4
	template <>
	inline auto& SimpleAnimator::GetMap<CalyxEngine::Vector4>() { return vec4Anims_; }
	template <>
	inline auto& SimpleAnimator::GetMap<CalyxEngine::Vector4>() const { return vec4Anims_; }

} // namespace CalyxEngine