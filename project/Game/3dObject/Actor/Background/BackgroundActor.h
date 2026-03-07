#pragma once
// ===================================================================== */
//  include space
// ===================================================================== */
#include <Engine/Objects/3D/Actor/BaseGameObject.h>

/*-----------------------------------------------------------------------------------------
 * BackgroundActor
 * - 背景オブジェクトの基底クラス
 * - 背景オブジェクトの共通インターフェースや基本機能を提供
 * - 具体的な背景オブジェクトはこのクラスを継承して実装する
 *---------------------------------------------------------------------------------------*/
class BackgroundActor :
	public BaseGameObject {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/**
 	* \brief コンストラクタ
 	* \param modelName モデル名
 	* \param objectName オブジェクト名
 	*/
	BackgroundActor(const std::string&         modelName,
				   std::optional<std::string> objectName = std::nullopt);

	BackgroundActor();
	~BackgroundActor() override;

	/**
	 * \brief タイプ名を取得
	 * \return タイプ名
	 */
	std::string_view GetTypeName() const override { return "BackgroundActor"; }
};