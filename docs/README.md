# Calyx Engine Documentation

## Editor Extension

- [ゲーム固有 Editor Tool 拡張](GameEditorExtensionREADME.md)

## Collision と Physics の責務

Calyx では、衝突検出と押し戻し処理を分けて扱います。

- `CollisionManager`
  - コライダーの登録と解除を管理します。
  - コライダー同士の重なりを判定します。
  - `OnCollisionEnter` / `OnCollisionStay` / `OnCollisionExit` を通知します。
  - 位置補正や押し戻しは担当しません。
- `PhysicsSystem`
  - `CollisionManager` に登録されたコライダーを読み取ります。
  - `PhysicsBody` が有効なオブジェクトだけを押し戻します。
  - `Trigger` コライダーは押し戻し対象から除外します。
  - `Static` と `Kinematic` の組み合わせに応じて Transform を補正します。
- `PhysicsBody`
  - 押し戻しを受けるかどうかを管理します。
  - `Static` / `Kinematic` の Body 種別を管理します。
  - 押し戻し反映率 `Pushback Ratio` を管理します。

## Body 種別

| 種別 | 用途 | 押し戻し時の挙動 |
| --- | --- | --- |
| `Static` | 壁、床、地形、動かない障害物 | 自分は動かず、相手を押し戻します |
| `Kinematic` | プレイヤー、敵、スクリプトで動く物体 | 押し戻し補正を受けます |

現在の実装は位置補正ベースの押し戻しです。速度、質量、重力、摩擦、反発を持つ `Dynamic` Body はまだ実装していません。

## Trigger

`Collider` の `Is Trigger` が有効な場合、そのコライダーは押し戻し対象になりません。

Trigger は以下の用途に使います。

- イベントエリア
- 当たり判定だけを取りたい攻撃範囲
- アイテム取得範囲
- センサー

Trigger でも `CollisionManager` のイベント通知は使えますが、`PhysicsSystem` は位置補正を行いません。

## 更新順

1. 各 `SceneObject` の `Update` / `AlwaysUpdate` で移動します。
2. 各オブジェクトのコライダーが Transform に同期されます。
3. `PhysicsSystem::ResolveAll()` が押し戻しを解決します。
4. `CollisionManager::UpdateCollisionAllCollider()` が接触イベントを通知します。

この順番により、移動後のめり込みを同じフレーム内で補正し、その後に接触イベントを発行できます。

## エディタでの設定

1. オブジェクトに Collider を追加します。
2. Collider の `Is Trigger` を用途に合わせて設定します。
3. Collider の下に表示される `PhysicsBody` を設定します。
4. 壁や床は `Static` にします。
5. プレイヤーや敵など動くオブジェクトは `Kinematic` にします。

`Actor` 派生のオブジェクトは、デフォルトで `Kinematic` になります。

## 終了時の安全性

`CollisionManager` はアプリ終了時の破棄順に依存しないよう、プロセス終了まで生存します。

これは、`Collider` のデストラクタが終了処理中に `CollisionManager::Unregister()` を呼ぶためです。`CollisionManager` が先に破棄されると、破棄済みの内部リストへアクセスしてクラッシュする可能性があります。
