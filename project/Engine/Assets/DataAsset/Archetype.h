#pragma once

#include "DataAsset.h"
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <externals/nlohmann/json.hpp>

namespace CalyxEngine {

	/**
	 * @brief オブジェクトの設計図（Prefab相当）クラス
	 */
	class Archetype : public DataAsset {
	public:
		Archetype() = default;
		virtual ~Archetype() = default;

		std::string GetAssetTypeName() const override { return "Archetype"; }

		/**
		 * @brief 既存のオブジェクトからアーキタイプを作成する
		 */
		void CreateFromObject(const std::shared_ptr<SceneObject>& object);

		/**
		 * @brief このアーキタイプを元に新しいオブジェクトを生成する
		 */
		std::shared_ptr<SceneObject> Instantiate() const;

	private:
		nlohmann::json serializedData_;
	};

}
