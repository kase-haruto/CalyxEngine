#pragma once
#include <string>

// engine
#include <Engine/Foundation/Serialization/SerializableObject.h>

/*-----------------------------------------------------------------------------------------
 * IBossAttackParam
 * - ボスの攻撃パラメータ基底クラス
 * - 各種攻撃パラメータクラスはこのクラスを継承して実装する
 *---------------------------------------------------------------------------------------*/
struct IBossAttackParam :
	public CalyxEngine::SerializableObject {
	IBossAttackParam();
	CalyxEngine::ParamPath GetParamPath() const override;

	void SetAttackName(const std::string& name){attackName = name;}

public:
	float cooldown = 0.0f;

private:
	//=========================================================
	// private members
	//=========================================================
	std::string attackName            = "BaseAttack";
	std::string serializeSubRootPath_ = "Actor/Boss/Attack";
};

/*-----------------------------------------------------------------------------------------
 * IBossAttack
 * - ボスの攻撃インターフェースクラス
 * - 各種攻撃クラスはこのクラスを継承して実装する
 *---------------------------------------------------------------------------------------*/
class IBossAttack {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	IBossAttack();
	virtual ~IBossAttack();

	/**
	 * \brief 攻撃の実行が可能かを返す
	 * \return 実行可能ならtrue、不可能ならfalse
	 */
	virtual bool Execute(class Boss& boss,class BossShootingController& shootingController) const;
	/**
	 * \brief GUI表示
	 */
	virtual void ShowGui() = 0;

protected:
	void SetAttackName(const std::string& name) { attackName_ = name; }
	const std::string& GetAttackName() const { return attackName_; }

private:
	//=========================================================
	// private members
	//=========================================================
	std::string attackName_ = "BaseAttack";
};