# Spriteの分割画像選択 / モデルの連番再生API

1枚の画像を等間隔の列・行に分割して表示します。フレーム番号は0始まりで、左上から右へ、次の行へ進みます。余白やセル間隔のないシートを使用してください。

Spriteの数字選択は `Sprite` / `SpriteObject2d` の直接APIで行います。時間による再生はありません。モデルの連番再生は `BaseGameObject::GetTextureAnimator()` を使用します。モデルは通常のUVが0〜1のPlaneなどを想定しています。テクスチャパスは `Resources/Assets` からの相対パスです（例: `Textures/digits.png`）。

## 数字を表示する

```cpp
// 初期化時。横一列に0〜9を配置した画像。
sprite->SetTextureSheet("Textures/digits.png", 10, 1);
sprite->SetTextureFrame(0);

// カウントの変更時。UVの区画のみを変更し、指定した数字で固定される。
sprite->SetTextureFrame(count % 10);
```

画像を設定済みなら `SetTextureGrid(10, 1)` で分割数だけを設定できます。区画番号は `SetTextureFrame` で指定します。複数桁は桁ごとにSpriteを用意して、十の位なら `(count / 10) % 10` を指定します。表示サイズは `SpriteObject2d::SetScale()` で設定してください。シート設定は表示サイズを変更しません。

## Planeのリアクションを再生する

```cpp
// plane はモデルを設定済みの BaseGameObject。
auto& reaction = plane->GetTextureAnimator();
reaction.SetTextureSheet("Textures/reactions.png", 4, 2);
reaction.PlayFrames(0, 4, 0.1f, false); // 0〜3を各0.1秒、1回再生

// 別のリアクションに切り替えるタイミングで呼ぶ。
reaction.PlayFrames(4, 4, 0.08f, false);

// 更新処理などで終了を調べる。終了後は最後のコマを保持する。
if(reaction.IsFinished()) {
    reaction.ShowFrame(0);
}

// 別シートへの切り替え。先頭フレームに戻り、再生は停止する。
reaction.SetTextureSheet("Textures/idle.png", 6, 1);
reaction.PlayFrames(0, 6, 0.15f, true);

// 通常の1枚絵へ戻す。再生を停止し、UVを画像全体に戻す。
reaction.SetTexture("Textures/neutral.png");
```

`PlayFrames` は呼ぶたびに再スタートします。毎フレームではなく、状態が切り替わったときに呼びます。モデルのコード指定テクスチャは共有マテリアルの画像より優先され、他のモデルのマテリアルアセットは変更しません。Material Graphでは `ObjectTexture` が対象で、独自の `Texture2D` ノードは元の指定を維持します。

## 既存のアニメーションアセットを使う

```cpp
auto& animation = plane->GetTextureAnimator();
animation.SetAnimationAsset(asset);          // shared_ptr<SpriteAnimationAsset>
animation.Play("Idle");
animation.Play("Hit", false); // 同じクリップなら時刻を維持
animation.SetReversed(true);
animation.Play("Hit");        // 終端から逆再生
animation.Stop();             // 現在位置で停止
animation.Reset();            // クリップ開始位置へ戻して停止
```

`SetLoopOverride` / `ClearLoopOverride` も使用できます。`PlayFrames` は引数のloopを採用するため、既存のループOverrideを解除します。逆再生の設定はシート切り替え後も保持します。

## 更新と入力の扱い

- モデルが所有するAnimatorは `AlwaysUpdate(dt)` が更新します。派生クラスで上書きした場合は基底処理も呼びます。Animatorを重複更新しないでください。
- 単独の `SpriteAnimator2d` を使う場合は `Bind(sprite)` または `BindModel(actor)` 後、毎フレーム `Update(dt)` を呼びます。対象オブジェクトはAnimatorより長く生存させてください。
- 既存の `AnimatedSpriteSceneObject2d` の再生機能は維持しています。数字の固定表示には通常の `SpriteObject2d` を使用してください。
- `SetTextureSheet` / `PlayFrames` / `Play` は不正な入力でfalseを返し、既存の再生を維持します。分割数は各軸1〜46340です。画像のロード成功を表す戻り値ではありません。
- `ShowFrame` の範囲外番号はシート内に丸めます。負数は0になります。負数・NaN・無限大のdtは無視します。
- 既存のオブジェクトの `SetTexture` はUVと再生状態を保持します。モデルの連番から通常画像へ戻すときは `GetTextureAnimator().SetTexture(...)` を使用してください。Spriteでは `SetTextureSheet(path, 1, 1)` で画像全体に戻せます。
- 今回の追加APIは実行時の設定です。シーン保存用の新しいフィールドやUIは追加していません。

モデルに以前からあった `UpdateTexture` は複数の画像ハンドルを切り替える処理です。今回のAPIは1枚の画像のUVを切り替えます。コードからテクスチャを指定したモデルでは旧ハンドル再生による上書きを抑止します。

## 再生処理のテスト

Develop / x64でソリューションをビルド後、Visual Studioのx64 Native Tools Command Promptから `Tools\Tests\RunTextureSheetAnimationTests.cmd` を実行します。単発再生の終端保持、逆再生、ループ、大きなdt、不正入力、アセット共有の維持を検証します。GPU描画の見た目は対象外です。
