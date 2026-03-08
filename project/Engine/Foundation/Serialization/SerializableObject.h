#pragma once

#include "Engine/Foundation/Math/Vector2.h"
#include "Engine/Foundation/Math/Vector4.h"
#include "SerializableField.h"
#include "SerializableFieldBuilder.h"

#include <string>
#include <type_traits>
#include <vector>

namespace CalyxEngine {

	enum class ParamDomain {
		Game,
		Engine,
		Editor,
	};

	struct ParamPath {
		ParamDomain domain;
		std::string name;

		std::optional<std::string> subDirectory;
	};

	/*-----------------------------------------------------------------------------------------
	 * SerializableObject
	 * - シリアライズ可能オブジェクト基底クラス
	 * - メンバ変数を保存対象として登録し、外部ファイルとの同期を管理するクラス
	 *---------------------------------------------------------------------------------------*/
	class SerializableObject {
	public:
		/**
		 * \brief デストラクタ
		 */
		virtual ~SerializableObject() = default;

		/**
		 * \brief パラメータパスを取得
		 * \return パラメータパス
		 */
		virtual ParamPath GetParamPath() const { return {ParamDomain::Game,"Default",std::nullopt}; }

		// --- 各オブジェクトから呼ぶAPI ---
		/**
		 * \brief パラメータを保存
		 * \return 成功したか
		 */
		bool SaveParams() const;
		/**
		 * \brief パラメータを読み込み
		 * \return 成功したか
		 */
		bool LoadParams();

		/**
		 * \brief 保存/読み込みボタンのGUIを表示
		 */
		void SaveAndLoadButtonGui();

		/**
		 * \brief フィールドリストを取得（編集用）
		 * \return フィールドリスト
		 */
		std::vector<SerializableField>& FieldsMutable() { return fields_; }
		/**
		 * \brief フィールドリストを取得
		 * \return フィールドリスト
		 */
		const std::vector<SerializableField>& Fields() const { return fields_; }

	public:
		bool ShowGui();

	protected:
		/**
		 * \brief コンストラクタ
		 */
		SerializableObject() = default;

		/**
		 * \brief フィールドを追加
		 * \param key キー名
		 * \param value 変数への参照
		 */
		template <typename T>
		FieldBuilder AddField(const std::string& key, T& value) {
			fields_.push_back(SerializableField{
				key,
				ValuePtr{&value}});
			return FieldBuilder(fields_.back());
		}


	private:
		//===================================================================*/
		//                    private member variables
		//===================================================================*/
		std::vector<SerializableField> fields_; //< フィールドリスト
	};

} // namespace CalyxEnginez