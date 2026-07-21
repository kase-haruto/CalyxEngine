# Sprite Sorting Layer 設計

## 1. 目的

2Dオブジェクトの描画順を、オブジェクトの列挙順に依存せず明示的に管理する。

現在の2D描画では、`SceneObjectLibrary::GetAllObjectsRaw()` が返した順番でSpriteを`SpriteRenderer`へ登録している。`SceneObjectLibrary`の内部コンテナは`std::unordered_map`であるため、その列挙順は描画順として利用できない。

本設計では、次の2段階で描画順を決定する。

- **Sorting Layer**: Background、Character、UIなど、用途ごとの大きな描画グループ
- **Order in Layer**: 同一Sorting Layer内での個別の描画順

値が小さいものから先に描画し、値が大きいものほど手前に表示する。

## 2. 描画順の決定規則

Spriteは次の優先順位で比較する。

1. Sorting Layerの`order`
2. Spriteの`orderInLayer`
3. 同値時の`stableOrder`

通常のアルファブレンドでは後から描画されたSpriteが手前に見えるため、各値が大きいSpriteほど手前になる。

```text
奥
  Background / 0
  Character  / 0
  Character  / 10
  Effect     / 0
  UI         / 0
手前
```

`orderInLayer`は別のLayerを越えない。例えばCharacter LayerがUI Layerより奥に設定されている場合、`Character / 9999`は`UI / -9999`より先に描画される。

## 3. データ構造

### Sorting Layer

```cpp
using SortingLayerId = uint16_t;

struct SortingLayer {
    SortingLayerId id;
    std::string name;
    int32_t order;
};
```

- `id`: SceneやPrefabへ保存する不変の識別子
- `name`: Inspectorなどへ表示する名前
- `order`: Layer同士の描画順

Layer名ではなくIDをオブジェクトへ保存する。これにより、LayerをリネームしてもSceneやPrefabの更新は不要になる。

Layerを並べ替える場合もIDは変更せず、Layer設定の`order`だけを変更する。

### Spriteの描画情報

```cpp
struct SpriteSortKey {
    int32_t sortingLayerOrder = 0;
    int32_t orderInLayer = 0;
    uint64_t stableOrder = 0;
};

struct SpriteDrawEntry {
    Sprite* sprite = nullptr;
    SpriteSortKey sortKey;
};
```

`stableOrder`はLayerとOrderが同じSpriteの描画順を決定的にするために使用する。SceneObjectのGUIDから生成した値、または永続化可能な生成連番を使用し、実行ごとに変化するポインタ値は使用しない。

## 4. Layer設定例

| Layer | order | 用途 |
|---|---:|---|
| Background | -100 | 背景 |
| Default | 0 | 未分類のSprite |
| Character | 100 | プレイヤーや敵 |
| Effect | 200 | 2Dエフェクト |
| UI | 1000 | HUDやメニュー |

ID 0は`Default`として予約し、Layer情報を持たない既存データの読み込み先として使用する。

## 5. 使用方法

### Inspectorから設定する

`SpriteSceneObject2d`のInspectorに次の項目を表示する。

```text
Rendering
  Sorting Layer : Character
  Order in Layer: 10
```

利用者はLayerとLayer内順序を指定するだけでよく、`SpriteRenderer`へ直接登録する必要はない。

### C++から設定する

```cpp
auto* background = sceneContext->Instantiate<SpriteSceneObject2d>();
background->SetSortingLayer(SortingLayers::Background);
background->SetOrderInLayer(0);

auto* player = sceneContext->Instantiate<SpriteSceneObject2d>();
player->SetSortingLayer(SortingLayers::Character);
player->SetOrderInLayer(10);

auto* healthBar = sceneContext->Instantiate<SpriteSceneObject2d>();
healthBar->SetSortingLayer(SortingLayers::UI);
healthBar->SetOrderInLayer(20);
```

文字列によるLayer指定を補助APIとして提供してもよいが、内部処理と永続化には必ず`SortingLayerId`を使用する。

### 複数Spriteを持つオブジェクト

1つのオブジェクトが複数のSpriteを登録する場合、それぞれに異なる`orderInLayer`を指定できる。

```cpp
void PlayerHud::SubmitSprites(SpriteRenderer& renderer) const {
    renderer.Register(iconBackground_.get(), SortingLayers::UI, 10, stableId_ + 0);
    renderer.Register(icon_.get(),           SortingLayers::UI, 20, stableId_ + 1);
    renderer.Register(frame_.get(),          SortingLayers::UI, 30, stableId_ + 2);
}
```

## 6. SpriteRendererの責務

`SpriteRenderer`は`Sprite*`だけでなく、ソート情報を含む描画エントリを一時的に保持する。

```cpp
void Register(
    Sprite* sprite,
    SortingLayerId layerId,
    int32_t orderInLayer,
    uint64_t stableOrder);
```

描画直前にエントリをソートする。

```cpp
std::stable_sort(entries_.begin(), entries_.end(),
    [](const SpriteDrawEntry& a, const SpriteDrawEntry& b) {
        if (a.sortKey.sortingLayerOrder != b.sortKey.sortingLayerOrder) {
            return a.sortKey.sortingLayerOrder < b.sortKey.sortingLayerOrder;
        }
        if (a.sortKey.orderInLayer != b.sortKey.orderInLayer) {
            return a.sortKey.orderInLayer < b.sortKey.orderInLayer;
        }
        return a.sortKey.stableOrder < b.sortKey.stableOrder;
    });
```

`SceneObjectLibrary`のコンテナを描画順のために変更してはならない。ライブラリの順序変更は更新処理、シリアライズ、3Dオブジェクトなどにも影響するため、2D描画順の管理は`SpriteRenderer`内で完結させる。

## 7. SpriteSceneObject2dの責務

`SpriteSceneObject2d`は所属LayerとLayer内順序を保持する。

```cpp
class SpriteSceneObject2d {
public:
    void SetSortingLayer(SortingLayerId layerId);
    SortingLayerId GetSortingLayer() const;

    void SetOrderInLayer(int32_t order);
    int32_t GetOrderInLayer() const;

private:
    SortingLayerId sortingLayerId_ = kDefaultSortingLayerId;
    int32_t orderInLayer_ = 0;
};
```

登録処理では保持している設定をRendererへ渡す。

```cpp
void SpriteSceneObject2d::SubmitSprites(SpriteRenderer& renderer) const {
    if (!sprite_ || !IsDrawEnable()) {
        return;
    }

    renderer.Register(
        sprite_->GetSprite(),
        sortingLayerId_,
        orderInLayer_,
        MakeStableOrder(GetGuid()));
}
```

## 8. 保存形式

SceneおよびPrefabには、Layer IDとLayer内順序を保存する。

```json
{
  "sortingLayerId": 3,
  "orderInLayer": 10
}
```

読み込み時の規則は次のとおり。

- `sortingLayerId`が存在しない旧データはDefault Layerへ割り当てる
- `orderInLayer`が存在しない旧データは0とする
- 削除済みまたは不正なLayer IDはDefault Layerへフォールバックする

## 9. EditorでのLayer管理

Layer設定画面では、次の操作を提供する。

- Layerの追加
- Layer名の変更
- Layerの並べ替え
- Layerの削除
- Default Layerの表示

一覧は「上が手前」または「下が手前」のどちらかに統一し、画面内に方向を明記する。既存の描画規則が小さい値から先に描画するため、UIを「上が手前」とする場合は表示順と内部描画順が逆になる点に注意する。

Default Layerは削除不可とする。使用中Layerを削除する場合は、そのLayerを参照していたSpriteをDefaultへ移すか、読み込み時にDefaultへフォールバックさせる。

## 10. 処理の流れ

```text
SpriteSceneObject2d
  └─ SortingLayerIdとOrder in Layerを保持・保存
       ↓
ISpriteRenderable::SubmitSprites()
       ↓
SpriteRenderer::Register(sprite, layerId, order, stableOrder)
       ↓
Sorting Layer order → Order in Layer → stableOrderでソート
       ↓
Sprite::Draw()
```

## 11. 実装順序

1. `SortingLayerId`、Default Layer、Layer設定データを追加する
2. `SpriteRenderer`の登録データを`SpriteDrawEntry`へ変更する
3. 描画直前の安定ソートを追加する
4. `SpriteSceneObject2d`へLayerとOrderを追加する
5. Scene／Prefabの保存と旧データのフォールバックを追加する
6. InspectorへLayerとOrderの編集UIを追加する
7. Layer設定画面の追加・削除・リネーム・並べ替えを実装する
8. 同値、Layer削除、旧データ読み込みをテストする

