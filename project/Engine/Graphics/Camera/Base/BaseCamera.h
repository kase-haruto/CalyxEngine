// BaseCamera.h
#pragma once

#include "ICamera.h"
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>
#include <Engine/Graphics/Buffer/CameraBuffer.h>

#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
/* lib */
#include <numbers>

#include <d3d12.h>
#include <wrl.h>

struct CalyxEngine::Matrix4x4;
struct CalyxEngine::Vector3;

/*-----------------------------------------------------------------------------------------
 * BaseCamera
 * - カメラ基底クラス
 * - ビュー・プロジェクション行列の計算、カメラシェイク、定数バッファ管理を提供
 *---------------------------------------------------------------------------------------*/
/**
 * @brief BaseCameraの機能を提供するクラスです。
 */
class CALYX_API BaseCamera :
	public ICamera,
	public IConfigurable {
public:
	//==================================================================*//
	//			public functions
	//==================================================================*//
	/** \brief 既定名と既定投影設定でカメラを構築する */
	BaseCamera();
	/** \brief 指定名と既定投影設定でカメラを構築する \param name Scene上で識別するカメラ名 */
	BaseCamera(const std::string& name);
	/** \brief カメラが所有する定数バッファと設定を解放する */
	virtual ~BaseCamera() = default;

	/** \brief カメラシェイクと行列をフレーム更新する \param dt 前フレームからの経過時間（秒） */
	virtual void Update(float dt) override;
	/** \brief 再生停止状態に関係なく必要なカメラ処理を更新する \param dt 前フレームからの経過時間（秒） */
	virtual void AlwaysUpdate(float dt) override;

	/** \brief カメラ設定を編集するEditor GUIを表示する */
	void ShowImGui();
	/** \brief Transformと投影設定からビュー・プロジェクション行列を再計算する */
	virtual void UpdateMatrix();

	/** \brief パイプライン種別に応じたカメラ定数をコマンドリストへ設定する \param command 描画命令を記録するコマンドリスト \param pipelineType 設定先を決めるパイプライン種別 */
	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command,
					PipelineType                                      pipelineType);
	/** \brief パイプライン種別に応じたカメラ定数をコマンドリストへ設定する \param command 所有権を持たない描画コマンドリスト \param pipelineType 設定先を決めるパイプライン種別 */
	void SetCommand(ID3D12GraphicsCommandList* command, PipelineType pipelineType);
	/** \brief 指定ルートパラメータへカメラ定数バッファを設定する \param command 所有権を持たない描画コマンドリスト \param rootIndex 設定先のルートパラメータ番号 */
	void SetRootCommand(ID3D12GraphicsCommandList* command, uint32_t rootIndex) const;
	/** \brief カメラ距離ディザリングの有効状態をGPU定数へ設定する \param enabled 有効にする場合はtrue */
	void SetCameraDitherEnabled(bool enabled);

	/** \brief 指定時間と強度でカメラシェイクを開始する \param duration シェイク継続時間（秒） \param intensity 最大位置変位量 */
	void StartShake(float duration,float intensity) override;

	/** \brief リフレクションで使用するクラス名を取得する \return BaseCameraを示す固定文字列 */
	std::string_view GetObjectClassName() const override { return "BaseCamera"; }
	// config ============================================================
	/** \brief 保持するSceneObjectConfigをRuntimeカメラへ反映する */
	void ApplyConfig();
	/** \brief 現在のRuntimeカメラ状態をSceneObjectConfigへ抽出する */
	void ExtractConfig();
	/** \brief JSONからカメラ設定を復元する \param j SceneまたはPrefabから読み込んだJSON */
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	/** \brief 現在のカメラ設定をJSONへ保存する \param j 保存結果を書き込むJSON */
	void ExtractConfigToJson(nlohmann::json& j) const override;

	/** \brief Scene上で使用するカメラ種別名を取得する \return 現在のカメラ名 */
	std::string GetObjectTypeName() const override { return name_; }

protected:
	//==================================================================*//
	//			protected functions
	//==================================================================*//
	/** \brief 左手座標系の透視投影行列を生成する \param fovY 垂直視野角（ラジアン） \param aspectRatio 画面の横幅と高さの比 \param nearClip 近クリップ距離 \param farClip 遠クリップ距離 \return 透視投影行列 */
	CalyxEngine::Matrix4x4 MakePerspectiveFovMatrix(float fovY,float aspectRatio,float nearClip,float farClip);
	/** \brief Scene上で識別するカメラ名を設定する \param name 新しいカメラ名 */
	void                 SetName(const std::string& name);

public:
	//==================================================================*//
	//			getter / setter
	//==================================================================*//
	/** \brief カメラの座標とEuler回転をまとめて設定する \param pos ワールド座標 \param rotate Euler回転（ラジアン） */
	void SetCamera(const CalyxEngine::Vector3& pos,const CalyxEngine::Vector3& rotate);

	/** \brief ビュー行列を取得する \return カメラが保持するビュー行列 */
	const CalyxEngine::Matrix4x4& GetViewMatrix() const;
	/** \brief プロジェクション行列を取得する \return カメラが保持する投影行列 */
	const CalyxEngine::Matrix4x4& GetProjectionMatrix() const;
	/** \brief ビュー・プロジェクション行列を取得する \return 描画に使用する合成行列 */
	const CalyxEngine::Matrix4x4& GetViewProjectionMatrix() const;
	/** \brief カメラのEuler回転を取得する \return Euler回転（ラジアン） */
	const CalyxEngine::Vector3&   GetRotate() const;
	/** \brief カメラのワールド座標を取得する \return ワールド座標 */
	const CalyxEngine::Vector3&   GetTranslate() const;
	/** \brief 垂直視野角を取得する \return 垂直視野角（ラジアン） */
	float                       GetFovY() const { return fovAngleY_; }
	/** \brief 垂直視野角を設定する \param radians 垂直視野角（ラジアン） */
	void                        SetFovY(float radians) { fovAngleY_ = radians; }
	/** \brief 現在のアスペクト比を取得する \return 画面の横幅と高さの比 */
	float                       GetAspectRatio() const { return aspectRatio_; }
	/** \brief カメラが描画対象として有効か判定する \return 有効な場合はtrue */
	bool                        IsActive() const { return isActive_; }
	/** \brief カメラの有効状態を設定する \param isActive 有効にする場合はtrue */
	void                        SetActive(bool isActive) { isActive_ = isActive; }
	/** \brief ビューポートに合わせてアスペクト比を設定する \param aspect 画面の横幅と高さの比 */
	void                        SetAspectRatio(float aspect) override;
	/** \brief 近クリップ距離を取得する \return 近クリップ距離 */
	float                       GetNear() const { return nearZ_; }

protected:
	//==================================================================*//
	//			protected variables
	//==================================================================*//

	CalyxEngine::Matrix4x4 viewMatrix_;       //< Transformから計算したビュー行列
	CalyxEngine::Matrix4x4 projectionMatrix_; //< 透視投影設定から計算したプロジェクション行列

	float aspectRatio_ = 16.0f / 9.0f;                                          //< 画面の横幅と高さの比
	float nearZ_       = 0.1f;                                                  //< 近クリップ距離
	float farZ_        = 500.0f;                                                //< 遠クリップ距離
	float fovAngleY_   = 75.0f * static_cast<float>(std::numbers::pi) / 180.0f; //< 垂直視野角（ラジアン）

protected:
	// カメラシェイク関連
	bool isShaking_ = false;                       //< カメラシェイクを更新中か
	float shakeDuration_ = 0.0f;                   //< シェイクの総継続時間（秒）
	float shakeElapsed_ = 0.0f;                    //< シェイク開始からの経過時間（秒）
	float shakeIntensity_ = 0.0f;                  //< シェイク中の最大位置変位量
	CalyxEngine::Vector3 originalPosition_;        //< シェイク終了時に復元する元のカメラ座標

protected:
	//==================================================================*//
	//			protected variables
	//==================================================================*//
	bool isActive_ = true;                        //< 描画に使用可能なカメラか
	CalyxEngine::Matrix4x4 viewProjectionMatrix_; //< ビュー行列と投影行列の合成結果

private:
	//==================================================================*//
	//			private variables
	//==================================================================*//
	Camera3DBuffer cameraBuffer_; //< 描画時にカメラ情報を転送するGPU定数バッファ

	ConfigurableObject<SceneObjectConfig> config_; //< SceneとPrefabに保存するカメラ設定
};
