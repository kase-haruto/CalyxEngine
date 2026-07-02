# SceneObject参照設計

## 目的

CalyxEngineでは、シーン内オブジェクト間の明示的な関係をGUIDで保存します。
メモリアドレスを保存せず、ロード後にGUIDから対象を解決することで、シーン保存、Prefab複製、
対象削除へ安全に対応します。

参照APIは用途に応じて使い分けます。

```cpp
// Player固有のゲーム機能を利用する場合
CalyxEngine::SceneObjectRef<Player> player;

// 位置、回転、スケールだけを読む場合
CalyxEngine::TransformRef targetTransform;
```

`SceneObjectRef<SceneObject>`がSceneObject全体をコピーすることはありません。保持するのはGUIDと
非所有キャッシュだけです。TransformRefを分ける理由はメモリ削減ではなく、利用側が必要以上に
SceneObject APIへ依存することを防ぐためです。

## 既存エンジンとの比較

### Unity

UnityではTransformが独立したComponentであり、GameObjectではなくTransformを直接参照できます。
すべてのGameObjectはTransformを持ち、親子関係もTransform同士で構築されます。

```csharp
[SerializeField]
private Transform target;

Vector3 position = target.position;
childTransform.SetParent(parentTransform);
```

参考: https://docs.unity3d.com/2022.1/Documentation/Manual/class-Transform.html

### Unreal Engine

Unreal EngineのActorはTransformを直接保持せず、RootComponentであるUSceneComponentが
位置、回転、スケールを提供します。Transformだけが必要ならActorではなくSceneComponentを
参照できます。

```cpp
UPROPERTY(EditAnywhere)
TObjectPtr<USceneComponent> TargetComponent;
```

Unrealでは用途に応じて`TObjectPtr`、`TWeakObjectPtr`、`TSoftObjectPtr`なども使い分けます。

参考:

- https://dev.epicgames.com/documentation/unreal-engine/actors-in-unreal-engine
- https://dev.epicgames.com/documentation/unreal-engine/object-pointers-in-unreal-engine

### CalyxEngine

現在のWorldTransformはSceneObject内部の値であり、独立したGUIDや寿命を持ちません。
したがって、WorldTransformポインタを永続参照として保存できません。

```cpp
WorldTransform* target_; // 保存不能であり、削除後にダングリングする可能性がある
```

TransformRefは所有元SceneObjectのGUIDを永続アドレスとして保持し、外部には
`const WorldTransform*`だけを公開します。これはComponent化前のCalyxEngineで、
UnityやUnrealと同等のインターフェース分離を実現するための設計です。

## 全体構成

```text
SerializableObject
    └─ SerializableField
          └─ ISceneObjectReference
                ├─ SceneObjectRef<T>
                └─ TransformRef

ISceneObjectResolver
    └─ SceneObjectLibrary
```

### ISceneObjectResolver

GUID検索だけを公開する抽象です。参照型をSceneContextやSceneObjectLibraryの具象実装から分離し、
テストではFakeResolverへ差し替えられます。

```cpp
class ISceneObjectResolver {
public:
    virtual ~ISceneObjectResolver() = default;

    virtual std::shared_ptr<SceneObject>
        ResolveSceneObject(const Guid& guid) const = 0;
};
```

### ISceneObjectReference

Serializer、Prefab、Editorが参照型を共通処理するための抽象です。
Editorには対象の表示名と参照種別だけを返し、型消去されたSceneObjectポインタは公開しません。

主な責務は次の通りです。

- GUIDの取得、設定、解除
- Inspectorから割り当て可能かの検証
- Editor用表示名の解決
- SceneObject複製時のGUIDリマップ
- SceneObject参照とTransform参照の種別表示

## SceneObjectRef&lt;T&gt;

特定のSceneObjectまたは派生型そのものを利用する参照です。

```cpp
CalyxEngine::SceneObjectRef<Player> targetPlayer;

if(auto player = targetPlayer.Resolve()) {
    player->ApplyDamage(10);
}
```

責務:

- 対象SceneObject GUIDの保持
- `T`による割り当て型検証
- `weak_ptr<T>`による非所有キャッシュ
- Resolverを使った遅延解決
- JSONへのGUID保存
- Prefab複製時のGUIDリマップ

Resolve時はweakキャッシュが生存していても、Resolverで現在のシーン所属を再確認します。
別のshared_ptrが削除済みObjectを延命している場合でも、シーンから削除されていればnullptrを
返すためです。SceneObjectLibraryのGUID検索はunordered_mapによるO(1)です。

## TransformRef

対象SceneObjectのWorldTransformだけを読む参照です。

```cpp
CalyxEngine::TransformRef targetTransform;

const WorldTransform* transform = targetTransform.Resolve();
if(!transform) return;

const CalyxEngine::Vector3 position = transform->GetWorldPosition();
```

内部構造:

```cpp
class TransformRef final : public ISceneObjectReference {
public:
    const WorldTransform* Resolve() const;
    const WorldTransform* Resolve(const ISceneObjectResolver& resolver) const;

private:
    SceneObjectRef<SceneObject> owner_;
};
```

内部でSceneObjectRefを使用する理由:

- WorldTransform自体にはGUIDがない
- WorldTransformの寿命は所有元SceneObjectに従う
- 既存のシーン検索、削除判定、Prefabリマップを再利用できる
- WorldTransformポインタをシーンデータへ保存しなくてよい

SceneObjectを外部へ返さない理由:

- 追跡や注視処理は通常、位置を読むだけで足りる
- 利用側がSceneObjectのゲーム機能や変更APIへ依存することを防ぐ
- Transform参照とオブジェクト参照の責務を明確にする

`ResolveMutable()`は提供しません。書き込み責務を持つ具体的な機能が必要になるまでは、
読み取り専用APIを維持します。

### nullptrになる条件

TransformRef::Resolve()は次の場合にnullptrを返します。

- GUIDが未設定
- SceneContextが存在しない
- GUIDが現在のシーンに存在しない
- 対象がシーンから削除済み
- 別シーンのGUIDを参照している

返されたWorldTransformポインタはフレームを越えて保存せず、利用する時点でResolveしてください。

## SerializableObjectでの使用方法

### Transformだけを参照するEnemy

```cpp
struct EnemyParams final : CalyxEngine::SerializableObject {
    EnemyParams() {
        AddField("targetTransform", targetTransform)
            .Category("Target")
            .Tooltip("追跡対象のTransform");

        AddField("moveSpeed", moveSpeed)
            .Category("Movement")
            .Range(0.0f, 100.0f);
    }

    CalyxEngine::TransformRef targetTransform;
    float moveSpeed = 5.0f;
};
```

```cpp
void Enemy::Initialize() {
    params_.LoadParams();
}

void Enemy::ShowGui() {
    params_.ShowGui();
}
```

```cpp
void Enemy::Update(float deltaTime) {
    const WorldTransform* targetTransform =
        params_.targetTransform.Resolve();

    if(!targetTransform) {
        return;
    }

    const CalyxEngine::Vector3 targetPosition =
        targetTransform->GetWorldPosition();

    // targetPositionを使って追跡処理を行う。
}
```

利用者は個別のJSON保存、Inspectorウィジェット、Prefabリマップ処理を書く必要がありません。
AddFieldされたISceneObjectReferenceは既存SerializableObject機構で共通処理されます。

## Editor連携

HierarchyのSceneObjectを参照フィールドへドラッグ＆ドロップして設定します。

表示例:

```text
Target Player      Demo Player (Scene Object)  [x]
Target Transform   Demo Player (Transform)     [x]
```

状態表示:

- 未設定: `None (Transform)`
- 解決成功: `ObjectName (Transform)`
- 削除済み／型不一致: `Missing / Type mismatch (Transform)`
- Missing項目のTooltip: 参照種別とGUID

EditorはISceneObjectReferenceだけへ依存します。SceneObjectRefとTransformRefで個別UIを
実装せず、共通フィールドを使用します。

## Serializer連携

SceneObjectRefとTransformRefは、どちらもGUID文字列として保存されます。

```json
{
  "fields": {
    "targetTransform": "01234567-89ab-cdef-0123-456789abcdef"
  }
}
```

保存対象にメモリアドレスやweak_ptrの状態は含まれません。ロード時はGUIDだけを復元し、
実体は初回Resolve時に遅延解決します。

## Prefab複製とGUIDリマップ

Prefabを複製すると、内部SceneObjectには新しいGUIDが割り当てられます。
参照GUIDを変更しなければ、複製後のEnemyが複製元Playerを指すなどの誤接続が発生します。

```text
複製元Player GUID  ──remap──>  複製先Player GUID
          ↑                         ↑
複製元Enemy参照                 複製先Enemy参照
```

PrefabSerializerが作成する`oldGuid -> newGuid`対応表を、ISceneObjectReference::Remapへ
渡して参照を更新します。TransformRefでは所有元SceneObjectのGUIDが更新されます。

## 検索APIとの使い分け

| 目的 | 使用する仕組み |
|---|---|
| 特定Playerのゲーム機能を使用 | `SceneObjectRef<Player>` |
| 特定対象のTransformだけを参照 | `TransformRef` |
| 最も近いPlayerを毎回選択 | TargetingSystemなどのQuery |
| シーン内の全Playerを列挙 | `FindByType<Player>()` |
| 親子関係を構築 | `SetParent()` |
| オブジェクト間通知 | EventBus |
| エディタ／デバッグ検索 | `FindByClassName()` |

クラス名検索の先頭要素をゲーム上の依存関係として使用すると、同型オブジェクトが増えた際に
対象が不定になります。固定関係にはEditorで設定した明示参照を使用してください。

## SOLID原則との対応

- SRP: 参照保持、GUID検索、Editor描画、保存、Prefabリマップを分離する
- OCP: 新しい参照ビューはISceneObjectReference実装として追加できる
- LSP: SceneObjectRefとTransformRefをEditor／Serializerから同じ参照として扱える
- ISP: Transform利用側にはWorldTransformだけ、Editorには表示情報だけを公開する
- DIP: 参照型はSceneObjectLibraryではなくISceneObjectResolverへ依存する

## 将来のComponentRef&lt;T&gt;

WorldTransformや他機能をComponent化した場合は、Component単位の永続参照へ拡張します。

```cpp
struct ComponentAddress {
    Guid objectGuid;
    Guid componentGuid;
};

template<class TComponent>
class ComponentRef {
private:
    ComponentAddress address_;
    mutable std::weak_ptr<TComponent> cache_;
};
```

同じSceneObjectへ同型Componentを複数追加できる場合、型名だけでは参照先を一意にできません。
したがって、所有元の`objectGuid`に加えて、Componentインスタンス固有の`componentGuid`が必要です。

移行方針:

1. Componentへ固有GUIDを付与する
2. ComponentAddressをSerializerとPrefab Remapへ対応させる
3. ComponentRef&lt;T&gt;をISceneObjectReference相当のEditor契約へ接続する
4. TransformRefの保存データをRoot Transform Componentへ移行する互換処理を用意する

現在のTransformRefはこの移行を妨げず、Component化までの安全な限定参照として使用できます。

## 実装上の確認項目

- TransformRefが所有元SceneObject GUIDとして保存される
- ロード後にGUIDからWorldTransformを解決できる
- 対象削除後にResolveがnullptrを返す
- Prefab複製時に複製先GUIDへRemapされる
- EditorでHierarchyから割り当て／解除できる
- UI上でScene Object参照とTransform参照を区別できる
- TransformRefからSceneObjectを取得できない
- 読み取り専用WorldTransformだけが公開される
