#pragma once
/* ========================================================================
/* include
/* ===================================================================== */
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineJson.h>
#include <Engine/Objects/Transform/Transform.h>


#include <string>
#include <vector>

/*-----------------------------------------------------------------------------------------
 * RailCamera
 * - レール移動カメラクラス
 * - スプライン曲線に沿った移動、向きの制御、バンク（傾き）の計算を担当
 *---------------------------------------------------------------------------------------*/
class RailCamera
	: public BaseCamera {
public:
	/**
	 * \brief コンストラクタ
	 */
	RailCamera();
	/**
	 * \brief コンストラクタ
	 * \param name カメラ名
	 */
	RailCamera(const std::string& name);
	/**
	 * \brief デストラクタ
	 */
	~RailCamera() = default;

	/**
	 * \brief 初期化
	 */
	void Initialize();
	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;
	/**
	 * \brief GUI表示
	 */
	void ShowGui() override;
	/**
	 * \brief 常に実行される更新処理
	 * \param dt デルタタイム
	 */
	void AlwaysUpdate(float dt) override;

	// 位置/姿勢
	/**
	 * \brief 座標を取得
	 * \return 座標
	 */
	CalyxEngine::Vector3 GetPosition();
	/**
	 * \brief 回転を取得
	 * \return 回転（オイラー角）
	 */
	const CalyxEngine::Vector3& GetRotation() const { return worldTransform_.eulerRotation; }
	/**
	 * \brief ワールド変換を取得
	 * \return ワールド変換
	 */
	const WorldTransform& GetWorldTransform() const { return worldTransform_; }

	// スプライン設定API
	/**
	 * \brief スプラインを設定
	 * \param s スプラインデータ
	 */
	void SetSpline(const SplineData& s);
	/**
	 * \brief JSONからスプラインを読み込む
	 * \param path ファイルパス
	 * \return 成功したか
	 */
	bool LoadSplineFromJson(const std::string& path);
	/**
	 * \brief スプラインをクリア
	 */
	void ClearSpline();

	// パラメータ
	/**
	 * \brief 移動速度を設定
	 * \param s 速度（単位: ユニット/秒）
	 */
	void SetSpeed(float s) { speed_ = s; }
	/**
	 * \brief 先読み距離を設定
	 * \param d 距離（向き計算用）
	 */
	void SetLookAhead(float d) { lookAhead_ = d; }
	/**
	 * \brief バンクスケールを設定
	 * \param rad 最大ロール角（ラジアン）
	 */
	void SetBankScale(float rad) { tiltAngle_ = rad; }
	/**
	 * \brief バンクの補間速度を設定
	 * \param spd 補間速度
	 */
	void SetBankLerp(float spd) { tiltLerpSpeed_ = spd; }
	/**
	 * \brief 傾きを直接設定
	 * \param angleRad 目標角度（ラジアン）
	 * \param lerp 補間速度
	 */
	void SetTilt(float angleRad, float lerp = 10.0f);
	/**
	 * \brief ループ設定
	 * \param closed 閉じるか
	 */
	void SetClosed(bool closed);
	/**
	 * \brief 停止比率を設定
	 * \param r 停止比率
	 */
	void SetStopRatio(float r);

	/**
	 * \brief 進捗（0〜1）を取得
	 * \return 進捗
	 */
	float GetT() const;
	/**
	 * \brief 弧長進捗を取得
	 * \return 弧長
	 */
	float GetProgress() const;

	/**
	 * \brief スプラインデータを取得
	 * \return スプラインデータ
	 */
	const SplineData& GetSplineData() const { return spline_; }

	/**
	 * \brief タイプ名を取得
	 * \return タイプ名
	 */
	std::string_view GetObjectClassName() const override { return "RailCamera"; }

private:
	/**
	 * \brief パスから向きとロールを更新
	 * \param dt デルタタイム
	 */
	void UpdateOrientationFromPath(float dt);

private:
	// スプライン
	SplineData spline_;
	int		   arcSamplesPerSeg_ = 32; // 1セグメント間で32分割
	float	   totalLength_		 = 0.0f;

	// 状態
	float traveled_	 = 0.0f;  // 走行弧長（0〜totalLength）
	float speed_	 = 20.0f; // 等速（弧長ベース）
	float lookAhead_ = 2.0f;  // 先読み距離（向き用）

	// ロール（左右傾き）
	float zTiltOffset_	 = 0.0f;  // 現在の傾き
	float targetTilt_	 = 0.0f;  // 目標の傾き（曲率由来）
	float tiltAngle_	 = 0.3f;  // 最大傾き（ラジアン）
	float tiltLerpSpeed_ = 10.0f; // 傾き補間速度

	//
	float stopRatio_ = 0.9f; // 停止
};