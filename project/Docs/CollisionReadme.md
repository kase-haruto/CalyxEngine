# Collision Layer System

## 1. 目的

このCollisionシステムは、Colliderからゲーム固有の分類を取り除き、エンジンとして再利用可能なLayer方式へ移行するために実装した。

旧方式ではCollider自身がゲーム側の分類enumと衝突対象Maskを保持していた。この方式には次の問題がある。

- エンジンのColliderがゲーム固有のオブジェクト分類を知ってしまう。
- Layerを追加するたびにenum、Inspectorの固定配列、各ColliderのMaskを変更する必要がある。
- ゲームプロジェクトごとにエンジンコードの再コンパイルが必要になる。
- 各Colliderが個別に相手Maskを持つため、AからBとBからAの設定が食い違う可能性がある。
- enum値やbit配置の変更が、保存済みSceneやPrefabの互換性へ直接影響する。

新方式ではColliderは数値の`CollisionLayerId`だけを保持する。Layer名とLayer一覧は`CollisionLayerSettings`、Layer間の衝突可否は`CollisionMatrix`が管理する。

## 2. 構成

```text
Collider
  └─ CollisionLayerId
       │
       v
CollisionLayerSettings
  ├─ CollisionLayer { id, name }
  └─ CollisionMatrix
       │
       v
CollisionManager
  ├─ Layerの妥当性確認
  ├─ Matrixによる事前フィルタ
  ├─ Sphere / OBB / Capsule形状判定
  └─ Enter / Stay / Exit通知
```

関連ファイルは次の通り。

| ファイル | 責務 |
|---|---|
| `CollisionTypes.h` | Layer ID型、Default ID、最大Layer数の定義 |
| `CollisionMatrix.h/.cpp` | Layer組み合わせごとの衝突可否 |
| `CollisionLayerSettings.h/.cpp` | Layer一覧、名前、ID、Matrixの管理 |
| `CollisionLayerTableUI.h/.cpp` | Layer一覧とMatrixを編集するImGui UI |
| `Collider.h/.cpp` | Layer ID、形状、状態、通知Callbackの保持 |
| `CollisionManager.h/.cpp` | Collider管理、事前フィルタ、形状判定、通知 |
| `ColliderConfig.h` | Scene／Prefabへ保存するLayer ID |

## 3. ColliderがLayer IDだけを持つ理由

ColliderはLayer名を保存しない。

Layer名を各Colliderへ保存すると、Layerをリネームした際に、そのLayerを使用している全Scene、Prefab、生成済みオブジェクトを書き換える必要がある。また、文字列のタイプミスや表記揺れによって同じLayerが別Layerとして扱われる危険がある。

IDだけを保存する方式では、リネームは`CollisionLayerSettings`内の1箇所を変更するだけでよい。ColliderConfigもIDだけを保存するため、表示名の変更はSceneデータへ影響しない。

```cpp
using CollisionLayerId = uint8_t;

class Collider {
    CollisionLayerId layerId_ = kDefaultCollisionLayerId;
};
```

`uint8_t`を使用しているのは、最大32Layerに対して十分であり、Colliderごとの保持コストも小さいためである。ただし、型が表現できる範囲は0～255なので、実際の有効範囲は必ず`kMaxCollisionLayerCount`で検査する。

## 4. Layer 0をDefaultとして固定する理由

Layer 0は`Default`として予約し、削除とリネームを禁止している。

主な理由は次の通り。

- Layer設定を持たない新規Colliderの初期値として使える。
- 旧形式のSceneやPrefabを読み込んだときのフォールバック先になる。
- 不正な範囲のIDを正規化するときの安全な値として使える。
- プロジェクトごとに「Layer 0の意味」が変化することを防げる。

Defaultまで削除可能にすると、初期化前のColliderや旧データが必ず無効参照になる。最低1つの安定したLayerを保証することで、データ移行と新規生成の処理を単純にしている。

## 5. CollisionMatrixの実装

Matrixは32行の`uint32_t`で保持する。

```cpp
std::array<uint32_t, kMaxCollisionLayerCount> rows_{};
```

`rows_[A]`のbit Bが、Layer AとLayer Bの衝突可否を表す。最大32Layerなので、1行を1つの`uint32_t`で表現できる。

### 対称性

衝突判定はAからBを見る場合とBからAを見る場合で結果が変わらない。そのため`SetCanCollide(A, B, value)`は次の2セルを同時に更新する。

```text
matrix[A][B]
matrix[B][A]
```

対称更新をMatrixクラス内部で保証することで、Editor UIや呼び出し側が片側だけ更新しても不整合が発生しない。UI側の実装規約だけに依存せず、データ構造自身が不変条件を守る設計にしている。

### 無効IDへの対応

32以上のIDをbit shiftに使うと未定義動作になる可能性がある。また、配列アクセスにも使用できない。そのためMatrixの公開関数はすべて範囲を確認する。

- `CanCollide`は`false`を返す。
- `SetCanCollide`は変更せず終了する。
- `ClearLayer`は変更せず終了する。

CollisionManager側でもLayerが現在の一覧に登録されているか確認する。これにより「数値としては0～31だが、削除済みで現在存在しないID」も安全に衝突対象外へできる。

## 6. CollisionLayerSettingsの責務

`CollisionLayerSettings`は次の情報を一元管理する。

- 現在利用可能なLayer一覧
- Layer IDと表示名の対応
- Layer追加、削除、リネーム
- Layer IDの妥当性
- CollisionMatrix

ColliderやCollisionManagerがそれぞれLayer一覧を複製すると、Editor操作後に同期漏れが発生する。そのため、各`SceneContext`が設定を所有し、現在シーンとして有効化された`CollisionLayerSettings`を共通参照する。

### Layer追加

追加時は次を検証する。

1. Layer数が32未満である。
2. 名前が空でない。
3. 同名Layerが存在しない。
4. ID 1～31から未使用の最小IDを見つける。

追加したLayerは既存Layerと衝突可能な状態で初期化する。追加直後にすべての衝突が無効になると、Colliderを設定しても動作しない原因が分かりにくいためである。不要な組み合わせはMatrix UIから明示的に無効化する。

### Layer削除

Layerを削除すると、一覧から取り除くだけでなくMatrixの対応する行と列も無効化する。

既存Sceneや実行中Colliderが削除済みIDを保持している可能性がある。そのColliderのID自体をSettingsから書き換えることはしない。SettingsはColliderの所有権を持たないためである。CollisionManagerが未登録IDを検出し、衝突しないものとして扱う。

この設計により、Layer削除直後でも範囲外アクセスやクラッシュは発生しない。

### Layerリネーム

リネームではIDを変更しない。SceneとPrefabが保存しているのはIDなので、表示名だけを変更してもColliderConfigの再保存は不要である。

### ゲームコードからのLayer検索

ゲーム側で衝突相手を分類するときは、旧方式の`ColliderType`やオブジェクト名ではなく、`Collider::GetLayerId()`を使用する。ゲームコードにシーン固有の数値IDを直接書かず、`FindLayerId()`で設定名からIDを取得する。

`FindLayerId()`は該当するLayerが存在しない場合に`std::nullopt`を返す。設定漏れをDefault Layerへ暗黙変換しないため、意図しない相手を同じ分類として扱わない。

例えばWall、Impediment、対象のStageGimmickを同じ挙動にする場合は、各Colliderへ`Obstacle` Layerを設定し、次のように判定する。

```cpp
void PlayerClone::OnCollisionEnter(Collider* other) {
	if(isGhost_ || !other) {
		return;
	}

	SceneContext* context = SceneContext::Current();
	if(!context) {
		return;
	}

	const auto& collisionSettings =
		context->GetSettings().GetCollisionSettings();
	const auto obstacleLayer = collisionSettings.FindLayerId("Obstacle");

	if(!obstacleLayer || other->GetLayerId() != *obstacleLayer) {
		return;
	}

	context->RemoveObject(
		std::static_pointer_cast<SceneObject>(shared_from_this()));
}
```

Editorでは次の設定を行う。

1. Collision Settingsへ`Obstacle` Layerを追加する。
2. Wall、Impediment、対象のStageGimmickのColliderへ`Obstacle` Layerを割り当てる。
3. Collision Matrixで`PlayerClone`と`Obstacle`の組み合わせを有効にする。

Collision Matrixで組み合わせが無効な場合は、形状判定より前に除外されるため`OnCollisionEnter()`自体が呼ばれない。ゲーム側のLayer ID判定は、Matrixを通過した衝突相手のうち、どのゲーム処理を実行するかを選ぶために使用する。

## 7. CollisionManagerの判定順序

`CheckCollisionPair`は次の順番で処理する。

```text
nullptr確認
  ↓
Collision有効状態
  ↓
Layer IDの存在確認
  ↓
CollisionMatrix確認
  ↓
Sphere / OBB / Capsule形状判定
```

Layer判定を形状判定より前に行う理由は、bit確認と一覧確認が、OBBの分離軸判定やCapsule距離計算より安価だからである。Matrixで衝突しないことが分かっている組み合わせに対して、形状取得や数学的判定を行う必要はない。

Layerフィルタを通過した後の形状判定、CollisionPair管理、Enter／Stay／Exit通知は旧実装を維持している。このためLayerが衝突可能な組み合わせでは、通知タイミングは従来と変わらない。

Matrix設定をONからOFFへ変更した場合、そのペアは次フレームの`currentCollisions_`へ入らない。前フレームに衝突していれば従来の差分処理によってExitが通知される。

## 8. ColliderのRegister／Unregister

Colliderのコンストラクタ、デストラクタ、`SetCollisionEnabled`による登録動作は維持している。

- 有効状態で生成されたColliderはRegisterされる。
- 無効から有効へ変更されたColliderはRegisterされる。
- 有効から無効へ変更されたColliderはUnregisterされる。
- デストラクタは登録状態にかかわらずUnregisterを要求する。

CollisionManagerの更新中に登録状態が変化した場合は、既存の遅延Register／Unregisterキューが使われる。Layer方式への変更は、このライフサイクルには干渉しない。

## 9. InspectorとCollisionLayerTableUI

Collider InspectorのLayer Comboは、固定文字列配列ではなく`CollisionLayerSettings::GetLayers()`から項目を生成する。

これにより、Collision Settingsで行った追加、削除、リネームがInspectorへ即時反映される。Layer変更は既存の`ValueEditCommand`を通すため、Undo／Redoにも対応する。

`CollisionLayerTableUI::Draw()`はウィンドウの`Begin`／`End`を行わない独立UIである。現在は`Scene Settings`ウィンドウのCollisionカテゴリから、対象SceneContextの設定を明示して呼び出す。

```cpp
if(ImGui::Begin("Collision Settings")) {
    CollisionLayerTableUI::Draw();
}
ImGui::End();
```

UI内ではLayer追加、選択Layerのリネームと削除、MatrixのCheckbox編集ができる。Matrixの各Checkboxは行IDと列IDをImGui IDスタックへ積み、同じ表示ラベルを使ってもIDが衝突しないようにしている。

Default選択時はリネームと削除をUI上でも無効化する。ただし安全性をUIだけに依存させず、Settings API側でも同じ操作を拒否する。

## 10. ColliderConfigと旧データ

ColliderConfigは次の値だけを保存する。

```json
{
  "layerId": 0
}
```

旧形式の分類値と対象Maskは保存しない。`layerId`を持たない旧データはDefault Layerとして読み込む。

Layer名を保存しないため、リネームによってScene JSONが大量に変更されることもない。範囲外の`layerId`はColliderへ適用するときにDefaultへ正規化する。

一方、0～31の範囲内でも現在Settingsに存在しないIDは保持する。この場合、Inspectorでは`Invalid Layer`と表示され、CollisionManagerでは衝突しない。参照切れをDefaultへ黙って置き換えないため、設定ミスをEditor上で確認できる。

## 11. 他方式との比較

| 方式 | 利点 | 問題 | 採用判断 |
|---|---|---|---|
| ゲーム固有enum | 実装が単純、コード補完が効く | エンジンとゲームが密結合、追加ごとに再コンパイル | 不採用 |
| Colliderごとの対象bit mask | Collider単位で細かく設定可能 | 設定量が多い、A/Bの意図が非対称になりやすい | 現段階では不採用 |
| Layer名を文字列保存 | JSONが読みやすい | リネームに弱い、比較コスト、タイプミス | 不採用 |
| 文字列hashをID化 | 名前から高速検索可能 | hash衝突、リネーム時にIDが変化 | 不採用 |
| GUIDをLayer IDに使用 | 削除・再作成でも識別が安定 | 32x32 Matrixとの対応が複雑、保存サイズ増加 | 現規模では不採用 |
| 数値Layer ID + Matrix | 小さい、判定が高速、名前変更に強い | ID再利用と設定保存の管理が必要 | 採用 |

ColliderごとのMask方式は、同じLayerでも個別に例外を設定したい場合には有効である。ただし最初からMatrixとMaskを併用するとInspectorと判定規則が複雑になる。現在はシーン全体の規則をCollisionMatrixへ集約し、必要性が明確になった段階でCollider単位の追加Maskを検討する。

## 12. 実装上の工夫

### 防御を複数の境界に置く

不正IDはCollider setter、CollisionLayerSettings、CollisionMatrix、CollisionManagerの各境界で安全に扱う。特定の呼び出し経路だけを信用しないため、Editor以外から値を設定してもクラッシュしない。

### データ構造側で対称性を保証する

Matrixの対称性をUI側ではなく`SetCanCollide`内部で保証している。将来、設定ファイル読込やスクリプトAPIからMatrixを書き換える場合も同じ不変条件が維持される。

### 表示名と保存値を分離する

表示名は人間が編集するデータ、Layer IDはSceneが参照するデータとして分離している。これにより、UIの使いやすさと保存データの安定性を両立している。

### 既存Collision通知を変更しない

Layerフィルタは`CheckCollisionPair`の入口へ追加し、形状判定後のCollisionPair処理には変更を加えていない。変更範囲を狭くすることで、Enter／Stay／Exitや遅延登録処理への回帰リスクを抑えている。

## 13. 現段階の注意点と今後の拡張

- Layer一覧とMatrixは`SceneSettings`内へ保存され、SceneSerializerによって`.scene`ファイルへ永続化される。
- Layer削除後に同じ数値IDが再利用されると、そのIDを残している古いColliderは新しいLayerを参照する。厳密な削除世代管理が必要になった場合は、ID世代番号またはGUIDの導入を検討する。
- Layer名の比較は現在case-sensitiveである。大文字小文字を同一視する場合は、追加とリネームの両方で共通の正規化処理を追加する。
- Collider単位の例外ルールが必要になった場合は、Matrix判定後に追加MaskをAND条件として適用できる。ただしMatrixを基本規則とする責務は維持する。
- Layer設定の保存形式を追加する場合は、Layer ID、名前、Matrix行bitを保存し、読込時にDefault Layerと対称性を再検証する。

## 14. 変更時に守る不変条件

今後Collisionシステムを変更する場合も、次の条件を維持する。

1. Layer 0は常にDefaultとして存在する。
2. 有効なLayer IDは0～31の範囲内である。
3. CollisionMatrixは常に対称である。
4. 未登録または不正なLayer IDは形状判定へ進まない。
5. Colliderはゲーム固有名を保持しない。
6. ColliderConfigはLayer名ではなくLayer IDを保存する。
7. Layerフィルタは形状判定より前に実行する。
8. Matrixを通過したペアのEnter／Stay／Exit挙動は従来通りとする。
9. Register／Unregisterの遅延処理をLayer管理から独立させる。
