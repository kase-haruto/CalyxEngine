#pragma once

/*-----------------------------------------------------------------------------------------
 * ILockonTarget
 * - ロックオン対象インターフェース
 * - ロックオン可能なオブジェクトが実装するインターフェース
 *---------------------------------------------------------------------------------------*/
class ILockonTarget {
public:
	enum class LockedState {
		NotLocked, //< ロックされていない状態
		Locked,    //< ロックされている状態
	};

public:
	//=====================================================================
	// Public Methods
	//=====================================================================
	virtual ~ILockonTarget() = default;
	/**
	 * \brief ロック状態を設定
	 * \param state ロック状態
	 */
	virtual void SetLockedState(LockedState state) = 0;
	/**
	 * \brief ロック状態を取得
	 * \return ロック状態
	 */
	virtual LockedState GetLockedState() const = 0;
	/**
	 * \brief ロックオンされているか
	 * \return ロックオンされていればtrue
	 */
	virtual bool IsLocked() const { return GetLockedState() == LockedState::Locked; }
	/**
	 * \brief ロック状態を取得
	 */
	virtual void OnLoced() {}
	/**
	 * \brief ロック解除状態を取得
	 */
	virtual void OnUnloced() {}
};