#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Objects/3D/Geometory/AABB.h>

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
	 * \brief ロックされた時のコールバック
	 */
	virtual void OnLocked() {}

	/**
	 * \brief ロック解除された時のコールバック
	 */
	virtual void OnUnlocked() {}

	/**
	 * \brief 中心座標を取得
	 * \return 中心座標
	 */
	virtual const CalyxMath::Vector3 GetCenterPos() const = 0;

	/**
	 * \brief ワールド座標を取得
	 * \return ワールド座標
	 */
	virtual const CalyxMath::Vector3 GetWorldPosition() const = 0;

	/**
	 * \brief ワールド座標系AABBを取得
	 * \return AABB
	 */
	virtual AABB GetWorldAABB() const = 0;

};