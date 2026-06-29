# Scene Settings設計

## 1. 目的

`SceneSettings`は、個々の`SceneObject`ではなく、シーン全体へ作用する設定を保存するための仕組みである。

最初の利用対象としてCollision Layer一覧とCollision Matrixをシーン設定へ移した。今後はRendering、Environment、Audio、Navigation、Physics等の設定カテゴリを同じ仕組みへ追加できる。

この設計では、次の2種類の情報を明確に分離する。

- ColliderがどのLayerに所属するか: ColliderConfigの`layerId`
- そのシーンにどのLayerが存在し、Layer同士が衝突するか: SceneSettingsのCollision設定

Colliderの所属Layerはオブジェクトごとに異なる可能性があるため、Collider側に残す。一方、Layer定義と対応表は同じシーン内で共有されるルールなので、Colliderインスタンスへ重複保存せずSceneSettingsで一元管理する。

## 2. 全体構成

```text
SceneContext
  ├─ SceneObjectLibrary
  ├─ CameraManager
  ├─ LightLibrary
  └─ SceneSettings
       └─ CollisionLayerSettings
            ├─ CollisionLayer一覧
            └─ CollisionMatrix
```

Editor側は次の構成になる。

```text
Settings menu
  └─ Scene Settings
       ├─ Category list
       │    └─ Collision
       └─ Category details
            └─ CollisionLayerTableUI
```

主な関連ファイルは次の通り。

| ファイル | 責務 |
|---|---|
| `Engine/Scene/Settings/SceneSettings.h/.cpp` | シーン設定カテゴリのルートコンテナ |
| `Engine/Scene/Settings/SceneSettingsWindow.h/.cpp` | Scene SettingsのEditorウィンドウ |
| `Engine/Scene/Context/SceneContext.h/.cpp` | SceneSettingsの所有と現在設定の有効化 |
| `Engine/Scene/Serializer/SceneSerializer.cpp` | SceneSettingsの保存・読込 |
| `Engine/Collision/CollisionLayerSettings.h/.cpp` | Collisionカテゴリの実データ |
| `Engine/Collision/CollisionLayerTableUI.h/.cpp` | Collisionカテゴリの編集UI |

## 3. SceneContextが設定を所有する理由

Layer一覧をグローバルなEngine Settingsへ保存すると、すべてのシーンが同じCollisionルールを使うことになる。しかし、ゲーム中のシーンとEditor Preview、テストシーン等で異なるLayer構成やMatrixを使用したい場合がある。

そこで、`SceneContext`が`SceneSettings`を値として所有する。

```cpp
class SceneContext {
    SceneSettings settings_;
};
```

この所有関係には次の利点がある。

- シーンを切り替えるだけでCollisionルールも切り替わる。
- Editor用ContextとRuntime Contextが独立した設定を持てる。
- LivePP用Scene snapshotにも設定を含められる。
- SceneContext破棄時に設定の寿命も同時に終了する。
- SceneSerializerがオブジェクトと設定を1つの`.scene`へ保存できる。

## 4. Colliderに残す情報

SceneSettingsへ移すのはLayer定義とMatrixであり、Colliderの`layerId`そのものではない。

例えば同じシーンにPlayer用Collider、Stage用Collider、Trigger用Colliderがある場合、それぞれが所属Layer IDを保持する必要がある。所属情報までシーン側の別テーブルへ移すと、ColliderとテーブルをGUID等で関連付ける追加処理が必要になり、Prefab再利用や複製処理も複雑になる。

したがって責務を次のように分ける。

```text
ColliderConfig.layerId
  = このColliderが所属するLayer

SceneSettings.collision
  = このシーンで利用可能なLayerと衝突ルール
```

この分離により、Prefabは所属Layer IDを保持したまま別シーンへ配置できる。配置先シーンに同じIDが存在しない場合は`Invalid Layer`として表示され、衝突判定へ進まない。

## 5. Scene JSON形式

SceneSerializerはScene JSONのトップレベルへ`settings`を追加する。

```json
{
  "version": 2,
  "sceneName": "GameScene",
  "settings": {
    "version": 1,
    "collision": {
      "layers": [
        { "id": 0, "name": "Default" },
        { "id": 1, "name": "Character" },
        { "id": 2, "name": "Stage" }
      ],
      "matrixRows": [
        7,
        6,
        7
      ]
    }
  },
  "objects": []
}
```

`settings`の下をカテゴリ単位のキーへ分けている。今後設定を追加しても、既存のCollision JSONを変更する必要がない。

```json
"settings": {
  "version": 1,
  "collision": {},
  "environment": {},
  "audio": {},
  "navigation": {}
}
```

未知のカテゴリは読込時に無視する。これにより、設定カテゴリの追加による旧シーンとの互換性を維持する。

## 6. Collision Matrixの保存形式

Matrixは`matrixRows`へ32個の`uint32_t`として保存する。配列のindexがLayer A、整数内のbitがLayer Bを表す。

```text
matrixRows[A] の bit B = Layer AとLayer Bの衝突可否
```

文字列の2次元配列ではなくbit列にした理由は次の通り。

- 実行時のCollisionMatrixと同じ表現へ直接対応できる。
- Layer名の変更に影響されない。
- 32x32個のboolを保存するより小さい。
- Layer削除による空きIDがあっても行indexが変化しない。

読込時は保存データのA-BとB-AのどちらかがONならONとして扱い、`SetCanCollide`を通して対称なMatrixへ正規化する。外部編集等でJSONが非対称になっても、実行時には対称性が維持される。

## 7. 保存と読込の順序

### 保存

`SceneSerializer::DumpJson`は次の順にルートJSONを構築する。

1. SceneObjectをシリアライズする。
2. `sceneName`を保存する。
3. `SceneSettings::ToJson`を`settings`へ保存する。
4. SceneObject配列を`objects`へ保存する。

Scene Settingsウィンドウの編集は現在SceneContextのデータへ即時反映される。ファイルへの書込みは通常の`Save Scene`で行う。

### 読込

`SceneSerializer::LoadJson`は次の順に処理する。

1. 既存SceneObjectとCollisionManager登録をClearする。
2. `settings`をSceneSettingsへ適用する。
3. SceneObjectを生成する。
4. ColliderConfigを適用する。
5. 親子関係やサブシステム参照を復元する。

SceneObjectより先にSceneSettingsを復元することで、Collider生成時点からLayer IDの妥当性とMatrixを参照できる。

## 8. 旧シーンとの互換性

`settings`を持たないversion 1以前のシーンはエラーにしない。

その場合は次のDefault設定を生成する。

- Layer 0: `Default`
- Default同士の衝突: ON

旧ColliderConfigに`layerId`が存在しない場合もLayer 0として読み込まれる。そのため、既存シーンは明示的な変換処理なしで開ける。

トップレベルがSceneObject配列だけのさらに古い形式も、従来の互換読込処理を維持する。

## 9. 現在シーンの設定切替

既存コードには`CollisionLayerSettings::GetInstance()`を使うCollider InspectorとCollisionManagerが存在する。これらをすべてSceneContext依存へ書き換えると、Collision層とScene層の依存が強くなる。

そこで`SceneContext::MakeCurrent()`が、現在シーンのCollision設定を有効な設定として登録する。

```cpp
void SceneContext::MakeCurrent() {
    current_ = this;
    CollisionLayerSettings::SetActiveSettings(
        &settings_.GetCollisionSettings());
}
```

`SetActiveSettings`は所有権を持たない。設定の所有者は常にSceneContextである。現在SceneContextが破棄される場合はactive pointerを解除し、Defaultだけを持つfallback設定へ戻す。

この方法により、既存Collision APIを維持しながらデータの所有権だけをシーンへ移している。

## 10. 複数SceneContextへの対応

Editorでは次のような複数Contextが存在する。

- 通常の編集シーン
- Runtime Context
- Particle Preview Context
- Prefab Edit Context
- Asset Preview Context

各Contextは独立したSceneSettingsを所有する。既存コードが`MakeCurrent()`でContextを切り替えると、Collision設定も同時に切り替わる。

Preview処理が終了して以前のContextへ戻る場合も`MakeCurrent()`を呼ぶため、active Collision設定が元のシーンへ復元される。

Scene Settingsウィンドウは`SceneContext::Current()`を受け取り、さらに対象の`CollisionLayerSettings&`をUIへ明示的に渡す。これにより、ウィンドウ表示中に別Contextへ切り替わっても、現在Contextの設定だけを編集する。

## 11. Scene Settings UI

Editorのメニューから次の順で開く。

```text
Settings
  └─ Scene Settings
```

ウィンドウは左側のカテゴリ一覧と右側の詳細領域に分かれている。

現在のカテゴリは`Collision`だけだが、Category enumと`DrawCategoryDetails`を分離しているため、カテゴリを追加してもCollision UIを変更する必要がない。

Collisionカテゴリでは次を編集できる。

- Layer一覧の確認
- Layer追加
- Layer削除
- Layerリネーム
- Collision MatrixのCheckbox編集

Layer 0 Defaultの削除とリネームはUIとデータAPIの両方で禁止する。

## 12. 新しいシーン設定カテゴリの追加方法

例としてEnvironment設定を追加する場合は次の手順になる。

### 1. データ型を定義する

```cpp
struct SceneEnvironmentSettings {
    float ambientIntensity = 1.0f;
    std::string skyboxGuid;
};
```

### 2. SceneSettingsへ所有させる

```cpp
class SceneSettings {
public:
    SceneEnvironmentSettings& GetEnvironmentSettings();

private:
    CollisionLayerSettings collisionSettings_;
    SceneEnvironmentSettings environmentSettings_;
};
```

### 3. ToJson／ApplyJsonへカテゴリを追加する

```cpp
json["environment"] = SerializeEnvironment(environmentSettings_);
```

カテゴリが存在しない旧シーンではDefault値を維持する。

### 4. SceneSettingsWindowへカテゴリを追加する

```cpp
enum class Category {
    Collision,
    Environment,
};
```

`GetCategoryLabel`と`DrawCategoryDetails`へ分岐を追加する。

この手順ではSceneSerializer本体をカテゴリごとに変更しない。SceneSerializerは`SceneSettings::ToJson`と`ApplyJson`だけを呼ぶため、カテゴリ固有処理はSceneSettings以下へ閉じ込められる。

## 13. Project Settingsとの区別

Engine Settings、Project Settings、Scene Settingsは寿命と適用範囲が異なる。

| 設定 | 適用範囲 | 保存先の例 |
|---|---|---|
| Engine Settings | Editor／Engine全体 | EngineSettings.json |
| Project Settings | 1ゲームプロジェクト全体 | project設定ファイル |
| Scene Settings | 1つのシーン | `.scene`内の`settings` |

Collision MatrixをScene Settingsへ置いたため、シーンごとに異なる衝突規則を設定できる。すべてのシーンで同じ規則を共有したい場合は、将来Project SettingsのテンプレートをSceneSettings初期値へコピーする方式を追加できる。

## 14. 注意点

- Scene Settingsの変更はUndo／Redo対象ではない。必要な場合はSceneSettings全体またはカテゴリ単位のCommandを追加する。
- ウィンドウで編集しただけではファイルへ保存されない。通常の`Save Scene`を実行する。
- Layerを削除すると、そのIDを持つColliderは`Invalid Layer`になる。
- 削除後に同じIDで新Layerを追加すると、古いColliderが新Layerを参照する可能性がある。厳密な世代管理が必要ならGUIDまたはgeneration付きIDを検討する。
- PrefabはLayer IDだけを持つため、配置先シーンで対応するLayer IDを用意する必要がある。
- Runtime中にMatrixを変更すると、次のCollision更新から反映される。前フレームまで衝突していたペアが無効になった場合はExit通知が発生する。

## 15. 維持すべき設計条件

今後Scene Settingsを拡張するときも、次の条件を維持する。

1. SceneContextがSceneSettingsの所有者である。
2. SceneObject固有情報をSceneSettingsへ重複保存しない。
3. 設定カテゴリはSceneSerializerではなくSceneSettings内でシリアライズする。
4. 未知カテゴリと欠落カテゴリを安全に扱う。
5. SceneObject生成より前にSceneSettingsを復元する。
6. Context切替時に、そのContext固有の実行時設定を有効化する。
7. Editor UIは対象SceneContextを明示して編集する。
8. 旧シーンはDefault設定で読み込めるようにする。
