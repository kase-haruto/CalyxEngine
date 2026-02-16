#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>

// forward declaration
struct CalyxMath::Vector3;
struct CalyxMath::Vector2;

/* ========================================================================
/*			デバッグ用カメラ
/* ===================================================================== */
class DebugCamera
	: public Camera3d {
public:
	//===================================================================//
	//							public メソッド
	//===================================================================//
	DebugCamera() = default;
	DebugCamera(const std::string& name);
	~DebugCamera() = default;

	void AlwaysUpdate(float dt) override;
	;
	void ShowGui() override; //< ImGuiによるGUI表示

	//===================================================================//
	//							public アクセッサ
	//===================================================================//
	//* ターゲット（注視点）を設定
	void SetTarget(const CalyxMath::Vector3& target) { target_ = target; }

	//* カメラとターゲットとの初期距離を設定
	void SetDistance(float dist) { distance_ = dist; }
	void SetInputEnabled(bool enable) { isInputEnabled_ = enable; }

	std::string_view GetTypeName() const override { return "DebugCamera"; }

private:
	//===================================================================//
	//							private メソッド
	//===================================================================//
	void Move();   //< パン処理
	void Rotate(); //< 回転処理(Orbit)
	void Zoom();   //< ズーム処理

	//===================================================================//
	//							private 変数
	//===================================================================//
	CalyxMath::Vector3 target_{0.0f,0.0f,0.0f}; //* ターゲット（注視点）
	float   distance_ = 10.0f;       //* カメラまでの距離
	CalyxMath::Vector2 orbitAngle_{0.0f,0.0f};  //* オービット時の回転角度(Yaw, Pitch)

	// 操作速度
	float rotateSpeed_ = 0.005f; //* 回転速度
	float panSpeed_    = 1.0f;  //* パンスピード
	float zoomSpeed_   = 1.0f;   //* ズーム速度

	// ドラッグ状態の管理（Rotate用）
	CalyxMath::Vector2 lastMousePosRotate_{0.0f,0.0f}; //* Rotate用の前フレームのマウス位置
	bool    isDraggingRotate_{false};       //* Rotateがドラッグ中かどうか

	// ドラッグ状態の管理（Move用）
	CalyxMath::Vector2 lastMousePosMove_{0.0f,0.0f}; //* Move用の前フレームのマウス位置
	bool    isDraggingMove_{false};       //* Moveがドラッグ中かどうか
	bool	isInputEnabled_{true};
};