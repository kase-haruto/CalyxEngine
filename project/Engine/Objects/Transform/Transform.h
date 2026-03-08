#pragma once

#include <Engine/Graphics/Buffer/DxConstantBuffer.h>

#include <Data/Engine/Configs/Scene/Objects/Transform/UvTransformConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Transform/WorldTransformConfig.h>

// math
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>

// c++
#include <string>

enum class RotationSource {
	Euler,
	Quaternion
};

struct TransformationMatrix {
	CalyxMath::Matrix4x4 world				   = CalyxMath::Matrix4x4::MakeIdentity(); //< ワールド行列
	CalyxMath::Matrix4x4 WorldInverseTranspose = CalyxMath::Matrix4x4::MakeIdentity(); //< ワールド逆転置行列
};

/*-----------------------------------------------------------------------------------------
 * EulerTransform
 * - オイラー角ベースのトランスフォーム構造体
 *---------------------------------------------------------------------------------------*/
struct EulerTransform {
	CalyxMath::Vector3 scale;	  //< スケール
	CalyxMath::Vector3 rotate;	  //< 回転(オイラー角)
	CalyxMath::Vector3 translate; //< 座標

	/**
	 * \brief 初期化
	 */
	void Initialize() {
		scale	  = {1.0f, 1.0f, 1.0f};
		rotate	  = {0.0f, 0.0f, 0.0f};
		translate = {0.0f, 0.0f, 0.0f};
	}

	/**
	 * \brief ImGui表示
	 * \param lavel ラベル名
	 */
	void ShowImGui(const std::string& lavel = "Transform");
};

/*-----------------------------------------------------------------------------------------
 * Transform2D
 * - 2D空間のトランスフォーム構造体
 *---------------------------------------------------------------------------------------*/
struct Transform2D {
	CalyxMath::Vector2 scale;	  //< スケール
	float			   rotate;	  //< 回転
	CalyxMath::Vector2 translate; //< 座標

	/**
	 * \brief 初期化
	 */
	void Initialize() {
		scale	  = {1.0f, 1.0f};
		rotate	  = 0.0f;
		translate = {0.0f, 0.0f};
	}
	/**
	 * \brief 行列を取得
	 * \return 行列
	 */
	CalyxMath::Matrix4x4 GetMatrix() const;
	/**
	 * \brief ImGui表示
	 * \param lavel ラベル名
	 */
	void ShowImGui(const std::string& lavel = "Transform");
	/**
	 * \brief コンフィグを抽出
	 * \return コンフィグ
	 */
	Transform2DConfig ExtractConfig() const;
	/**
	 * \brief コンフィグ同期付きImGui表示
	 * \param config コンフィグ
	 * \param lavel ラベル名
	 */
	void ShowImGui(Transform2DConfig& config, const std::string& lavel = "Transform");
	/**
	 * \brief コンフィグを適用
	 * \param config コンフィグ
	 */
	void ApplyConfig(const Transform2DConfig& config);
};

struct QuaternionTransform {
	CalyxMath::Vector3	  scale;	 //< スケール
	CalyxMath::Quaternion rotate;	 //< 回転(クォータニオン)
	CalyxMath::Vector3	  translate; //< 座標
};

/*-----------------------------------------------------------------------------------------
 * BaseTransform
 * - トランスフォーム基底クラス
 * - 定数バッファとの同期、親子関係の管理、行列計算を行う
 *---------------------------------------------------------------------------------------*/
class BaseTransform : public DxConstantBuffer<TransformationMatrix> {
public:
	//========================================================================*/
	//	public functions
	//========================================================================*/
	/**
	 * \brief コンストラクタ
	 */
	BaseTransform() = default;
	/**
	 * \brief デストラクタ
	 */
	virtual ~BaseTransform() = default;

	//--------- main -----------------------------------------------------
	/**
	 * \brief 初期化
	 */
	virtual void Initialize();
	/**
	 * \brief 更新処理 (ビュープロジェクション行列を考慮)
	 * \param viewProMatrix ビュープロジェクション行列
	 */
	virtual void Update([[maybe_unused]] const CalyxMath::Matrix4x4& viewProMatrix) {}
	/**
	 * \brief 更新処理
	 */
	virtual void Update() {}
	/**
	 * \brief コマンドをセット
	 * \param commandList コマンドリスト
	 * \param rootParameterIndex ルートパラメータのインデックス
	 */
	virtual void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
							UINT											  rootParameterIndex) const {
		DxBuffer::SetCommand(commandList, rootParameterIndex);
	};

	//--------- ImGui ---------------------------------------------------
	/**
	 * \brief ImGui表示
	 * \param lavel ラベル名
	 */
	virtual void ShowImGui(const std::string& lavel = "Transform");

	//--------- accessor -------------------------------------------------
	/**
	 * \brief ワールド座標を取得
	 * \return ワールド座標
	 */
	virtual CalyxMath::Vector3 GetWorldPosition() const;

public:
	//========================================================================*/
	//	public variables
	//========================================================================*/
	CalyxMath::Vector3	  scale;	   //< スケール
	CalyxMath::Quaternion rotation;	   //< 回転(クォータニオン)
	CalyxMath::Vector3	  translation; //< 座標

	CalyxMath::Vector3 eulerRotation; //< 回転(オイラー角)

	TransformationMatrix matrix;		   //< 行列データ
	BaseTransform*		 parent = nullptr; //< 親トランスフォーム

	RotationSource rotationSource = RotationSource::Quaternion; //< 回転ソース
};

/*-----------------------------------------------------------------------------------------
 * WorldTransform
 * - ワールド空間のトランスフォームクラス
 *---------------------------------------------------------------------------------------*/
class WorldTransform : public BaseTransform {
public:
	//========================================================================*/
	//	public functions
	//========================================================================*/
	/**
	 * \brief コンストラクタ
	 */
	WorldTransform() = default;
	/**
	 * \brief デストラクタ
	 */
	~WorldTransform() override = default;

	/**
	 * \brief 更新処理 (ビュープロジェクション行列を考慮)
	 * \param viewProMatrix ビュープロジェクション行列
	 */
	virtual void Update(const CalyxMath::Matrix4x4& viewProMatrix) override;

	/**
	 * \brief 更新処理
	 */
	void Update() override;

	/**
	 * \brief 継承設定を考慮した親行列を取得
	 * \return 親行列
	 */
	CalyxMath::Matrix4x4 GetEffectiveParentMatrix() const;

	/**
	 * \brief 前方ベクトルを取得
	 * \return 前方ベクトル
	 */
	CalyxMath::Vector3 GetForward() const;

	/**
	 * \brief ImGui表示オーバーライド（親への継承設定用）
	 */
	void ShowImGui(const std::string& lavel = "Transform") override;

	//--- コンフィグ同期 ---
	/**
	 * \brief コンフィグを適用
	 * \param config コンフィグ
	 */
	void ApplyConfig(const WorldTransformConfig& config);
	/**
	 * \brief コンフィグを抽出
	 * \return コンフィグ
	 */
	WorldTransformConfig ExtractConfig();

public:
	bool inheritTranslate = true; //< 親の座標を継承するか
	bool inheritRotate	  = true; //< 親の回転を継承するか
	bool inheritScale	  = true; //< 親のスケールを継承するか
};

//============================================================================*/
//	json serialization
//============================================================================*/
