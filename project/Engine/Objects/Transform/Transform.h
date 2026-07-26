#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>

#include <Engine/Graphics/Buffer/DxConstantBuffer.h>

#include <Data/Engine/Configs/Scene/Objects/Transform/UvTransformConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Transform/WorldTransformConfig.h>

// math
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>

// c++
#include <cstdint>
#include <string>

/*-----------------------------------------------------------------------------------------
 * RotationSource
 * - Transformの回転計算に使用する入力表現を示す列挙型
 * - Editorで編集されたEuler角とRuntimeのQuaternionの優先関係を管理
 *---------------------------------------------------------------------------------------*/
enum class RotationSource {
	Euler,
	Quaternion
};

/*-----------------------------------------------------------------------------------------
 * TransformationMatrix
 * - TransformからGPUへ転送する行列を保持する定数バッファ用データ構造
 * - ワールド行列と法線変換用の逆転置行列を管理
 *---------------------------------------------------------------------------------------*/
struct TransformationMatrix {
	CalyxEngine::Matrix4x4 world				   = CalyxEngine::Matrix4x4::MakeIdentity(); //< ワールド行列
	CalyxEngine::Matrix4x4 WorldInverseTranspose = CalyxEngine::Matrix4x4::MakeIdentity(); //< ワールド逆転置行列
};

/*-----------------------------------------------------------------------------------------
 * EulerTransform
 * - オイラー角ベースのトランスフォーム構造体
 *---------------------------------------------------------------------------------------*/
struct EulerTransform {
	CalyxEngine::Vector3 scale;	  //< スケール
	CalyxEngine::Vector3 rotate;	  //< 回転(オイラー角)
	CalyxEngine::Vector3 translate; //< 座標

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
	void ShowImGui(const std::string& lavel = "Transform", bool defaultOpen = true);
};

/*-----------------------------------------------------------------------------------------
 * Transform2D
 * - 2D空間のトランスフォーム構造体
 *---------------------------------------------------------------------------------------*/
struct Transform2D {
	CalyxEngine::Vector2 scale;	  //< スケール
	float			   rotate;	  //< 回転
	CalyxEngine::Vector2 translate; //< 座標

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
	CalyxEngine::Matrix4x4 GetMatrix() const;
	/**
	 * \brief ImGui表示
	 * \param lavel ラベル名
	 */
	void ShowImGui(const std::string& lavel = "Transform", bool defaultOpen = true);
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
	void ShowImGui(Transform2DConfig& config, const std::string& lavel = "Transform", bool defaultOpen = true);
	/**
	 * \brief コンフィグを適用
	 * \param config コンフィグ
	 */
	void ApplyConfig(const Transform2DConfig& config);
};

/*-----------------------------------------------------------------------------------------
 * QuaternionTransform
 * - Quaternion回転を使用する基本的な3D配置を保持するデータ構造
 * - 拡大率、回転、座標をまとめて管理
 *---------------------------------------------------------------------------------------*/
struct QuaternionTransform {
	CalyxEngine::Vector3	  scale;	 //< スケール
	CalyxEngine::Quaternion rotate;	 //< 回転(クォータニオン)
	CalyxEngine::Vector3	  translate; //< 座標
};

/*-----------------------------------------------------------------------------------------
 * BaseTransform
 * - トランスフォーム基底クラス
 * - 定数バッファとの同期、親子関係の管理、行列計算を行う
 *---------------------------------------------------------------------------------------*/
class CALYX_API BaseTransform : public DxConstantBuffer<TransformationMatrix> {
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
	virtual void Update([[maybe_unused]] const CalyxEngine::Matrix4x4& viewProMatrix) {}
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
	virtual CalyxEngine::Vector3 GetWorldPosition() const;
	/**
	 * \brief 行列が最後に更新された世代番号を取得する
	 * \return Transform変更時に増加する世代番号
	 */
	uint64_t GetRevision() const { return revision_; }

public:
	//========================================================================*/
	//	public variables
	//========================================================================*/
	CalyxEngine::Vector3	  scale;	   //< スケール
	CalyxEngine::Quaternion rotation;	   //< 回転(クォータニオン)
	CalyxEngine::Vector3	  translation; //< 座標

	CalyxEngine::Vector3 eulerRotation; //< 回転(オイラー角)

	TransformationMatrix matrix;		   //< 行列データ
	BaseTransform*		 parent = nullptr; //< 親トランスフォーム

	RotationSource rotationSource = RotationSource::Quaternion; //< 回転ソース

protected:
	uint64_t revision_ = 1; //< 行列が再計算された世代
};

/*-----------------------------------------------------------------------------------------
 * WorldTransform
 * - ワールド空間のトランスフォームクラス
 *---------------------------------------------------------------------------------------*/
class CALYX_API WorldTransform : public BaseTransform {
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
	virtual void Update(const CalyxEngine::Matrix4x4& viewProMatrix) override;

	/**
	 * \brief 更新処理
	 */
	void Update() override;

	/**
	 * \brief 継承設定を考慮した親行列を取得
	 * \return 親行列
	 */
	CalyxEngine::Matrix4x4 GetEffectiveParentMatrix() const;

	/**
	 * \brief 前方ベクトルを取得
	 * \return 前方ベクトル
	 */
	CalyxEngine::Vector3 GetForward() const;

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

private:
	/**
	 * \brief 現在値と親Transformの世代がキャッシュと一致するか判定する
	 * \param parentRevision 親Transformの現在の世代番号
	 * \return 行列の再計算が不要な場合はtrue
	 */
	bool IsCacheValid(uint64_t parentRevision) const;
	/**
	 * \brief 行列計算に使用した入力値をキャッシュへ保存する
	 * \param parentRevision 親Transformの現在の世代番号
	 */
	void StoreCache(uint64_t parentRevision);

	CalyxEngine::Vector3 cachedScale_{};                  //< 前回行列計算時の拡大率
	CalyxEngine::Quaternion cachedRotation_{};            //< 前回行列計算時のQuaternion回転
	CalyxEngine::Vector3 cachedEulerRotation_{};          //< 前回行列計算時のEuler回転
	CalyxEngine::Vector3 cachedTranslation_{};            //< 前回行列計算時の座標
	BaseTransform* cachedParent_ = nullptr;               //< 所有権を持たない前回計算時の親Transform
	uint64_t cachedParentRevision_ = 0;                   //< 前回計算時の親Transform世代番号
	RotationSource cachedRotationSource_ = RotationSource::Quaternion; //< 前回計算時の回転入力表現
	bool cachedInheritTranslate_ = true;                  //< 前回計算時の座標継承設定
	bool cachedInheritRotate_ = true;                     //< 前回計算時の回転継承設定
	bool cachedInheritScale_ = true;                      //< 前回計算時の拡大率継承設定
	bool cacheValid_ = false;                             //< キャッシュが一度以上構築されているか
};

//============================================================================*/
//	json serialization
//============================================================================*/
