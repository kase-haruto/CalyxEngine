# Calyx Physics

## 概要

`Engine/Physics` は、衝突検出そのものではなく、衝突結果を使った物理応答とキャラクター移動を担当します。

- `PhysicsBody`
  - Static / Kinematic の Body 種別を管理します。
  - 押し戻しを受けるかを管理します。
- `PhysicsSystem`
  - Trigger ではないコライダー同士を押し戻します。
  - Static は動かさず、Kinematic を補正します。
- `CharacterMovementComponent`
  - Unreal Engine の CharacterMovementComponent に寄せたキャラクター歩行層です。
  - `Walking` / `Falling` の移動モードを持ちます。
  - `FindFloor()` で床を探します。
  - 歩行可能な床なら `SnapToFloor()` で床へ吸着します。

## 地面設定

Plane は描画用メッシュなので、地面として使う場合は別途 Static な BoxCollider を配置します。

推奨例:

- Plane 表示面: `y = 0`
- BoxCollider center.y: `-0.1`
- BoxCollider size.y: `0.2`
- PhysicsBody: `Static`
- Collider `Is Trigger`: `false`

## キャラクター設定

キャラクターは CapsuleCollider を推奨します。

- Collider: `CapsuleCollider`
- PhysicsBody: `Kinematic`
- CharacterMovement:
  - `Walkable Floor Angle`: `45`
  - `Floor Probe Distance`: `0.35`
  - `Floor Snap Distance`: `0.2`
  - `Skin Width`: `0.05`

## 現在の制限

現在の `FindFloor()` は Static な BoxCollider への下方向 Probe です。  
厳密な CapsuleCast や移動前 Sweep はまだ実装していません。

今後の拡張候補:

- CapsuleCast
- StepOffset
- SlideAlongSurface
- CurrentFloor のキャッシュ
- 移動床
- Flying / Swimming などの MovementMode
