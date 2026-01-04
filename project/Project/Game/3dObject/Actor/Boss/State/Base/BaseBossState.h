#pragma once

#include "../../Details/BossStateType"

#include <string>

class Boss;

/**
 * \brief ボスの状態インターフェース
 */
class BaseBossState {
public:
	//===================================================================*/
	//                    structs
	//===================================================================*/
	struct TransitionRequest {
		enum class Type { None, Change, Push, Pop };

		Type op = Type::None;
		BossStateType nextType;
		int16_t param = 0;
	};

public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	BaseBossState();
	virtual ~BaseBossState();

	/**
	 * \brief 状態に入るときの処理
	 */
	virtual void Enter() {}
	/**
	 * \brief 状態から出るときの処理
	 */
	virtual void Exit() {}
	/**
	 * \brief 状態の更新
	 */
	virtual void Update(float dt) = 0;
	/**
	 * \brief GUI表示
	 */
	virtual void ShowGui();
	/**
	 * \brief 遷移リクエストのリセット
	 */
	void ResetRequest();

	// accessor ==========================================================//
	const TransitionRequest& GetTransitionRequest() const;
	BossStateType GetStateType() const;
	std::string GetStateName() const;
	void SetStatypeType(BossStateType type);
	void SetOwner(Boss* owner);

	// 遷移時のパラメータ
	int16_t GetTransitionParam() const { return transitionParm_; }
	void SetTransitionParam(int16_t parm);
protected:
	//===================================================================*/
	//                    protected methods
	//===================================================================*/
	/**
	 * \brief 状態遷移の要求
	 * \param next 次の状態の種類
	 * \param parm 遷移先に送るパラメータ（デフォルト：0）
	 */
	void RequestChange(BossStateType next, int16_t parm = 0);
	/**
	 * \brief 状態を上に積む要求
	 * \param next 次の状態の種類
	 */
	void RequestPush(BossStateType next);
	/**
	 * \brief 状態の破棄要求
	 */
	void RequestPop();

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	TransitionRequest request_;	//< 遷移リクエスト
	BossStateType state_;		//< ステートタイプ
	int16_t transitionParm_ = 0;	//< 遷移先に送るパラメータ

protected:
	Boss* owner_ = nullptr;
};
