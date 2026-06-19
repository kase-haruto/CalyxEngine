#pragma once
/* ========================================================================
/*          include space
/* ===================================================================== */
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Graphics/Camera/Frustum/Frustum.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Foundation/Math/Vector3.h>

// fwd
class WorldTransform;

/*-----------------------------------------------------------------------------------------
 * Camera3d
 * - メインカメラクラス
 * - 追従機能、視錐台カリング、定数バッファの更新などを計算する
 *---------------------------------------------------------------------------------------*/
class Camera3d : public BaseCamera {
public:
	//==================================================================*//
	//          public functions
	//==================================================================*//
	/**
	 * \brief コンストラクタ
	 */
	Camera3d();
	/**
	 * \brief コンストラクタ
	 * \param name カメラ名
	 */
	Camera3d(const std::string& name);
	/**
	 * \brief デストラクタ
	 */
	~Camera3d() override = default;

	/**
	 * \brief 常に実行される更新処理
	 * \param dt デルタタイム
	 */
	void AlwaysUpdate(float dt) override;
	/**
	 * \brief GUI表示
	 */
	void ShowGui() override;
	/**
	 * \brief シャドウ用視錐台の四隅を取得
	 * \param outCorners 出力先
	 * \param shadowFar 遠方距離
	 */
	void GetShadowFrustumCorners(CalyxEngine::Vector3 outCorners[8],float shadowFar) const;
	//--------- accessor -----------------------------------------------------
	/**
	 * \brief AABBが視野内か
	 * \param aabb 判定対象
	 * \return 視野内か
	 */
	bool IsVisible(const class AABB& aabb) const;
	/**
	 * \brief タイプ名を取得
	 * \return タイプ名
	 */
	std::string_view GetObjectClassName() const override { return "Camera3d"; }
	/**
     * \brief 前方ベクトルを取得
     * \return 前方ベクトル
     */
	CalyxEngine::Vector3 GetForward() const;

private:
	//==================================================================*//
	//          private functions
	//==================================================================*//
	// ベクトル版 SmoothDamp（Unity 近似）
	static CalyxEngine::Vector3 SmoothDampVec(const CalyxEngine::Vector3& current,
											  const CalyxEngine::Vector3& target,
											  CalyxEngine::Vector3&       currentVelocity,
											  float                       smoothTime,float dt);

	// 回転の指数補間率（0..1）
	static float ExpLerpAlpha(float dt,float tau);

private:
	//==================================================================*//
	//          private variables
	//==================================================================*//
	Frustum frustum_; // 視錐台
};