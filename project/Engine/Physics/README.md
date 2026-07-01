# Calyx Physics

## 概要

`Engine/Physics`は、Colliderが検出した接触を使った物理応答、Dynamic Bodyの固定時間シミュレーション、キャラクター移動を担当する。

ColliderとPhysicsBodyは責務を分離している。

- `Collider`
  - Sphere／OBB／Capsuleなどの衝突形状を保持する。
  - Collision Layer、Trigger、Enter／Stay／Exit通知を管理する。
- `PhysicsBody`
  - Static／Kinematic／DynamicのBody種別を保持する。
  - Dynamicの線形速度、質量、重力設定を保持する。
- `PhysicsSystem`
  - 60 Hzの固定時間ステップを実行する。
  - Dynamicへ重力を適用して位置を積分する。
  - 接触法線方向の速度と、Collider同士のめり込みを補正する。
- `CharacterMovementComponent`
  - `Walking`／`Falling`の移動モードを管理する。
  - 床探索、重力、ジャンプ、床への吸着を担当する。

Collision LayerとCollision Matrixについては`Docs/CollisionReadme.md`を参照する。

## Body種別

| Body種別 | 移動方法 | 重力 | 物理Solverによる位置補正 | 主な用途 |
|---|---|---:|---:|---|
| `Static` | 原則として移動しない | なし | 受けない | 床、壁、固定障害物 |
| `Kinematic` | ゲームコードで移動する | なし | 現在は受ける | キャラクター、移動床、扉 |
| `Dynamic` | 物理システムが速度から移動する | 設定可能 | 受ける | 箱、ボール、落下物 |

### Static

Staticは無限質量の固定物として扱う。衝突判定には参加するが、位置補正と速度補正によって移動しない。

床、壁、建物など、実行中に移動しないオブジェクトへ使用する。

### Kinematic

KinematicはTransformや`CharacterMovementComponent`など、ゲームコード側が移動を決定するBodyである。重力とDynamic用の速度積分は適用しない。

現在は既存挙動との互換性を保つため、次の位置補正を行う。

- Kinematic対StaticではKinematicだけを補正する。
- Kinematic対Kinematicでは両方を同じ比率で補正する。
- Kinematic対DynamicではKinematicを動かさず、Dynamicだけを補正する。

将来Kinematic用のSweep／Slideを導入した後は、Kinematicを汎用位置Solverから外すことを検討する。

### Dynamic

Dynamicは線形速度、質量、重力設定を持ち、固定時間ステップごとに物理システムが位置を更新する。

現在のDynamicは並進だけを扱う最小実装である。回転、角速度、摩擦、反発はまだ扱わない。

## PhysicsBody設定

PhysicsBodyは次の値をScene／Prefabへ保存する。

| 設定 | 内容 |
|---|---|
| `enabled` | 物理応答を有効にするか |
| `bodyType` | `0: Static`、`1: Kinematic`、`2: Dynamic` |
| `pushbackRatio` | 位置補正へ掛けるオブジェクト単位の比率 |
| `useGravity` | Dynamicへワールド重力を適用するか |
| `gravityScale` | ワールド重力へ掛ける倍率 |
| `mass` | Dynamicの質量 |

線形速度は実行中の状態なのでScene／Prefabへ保存しない。旧データに新しい設定が存在しない場合は、JSON読込時に既定値を使用する。

Dynamicは次のようにコードから初期化できる。

```cpp
PhysicsBody& body = object->GetPhysicsBody();
body.SetEnabled(true);
body.SetBodyType(PhysicsBodyType::Dynamic);
body.SetMass(2.0f);
body.SetLinearVelocity({0.0f, 5.0f, 0.0f});
```

## 固定時間ステップ

Runtime中のPhysicsは`PhysicsSystem::Update()`へ描画フレームの`deltaTime`を渡し、内部で60 Hzの固定Stepへ変換する。

```text
SceneContext::Update
  ↓
PhysicsSystem::Update(deltaTime)
  ↓ accumulatorへ時間を蓄積
PhysicsSystem::Step(1 / 60秒)
```

1描画フレームで実行する物理Stepは最大4回に制限する。長時間停止後や大きなフレーム落ちで古い時間を無制限に処理すると、処理落ちがさらに物理Stepを増やすためである。

1回の固定Stepは次の順序で処理する。

```text
Dynamicへ重力を適用
  ↓
線形速度から位置を積分
  ↓
Colliderを移動後のTransformへ同期
  ↓
押し戻し前の接触からEnter／Stay／Exitを確定
  ↓
法線方向の速度を補正
  ↓
めり込み位置を補正
```

Editor中は物理時間を進めず、配置操作によって発生しためり込みだけを解消する。

## 速度補正

Dynamicが接触面へ近づいている場合だけ、接触法線方向の相対速度をImpulseで打ち消す。

```text
relativeVelocity = velocityA - velocityB
velocityAlongNormal = Dot(relativeVelocity, contactNormal)
```

`velocityAlongNormal >= 0`なら互いに離れているため補正しない。現在の反発係数は0として扱うため、床へ落下したDynamicは法線方向の速度を失い、その場で停止する。

Dynamic同士では逆質量に応じて速度変化を分配する。StaticとKinematicの逆質量は0として扱う。

## めり込み位置補正

位置補正では、Contactが返した法線と侵入量を使用する。

- 法線はBody AをBody Bから離す方向へ統一する。
- 微小な侵入量`0.001`は許容して振動を抑える。
- 1反復では残り侵入量の80%を補正する。
- Colliderの組み合わせを最大4回反復する。
- Transformを補正した直後にColliderも同期する。

複数の壁や床へ同時に接触した場合、1つの接触を直した結果として別のColliderへ入ることがある。位置Solverを反復することで、この連鎖した侵入を段階的に減らす。

Sphere、OBB、Capsuleの組み合わせに対応している。Capsule同士は中心線分同士の最近点を解析的に求め、固定サンプリングによる判定漏れを避けている。

## Triggerと衝突イベント

TriggerはEnter／Stay／Exitの検出には参加するが、速度補正と位置補正には参加しない。

物理接触のイベントは押し戻し前に確定する。押し戻し後にだけ重なりを調べると、完全に分離された接触のEnterが欠落するためである。

Runtimeでは固定Physics Step単位で通知する。そのため、描画フレーム内で複数のPhysics Stepが実行された場合、Stayが同じ描画フレーム中に複数回通知される可能性がある。

## 地面設定

Planeは描画用メッシュなので、地面として使う場合は別途StaticなBoxColliderを配置する。

推奨例:

- Plane表示面: `y = 0`
- BoxCollider center.y: `-0.1`
- BoxCollider size.y: `0.2`
- PhysicsBody: `Static`
- Collider `Is Trigger`: `false`

## Dynamic設定

落下する箱やボールには次の設定を使用する。

- Collider: `BoxCollider`または`SphereCollider`
- PhysicsBody: `Dynamic`
- Use Gravity: `true`
- Gravity Scale: `1.0`
- Mass: `1.0`以上
- Pushback Ratio: `1.0`
- Collider `Is Trigger`: `false`

`pushbackRatio`を1未満にすると意図的に侵入量が残りやすくなる。通常の物理オブジェクトでは`1.0`を推奨する。

## キャラクター設定

キャラクターはCapsuleColliderを推奨する。

- Collider: `CapsuleCollider`
- PhysicsBody: `Kinematic`
- CharacterMovement:
  - `Walkable Floor Angle`: `45`
  - `Floor Probe Distance`: `0.35`
  - `Floor Snap Distance`: `0.2`
  - `Skin Width`: `0.05`

`CharacterMovementComponent`は現在、描画フレーム側で移動入力と重力を処理する。Dynamic用の固定Stepとは別系統なので、今後Kinematic用のSweep／Slideと同時にPhysics Stepへ統合する。

## 現在の制限

- 衝突判定は移動後の形状を調べる離散判定である。
- CCDとSwept判定を実装していないため、高速Bodyは薄いColliderを通過する可能性がある。
- Dynamicは並進だけで、回転と角速度を持たない。
- 摩擦係数と反発係数を実装していない。
- `AddForce()`やForce Accumulatorを実装していない。
- Kinematicの移動速度をDynamicへ伝達しない。
- Broad Phaseは総当たりである。
- CollisionManagerとPhysicsSystemに形状判定処理が分かれており、Contact生成はまだ一元化されていない。
- Sleep、Constraint、Jointを実装していない。
- 物理Transformの描画補間を実装していない。
- `FindFloor()`はStaticなBoxColliderへの下方向Probeであり、厳密なCapsuleCastではない。

## 今後の実装順序

1. CollisionManagerとPhysicsSystemのContact生成を一元化する。
2. SphereCast／CapsuleCastとKinematic用Move-and-Slideを追加する。
3. 高速Dynamic向けに簡易SweepまたはCCDを追加する。
4. Force Accumulator、摩擦、反発係数を追加する。
5. Contact法線、侵入量、Sweep経路のDebug Drawを追加する。
6. Body数が増えた段階でBroad PhaseとSleepを追加する。
7. ゲーム上必要になった場合だけ回転、Constraint、Jointを追加する。
