#pragma once
#include <string>

namespace CalyxEngine {
	// 前方宣言
	struct SerializableField;

	/*--------------------------------------------------------
 	*		SerializableFieldBuilder class
 	*		- シリアライズ可能フィールドビルダークラス
 	*------------------------------------------------------*/
	class FieldBuilder {
	public:
		//=========================================================
		// public methods
		//=========================================================
		/**
		 * \brief フィールドを指定してビルダーを作成
		 * \param f
		 */
		explicit FieldBuilder(SerializableField& f);
		/**
		 * \brief カテゴリ設定
		 * \param c カテゴリ名
		 */
		FieldBuilder& Category(const std::string& c);
		/**
		 * \brief ツールチップ設定
		 * \param t ツールチップ文字列
		 */
		FieldBuilder& Tooltip(const std::string& t);
		/**
		 * \brief imguiのドラッグ速度設定
		 * \param s 速度
		 */
		FieldBuilder& Speed(float s);
		/**
		 * \brief 調整範囲指定
		 * \param mn 最小値
		 * \param mx 最大値
		 */
		FieldBuilder& Range(float mn,float mx);
		/**
		 * \brief 編集可能かどうか設定
		 */
		FieldBuilder& ReadOnly();
		/**
		 * \brief 非表示設定
		 */
		FieldBuilder& Hidden();

	private:
		//=========================================================
		// private members
		//=========================================================
		SerializableField& field_;
	};
}