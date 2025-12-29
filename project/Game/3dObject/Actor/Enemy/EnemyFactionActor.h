#pragma once

#include <Engine/Objects/3D/Actor/Actor.h>

/*---------------------------------------------------
 * EnemyKind enum class
 * - 敵の種別
 *-------------------------------------------------*/
enum class EnemyKind : uint8_t {
	Normal = 0, //< 通常型
	Boss,		//< ボス型
};

/*-----------------------------------------------------------------------------------------
 * EnemyFactionActor class
 * - 敵勢力のアクター基底クラス
 *---------------------------------------------------------------------------------------*/
class EnemyFactionActor
	: public Actor {
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
	 * \param kind 敵種別
	 */
	EnemyKind GetEnemyKind() const;
	/**
	 * \brief 撃破スコアを取得
	 * \return 撃破スコア
	 */
	int32_t GetKillScore() const ;

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
	 * \brief 撃破スコアを設定
	 * \param score 撃破スコア
	 */
	void SetKillScore(int32_t score);
	/**
	 * \brief 撃破スコアをスコアサービスに通知
	 */
	void PublishKillScore()const;

private:
	//===================================================================*/
	//                    private members
	//===================================================================*/
	EnemyKind kind_		 = EnemyKind::Normal; //< 敵種別
	int32_t	  killScore_ = 0;				  //< 撃破スコア
};
