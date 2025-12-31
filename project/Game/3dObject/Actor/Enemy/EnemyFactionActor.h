#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>
// engine
#include "Details/EnemyKind.h"
#include <Engine/Foundation/Serialization/SerializableObject.h>

/*-----------------------------------------------------------------------------------------
 * EnemyFactionActor class
 * - 敵勢力のアクター基底クラス
 *---------------------------------------------------------------------------------------*/
class EnemyFactionActor
	: public Actor,
	  public CalyxEngine::SerializableObject {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	/** \brief コンストラクタ*/
	EnemyFactionActor();
	EnemyFactionActor(const std::string& modelName, const std::string& objName);
	virtual ~EnemyFactionActor() override;
	/**
	 * \brief 敵の種別を設定
	 */
	EnemyKind GetEnemyKind() const;
	/**
	 * \brief 撃破スコアを取得
	 * \return 撃破スコア
	 */
	int32_t GetKillScore() const;
	/**
	 * \brief パラメータパスの取得
	 * \return パラメータパス
	 */
	CalyxEngine::ParamPath GetParamPath() const override;

protected:
	//===================================================================*/
	//                    protected methods
	//===================================================================*/
	/**
	 * \brief 敵の種別を設定
	 * \param kind 敵種別
	 */
	void SetEnemyKind(EnemyKind kind);
	/**
	 * \brief 撃破スコアをスコアサービスに通知
	 */
	void PublishKillScore() const;

protected:
	//===================================================================*/
	//                    protected methods
	//===================================================================*/
	int32_t killScore_ = 0; //< 撃破スコア

private:
	//===================================================================*/
	//                    private members
	//===================================================================*/
	EnemyKind kind_ = EnemyKind::Normal; //< 敵種別
};
