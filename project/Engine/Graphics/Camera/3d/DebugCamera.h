#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>

// forward declaration
struct CalyxEngine::Vector3;
struct CalyxEngine::Vector2;
struct CalyxEngine::Quaternion;

/* ========================================================================
/*			デバッグ用カメラ
/* ===================================================================== */
class DebugCamera
	: public Camera3d {
public:
	struct State {
		CalyxEngine::Vector3 target{0.0f, 0.0f, 0.0f};
		float distance = 10.0f;
		CalyxEngine::Vector2 orbitAngle{0.0f, 0.0f};
		float rotateSpeed = 0.005f;
		float panSpeed = 0.1f;
		float zoomSpeed = 0.1f;
		CalyxEngine::Vector3 translation{0.0f, 4.0f, -10.0f};
		CalyxEngine::Vector3 eulerRotation{0.0f, 0.0f, 0.0f};
	};

	//===================================================================//
	//							public メソッド
	//===================================================================//
	DebugCamera() = default;
	DebugCamera(const std::string& name);
	~DebugCamera() = default;
	void Initialize() override;
	void AlwaysUpdate(float dt) override;
	void ShowGui() override; //< ImGuiによるGUI表示

	//===================================================================//
	//							public アクセッサ
	//===================================================================//
	//* ターゲット（注視点）を設定
	void SetTarget(const CalyxEngine::Vector3& target) { target_ = target; }

	//* カメラとターゲットとの初期距離を設定
	void SetDistance(float dist) { distance_ = dist; }
	void SetInputEnabled(bool enable) { isInputEnabled_ = enable; }
	void CopyStateFrom(const DebugCamera& other);
	State CaptureState() const;
	void ApplyState(const State& state);

	std::string_view GetObjectClassName() const override { return "DebugCamera"; }

private:
	//===================================================================//
	//							private メソッド
	//===================================================================//
	void Move();   //< パン処理
	void Rotate(); //< 回転処理(Orbit)
	void Zoom();   //< ズーム処理
	CalyxEngine::Vector3 CalcOrbitOffset() const;
	void SyncOrbitFromTransform();
	void ApplyOrbitToTransform();
	bool IsTransformChangedExternally() const;
	void StoreAppliedTransform();

	//===================================================================//
	//							private 変数
	//===================================================================//
	CalyxEngine::Vector3 target_{0.0f,0.0f,0.0f}; //* ターゲット（注視点）
	float   distance_ = 10.0f;       //* カメラまでの距離
	CalyxEngine::Vector2 orbitAngle_{0.0f,0.0f};  //* オービット時の回転角度(Yaw, Pitch)

	// 操作速度
	float rotateSpeed_ = 0.005f; //* 回転速度
	float panSpeed_    = 0.1f;  //* パンスピード
	float zoomSpeed_   = 0.1f;   //* ズーム速度

	// ドラッグ状態の管理（Rotate用）
	CalyxEngine::Vector2 lastMousePosRotate_{0.0f,0.0f}; //* Rotate用の前フレームのマウス位置
	bool    isDraggingRotate_{false};       //* Rotateがドラッグ中かどうか

	// ドラッグ状態の管理（Move用）
	CalyxEngine::Vector2 lastMousePosMove_{0.0f,0.0f}; //* Move用の前フレームのマウス位置
	bool    isDraggingMove_{false};       //* Moveがドラッグ中かどうか
	bool	isInputEnabled_{true};
	CalyxEngine::Vector3 lastAppliedTranslation_{0.0f,0.0f,0.0f};
	CalyxEngine::Quaternion lastAppliedRotation_{};
	bool	hasAppliedTransform_{false};

	// 設定
	bool isInverse_ = false;
};