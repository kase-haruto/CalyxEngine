# Calyx Reflection / Object Collection

このフォルダには、`SceneObject` 系クラスをビルド時に自動収集する仕組みが入っています。
C++ の実行時リフレクションではなく、ヘッダに書いたマーカーをビルド前スクリプトが読み取り、C++ の登録コードを生成する方式です。

## 目的

クラス定義の直前に `CALYX_OBJECT(...)` を書くだけで、`SceneObjectRegistry` へ型登録できるようにします。
登録された型は、エディタ配置、シーン保存、シーンロード、Prefab生成で使われます。

```cpp
#include <Engine/Foundation/Reflection/CalyxReflection.h>

CALYX_OBJECT(Category = Event, DisplayName = "Camera Event")
class CameraEventObject : public BaseEventObject {
public:
	CameraEventObject();
};
```

次回ビルド時にこのクラスが `SceneObjectRegistry` に登録されます。
`Placeable = true` の場合は、エディタの配置パネルにも表示されます。

## 全体の流れ

1. MSBuild の PreBuildEvent で `Tools/Reflection/generate_reflection.ps1` が実行されます。
2. スクリプトが `Engine/**/*.h` と `Game/**/*.h` を走査します。
3. `CALYX_OBJECT(...)` の直後にある `class ClassName :` を検出します。
4. 検出結果から以下の生成ファイルを書き出します。
   - `Engine/Foundation/Reflection/CalyxObjectRegistry.generated.h`
   - `Engine/Foundation/Reflection/CalyxObjectRegistry.generated.cpp`
5. Engine側は `CalyxEngine::RegisterGeneratedSceneObjects()`、Game側は `CalyxEngine::RegisterGeneratedGameSceneObjects()` が呼ばれます。
6. 生成された登録コードが `SceneObjectRegistry` にクラス情報を登録します。
7. `PlaceToolPanel` が `SceneObjectRegistry` から配置可能オブジェクト一覧を取得し、Events に表示します。

## 主要ファイル

- `Engine/Foundation/Reflection/CalyxReflection.h`
  - `CALYX_OBJECT` などのマクロ定義です。
  - C++ 上では空マクロで、generator 用の目印として使います。

- `Tools/Reflection/generate_reflection.ps1`
  - MSBuild から実行される実際の generator です。
  - Visual Studio 環境だけで動かせるように PowerShell で書いています。

- `Tools/Reflection/generate_reflection.py`
  - Python 版 generator です。
  - 現在は MSBuild には接続していません。

- `Engine/Foundation/Reflection/CalyxObjectRegistry.generated.cpp`
  - 自動生成される登録コードです。
  - 手で編集しないでください。

- `Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.*`
  - シーンロードとエディタ配置の両方で使う実行時 Registry です。

- `Engine/Application/UI/Panels/PlaceToolPanel.cpp`
  - Registry から配置可能オブジェクトを取得し、配置パネルに表示します。

## CALYX_OBJECT

現在実際に使われているマーカーです。

最小例:

```cpp
CALYX_OBJECT(Category = Event, DisplayName = "Camera Event")
```

指定できる項目:

```cpp
CALYX_OBJECT(
	Category = Event,
	DisplayName = "Camera Event",
	Icon = "UI/Tool/event.png",
	Placeable = true
)
```

- `Category`
  - `ObjectType` に対応します。
  - 現在の配置パネルでは主に `Event` を使います。

- `DisplayName`
  - エディタに表示される名前です。
  - 未指定の場合は `TypeName` が使われます。

- `Icon`
  - `TextureManager::LoadTexture` に渡すアイコンパスです。
  - `Resources/Assets` からの相対パスで指定します。
  - デフォルトは `UI/Tool/event.png` です。

- `Placeable`
  - `true` の場合、エディタの配置パネルに表示されます。
  - デフォルトは `true` です。
  - 保存やロードには必要だが配置パネルに出したくない型は `false` にします。

- `TypeName`
  - 保存データや Registry で使う型名です。
  - 未指定の場合は C++ のクラス名が使われます。
  - シーン保存時の JSON `type` にはこの名前が書かれます。

通常の Event では、まずはこの形で十分です。

```cpp
CALYX_OBJECT(Category = Event, DisplayName = "Tutorial Event")
```

非表示にしたい場合だけ `Placeable = false` を指定します。

```cpp
CALYX_OBJECT(Category = Event, DisplayName = "Internal Event", Placeable = false)
```

この場合も `SceneObjectRegistry` には登録されるため、シーンロードやPrefab生成には利用できます。
配置パネルに表示されないだけです。

専用アイコンを使いたい場合だけ `Icon` を指定します。

```cpp
CALYX_OBJECT(Category = Event, DisplayName = "Boss Event", Icon = "UI/Tool/bossEvent.png")
```

## CALYX_GENERATED_BODY

`CALYX_GENERATED_BODY()` は現在は任意のプレースホルダーです。

現在の役割:

- C++ 上では何もしません。
- generator もまだ読み取っていません。
- 「このクラスは生成系に参加する」という目印として残せる程度です。

将来的な役割:

- クラスごとの生成コード差し込み場所にする可能性があります。
- 例: static な型情報取得、プロパティテーブル接続、エディタ用メタデータ hook、バージョン移行 hook など。

現段階では必須ではありません。

```cpp
class CameraEventObject : public BaseEventObject {
public:
	CameraEventObject();
};
```

このように書いて問題ありません。

## CALYX_PROPERTY

> 現在の方針（2026-07）
>
> `CALYX_PROPERTY` は実装しません。数値、真偽値、ベクトルなどの編集・保存には、
> 既存の `SerializableObject` と `AddField()` を使用します。
> オブジェクト同士の関係は Property Reflection へ混在させず、
> `SceneObjectRef<T>` による型付きGUID参照として独立して扱います。

### 現在使用するAPI

通常の設定値は `SerializableObject` へ登録します。

```cpp
struct EnemyParams final : CalyxEngine::SerializableObject {
	EnemyParams() {
		AddField("moveSpeed", moveSpeed)
			.Category("Movement")
			.Tooltip("Enemyの移動速度")
			.Range(0.0f, 100.0f);
	}

	float moveSpeed = 5.0f;
};
```

特定のSceneObjectとの関係は `SceneObjectRef<T>` で宣言します。

```cpp
CalyxEngine::SceneObjectRef<Player> targetPlayer_;
```

InspectorではHierarchyから対象を割り当てます。

```cpp
GuiCmd::SceneObjectReferenceField("Target Player", targetPlayer_);
```

実行時には型付きポインタとして解決します。

```cpp
if(auto player = targetPlayer_.Resolve()) {
	const auto position = player->GetWorldTransform().GetWorldPosition();
}
```

詳細は `Docs/SceneObjectReference_README.md` を参照してください。

### 将来Property Reflectionを導入する場合の責務

将来 `CALYX_PROPERTY` を実装する場合も、次の責務だけを自動化対象とします。

- `SerializableObject::AddField()` 相当のフィールド登録
- Inspector用メタデータの宣言
- 保存キー、表示名、Category、Tooltip、Min、Maxの指定

`SceneObjectRef<T>` の参照保持、GUID解決、型検証、Prefabリマップは
Property Reflectionへ移さず、独立した参照システムの責務として維持します。

想定構文は次の通りですが、現時点では使用できません。

```cpp
CALYX_PROPERTY(Edit, Save, Category = "Movement", Min = 0.0f)
float moveSpeed_ = 5.0f;
```

この分離により、Property Reflectionを将来追加または廃止しても、
シーン内のオブジェクト参照形式と保存済みGUIDへ影響を与えません。

`CALYX_PROPERTY(...)` も現在はプレースホルダーです。

現在の役割:

- C++ 上では何もしません。
- generator はまだプロパティを収集していません。

将来的な役割:

- フィールドを Inspector に自動表示するためのマーカーにする予定です。
- 表示名、範囲、カテゴリ、保存対象、エディタ専用フラグなどを指定できるようにする想定です。

将来的な構文例:

```cpp
CALYX_PROPERTY(Edit, DisplayName = "Trigger Radius", Min = 0.0f)
float triggerRadius_ = 1.0f;
```

現在の実装は、意図的に「オブジェクト自動収集」までに留めています。
プロパティ自動収集は、既存の `IConfigurable` と JSON 保存/読み込みの流れに合わせて次の段階で設計するのが安全です。

## 型名とシーン保存

生成された登録コードは `SceneObjectClassDesc::typeName` と factory を `SceneObjectRegistry` に登録します。
`SceneObjectRegistry::Create(typeName)` で生成されたオブジェクトには、生成時に `SceneObject::SetTypeName(typeName)` が呼ばれます。

シーン保存時は `SceneSerializer` が `SceneObject::GetTypeName()` を読み、JSONの `type` に保存します。

```json
{
  "type": "TutorialEvent"
}
```

`CALYX_OBJECT(TypeName = "Tutorial")` のように明示した場合は、`"Tutorial"` が保存されます。

このため、`CALYX_OBJECT` を使う通常のゲームオブジェクトでは、毎回 `GetObjectClassName()` をoverrideする必要はありません。

注意点:

- 保存対象のオブジェクトは `SceneObjectRegistry::Create()` 経由で生成するのが基本です。
- `std::make_shared<T>()` で直接生成した場合、Registry由来の `typeName` は自動設定されません。
- cpp側に `REGISTER_SCENE_OBJECT(T)` を追加で書く必要はありません。

## 新しい配置可能 Event の追加手順

1. 通常通り Event クラスを作成します。
2. ヘッダで `Engine/Foundation/Reflection/CalyxReflection.h` を include します。
3. クラス定義の直前に `CALYX_OBJECT(...)` を書きます。
4. プロジェクトをビルドします。
5. PreBuildEvent により生成ファイルが更新されます。
6. エディタの Events カテゴリに表示されます。

例:

```cpp
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/Event/BaseEventObject.h>

CALYX_OBJECT(Category = Event, DisplayName = "Tutorial Event")
class TutorialEvent : public BaseEventObject {
public:
	TutorialEvent();
};
```

この例では、デフォルトアイコン `UI/Tool/event.png` が使われ、配置可能状態で登録されます。

## 現在の制限

- generator は正規表現ベースです。
- `CALYX_OBJECT(...)` はクラス定義の直前に置いてください。
- 対象クラスは `Engine` または `Game` 配下の `.h` に書いてください。
- 対象クラスは `std::make_shared<ClassName>()` で作れるデフォルトコンストラクタを持つ必要があります。
- 現在実装済みなのはオブジェクト自動収集のみです。
- プロパティ自動収集は未実装です。
