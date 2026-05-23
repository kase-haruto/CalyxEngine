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
     * \brief 初期化
     */
    void Initialize();
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
	void GetShadowFrustumCorners(CalyxEngine::Vector3 outCorners[8], float shadowFar) const;
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

    //--------- follow target -------------------------------------------------
    /**
     * \brief 追従対象を設定
     * \param wt ターゲットのトランスフォーム
     */
    void SetFollowTarget(const WorldTransform* wt) { follow_.target = wt; }
    /**
     * \brief 追従対象を取得
     * \return 追従対象
     */
    const WorldTransform* GetFollowTarget() const { return follow_.target; }
    /**
     * \brief 追従の有効/無効を設定
     * \param e 有効か
     */
    void EnableFollow(bool e) { follow_.enabled = e; }
    /**
     * \brief 追従が有効か
     * \return 有効か
     */
    bool IsFollowEnabled() const { return follow_.enabled; }
    /**
     * \brief 前方ベクトルを取得
     * \return 前方ベクトル
     */
	CalyxEngine::Vector3 GetForward()const;

private:
    //==================================================================*//
    //          private functions
    //==================================================================*//
    void UpdateFollow(float dt);

    // ベクトル版 SmoothDamp（Unity 近似）
    static CalyxEngine::Vector3 SmoothDampVec(const CalyxEngine::Vector3& current,
                                 const CalyxEngine::Vector3& target,
                                 CalyxEngine::Vector3& currentVelocity,
                                 float smoothTime, float dt);

    // 回転の指数補間率（0..1）
    static float ExpLerpAlpha(float dt, float tau);

private:
	//==================================================================*//
	//          private variables
	//==================================================================*//
	Frustum frustum_; // 視錐台

	//======================= 追従用データ ==============================
	struct FollowSettings : public CalyxEngine::SerializableObject {
		FollowSettings();
		CalyxEngine::ParamPath GetParamPath() const override;

		bool   enabled          = true;            // 有効/無効
		float  distanceBack     = 13.0f;             // 後方距離（-F * distanceBack）
		float  heightOffset     = 4.0f;             // 上方向(Y)オフセット
		float  sideOffset       = 0.0f;             // 右(+)左(-)オフセット
		CalyxEngine::Vector3 lookAtOffset    = {0.0f, 1.5f, 0.0f}; // 必要なら使用

		// 位置スムージング
		float  posSmoothTime    = 0.78f;
		// 回転スムージング（時定数）
		float  rotTimeConstant  = 0.52f;

		// 俯角（ターゲットの forward を向きつつ少し下を見る）
		float  extraPitchDeg    = -10.0f;

		const WorldTransform* target = nullptr;     // 追従対象
		CalyxEngine::Vector3 posVel = {0,0,0};                   // SmoothDamp 用速度
	} follow_;
};