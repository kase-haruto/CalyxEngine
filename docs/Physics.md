# Calyx Physics Design

## 目的

このドキュメントは、Calyx の物理処理を `Collision` と分けて設計するための方針をまとめます。

現在の Calyx は、コライダー同士が重なった後に `PhysicsSystem` が押し戻す段階まで対応しています。  
地面を安定して走らせるには、押し戻しに加えて「床判定」「床への吸着」「斜面判定」「滑り移動」が必要です。

## 既存エンジンの考え方

### Unreal Engine

Unreal Engine では、キャラクター移動は通常 `CharacterMovementComponent` が担当します。

地面を走る処理は単なるコライダー押し戻しではなく、以下の情報を使って制御されます。

- 現在の移動モード
- 床として扱える法線か
- 歩ける斜面角度か
- 床までの距離
- 段差を上れるか
- 壁に当たったときに滑るか

つまり、地形のコリジョンは Static な床・壁として存在し、キャラクター側は専用の移動コンポーネントが床を検出して移動を制限します。

### Unity

Unity の `CharacterController` は、Rigidbody ではなくキャラクター移動用の専用コントローラです。

主な設定は以下です。

- `Slope Limit`
  - 登れる斜面角度を制限します。
- `Step Offset`
  - 上れる段差高さを制限します。
- `Skin Width`
  - わずかなめり込みを許容し、引っかかりや jitter を減らします。
- `Radius` / `Height`
  - キャラクターのカプセル形状を表します。

Unity でも、キャラクターはスクリプトで移動量を与え、コントローラが衝突に制限された移動へ変換します。  
地面の Mesh / Box / Terrain Collider は Static な障害物で、キャラクター側が床として認識します。

### Godot

Godot の `CharacterBody3D` は `move_and_slide()` を使ってキャラクター移動を行います。

重要な概念は以下です。

- `is_on_floor()`
  - 最後の移動で床に接触したかを返します。
- `floor_max_angle`
  - 床として扱う最大角度を決めます。
- `floor_snap_length`
  - 下方向に床へ吸着する距離を決めます。
- `get_floor_normal()`
  - 接地している床の法線を取得します。

Godot も、地面コリジョンに対してキャラクターが単純に押し戻されるだけではなく、床として分類し、滑り移動と接地状態を管理します。

## Calyx の現状

現在の Calyx は以下の状態です。

- `CollisionManager`
  - コライダー登録
  - overlap 判定
  - Enter / Stay / Exit 通知
- `PhysicsSystem`
  - Trigger ではないコライダー同士の押し戻し
  - `Static` / `Kinematic` の位置補正
- `PhysicsBody`
  - 押し戻し対象かどうか
  - Body 種別
  - Pushback Ratio

これは「壁にめり込んだら戻す」段階です。  
「地面を走る」には、キャラクター専用の移動解決がまだ不足しています。

## Plane + BoxCollider で通り抜ける理由

Plane は描画用メッシュであり、それ自体は厚みを持ちません。  
BoxCollider を付けた場合でも、以下の条件だと通り抜けやすくなります。

- BoxCollider の高さが 0 または極端に薄い
- 1フレームの移動量が床の厚みを超えている
- 重なった後の押し戻しだけで、移動途中の衝突を見ていない
- キャラクター側に床への吸着処理がない
- 接地判定がなく、床上にいる状態を保持していない
- 床法線を見て、床・壁・天井を分類していない

押し戻しは「すでに重なっている」ことが前提です。  
速い移動や薄い床では、前フレームでは床の上、次フレームでは床の下になり、重なりフレームを取り逃がすことがあります。

## Calyx に入れるべき設計

### 1. StaticCollider と地形

地面・壁・地形は `Static` な PhysicsBody として扱います。

推奨:

- Plane の見た目とは別に、厚みのある BoxCollider を置く
- 床 BoxCollider は最低でも `0.2f` 以上の厚みを持たせる
- 床の上面が見た目の Plane と一致するように配置する

例:

- Plane 表示位置: `y = 0`
- 床 BoxCollider center: `y = -0.1`
- 床 BoxCollider size.y: `0.2`

### 2. CharacterController を追加する

`PhysicsSystem` は汎用押し戻しを担当します。  
キャラクターの歩行は別責務として `CharacterController` または `CharacterMotor` を追加します。

候補クラス:

- `Engine/Physics/Character/CharacterController.h`
- `Engine/Physics/Character/CharacterController.cpp`
- `Engine/Physics/Character/CharacterControllerConfig.h`

責務:

- 入力やAIから希望移動量を受け取る
- 重力を適用する
- 移動前に下方向へ床 Probe を行う
- 床法線と角度で floor / wall / ceiling を分類する
- 床上なら下方向へ snap する
- 壁に当たったら壁法線に沿って slide する
- 接地状態 `isGrounded` を保持する

### 3. 床 Probe を追加する

地面を走るには、カプセルの下に短い判定を飛ばして床を探します。

初期実装では Ray でもよいですが、キャラクターの足元幅を考えると SphereCast / CapsuleCast が望ましいです。

段階:

1. `Raycast` で床を探す
2. `SphereCast` で足元の幅を持たせる
3. `CapsuleCast` で移動全体を扱う

### 4. 床分類

接触法線 `normal` と上方向 `up = (0, 1, 0)` の内積から床かどうかを判定します。

```cpp
float dot = Dot(normal, up);
float minFloorDot = cos(maxSlopeAngle);
bool isFloor = dot >= minFloorDot;
```

例:

- `maxSlopeAngle = 45度`
- `minFloorDot = cos(45度) = 0.707`
- 法線のY成分が `0.707` 以上なら床

### 5. Ground Snap

接地中または軽く浮いているとき、下方向に短い距離だけ床を探して吸着します。

目的:

- 下り坂でキャラクターが浮かない
- 微小な段差でガタつかない
- Plane と BoxCollider の境界で接地が途切れにくくなる

設定例:

- `groundSnapDistance = 0.2f`
- `skinWidth = 0.05f`
- `maxSlopeAngle = 45.0f`

### 6. Slide

壁にぶつかった場合、移動ベクトルから壁法線方向の成分を取り除きます。

```cpp
velocity = velocity - normal * Dot(velocity, normal);
```

これにより、壁へ向かう成分だけが消え、壁沿い方向の移動が残ります。

## 推奨実装順

1. `ContactManifold` を Collision/Physics 共通で扱える構造にする
2. `PhysicsSystem` の接触生成を公開可能な helper に分離する
3. `CharacterControllerConfig` を追加する
4. `CharacterController` を追加する
5. `BaseGameObject` または `Actor` に CharacterController を任意付与できるようにする
6. 床 Probe を Raycast で実装する
7. `isGrounded` / `floorNormal` / `floorAngle` を持たせる
8. `Ground Snap` を追加する
9. 壁との slide を追加する
10. 必要になったら SphereCast / CapsuleCast へ拡張する

## 直近の実装方針

最初の実装では、以下に絞るのが安全です。

- キャラクターは CapsuleCollider 前提
- 地形は Static BoxCollider 前提
- 床 Probe は Raycast
- 床なら Y 座標を補正
- 壁なら水平移動を slide
- Dynamic Rigidbody はまだ扱わない

この段階で、Plane 見た目 + 厚み付き BoxCollider の地面を安定して歩けるようにします。

## Unreal 寄せの CharacterMovementComponent

Calyx では Unreal Engine の `CharacterMovementComponent` に寄せて、キャラクター歩行を `Actor` 側の専用コンポーネントとして扱います。

追加する主な概念:

- `CharacterMovementMode`
  - `Walking`
  - `Falling`
- `FindFloorResult`
  - 床に当たったか
  - 歩行可能な床か
  - カプセル底面から床までの距離
  - 床の法線
  - 床コライダー
- `CharacterMovementComponent`
  - `FindFloor()`
  - `Tick()`
  - `SnapToFloor()`
  - `IsWalkable()`

### 更新順

`Actor::AlwaysUpdate()` で以下の順に処理します。

1. `CharacterMovementComponent::Tick()`
   - 床を探索します。
   - 歩行可能な床が近ければ `Walking` にします。
   - 床がなければ `Falling` にして重力を適用します。
   - `Walking` 中は床へ Snap します。
2. `BaseGameObject::AlwaysUpdate()`
   - Transform を更新します。
   - Collider を Transform に同期します。
   - Model を更新します。

### 初期実装の床探索

最初の `FindFloor()` は、キャラクター中心から下方向へ Probe します。

- キャラクター側は CapsuleCollider を推奨します。
- 地面側は Static な BoxCollider を対象にします。
- BoxCollider は床探索時だけキャラクター半径ぶん横方向へ膨らませます。
- 床法線と上方向の内積で歩行可能か判定します。

これは厳密な CapsuleCast ではありません。  
ただし、Plane の見た目に厚み付き BoxCollider を配置する初期段階では十分に安定します。

### 推奨する地面設定

Plane の見た目が `y = 0` にある場合:

- BoxCollider center.y = `-0.1`
- BoxCollider size.y = `0.2`
- PhysicsBody = `Static`
- Collider `Is Trigger` = `false`

キャラクター:

- CapsuleCollider
- PhysicsBody = `Kinematic`
- CharacterMovement `Floor Probe Distance` = `0.35`
- CharacterMovement `Floor Snap Distance` = `0.2`
- CharacterMovement `Skin Width` = `0.05`

### 今後の拡張

今後は以下を追加すると、さらに Unreal のキャラクター移動に近づきます。

- CapsuleCast による移動前 Sweep
- `StepOffset` による段差上り
- 壁ヒット時の `SlideAlongSurface`
- `CurrentFloor` のキャッシュ
- 移動床の速度継承
- `MovementMode::Flying`
- `MovementMode::Swimming`
