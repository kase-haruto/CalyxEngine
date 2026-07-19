# Effect Module Guide

EffectのEmitterにはモジュールを追加でき、パーティクルの更新処理を組み合わせて動きや見た目を作れます。EditorではEmitterの `Modules` セクションにある `Add Modules` から追加します。追加済みモジュールはチェックボックスで一時的に無効化でき、`Remove` で削除できます。

## モジュールの適用ルール

- 有効なモジュールだけがパーティクルごとに毎フレーム実行されます。
- CPU Effectでは、Editorに表示されている追加順（内部のモジュール配列順）に適用されます。同じ値を複数モジュールが変更する場合は順序によって結果が変わります。
- `OverLifetimeModule` は複数追加できます。それ以外のモジュールは同じEmitterに1つだけ追加できます。
- 設定はEffectのJSONへ保存され、再読み込み時にも復元されます。
- GPU EffectはCPU Effectと対応状況が異なります。詳細は「GPU Effectでの対応」を参照してください。

## GravityModule

パーティクルの速度へ、指定した加速度を毎フレーム加算します。落下、上昇気流、横風などを表現できます。

更新式は概ね `velocity += strength * deltaTime` です。

| パラメータ | 内容 | 初期値 |
| --- | --- | --- |
| `strength` | X/Y/Z方向の加速度。Yを負にすると下向き、正にすると上向きに加速します。 | `(0, -9.8, 0)` |

使用例:

- 火花を徐々に落下させる: Yを負にする
- 煙を上昇させる: Yを正にする
- 風に流す: XまたはZへ値を入れる

## SizeOverLifetimeModule

寿命の進行度 `age / lifetime` に合わせ、生成時のスケールを基準にサイズを変化させます。

| パラメータ | 内容 | 初期値 |
| --- | --- | --- |
| `isGrowing` | `true` なら0倍から生成時サイズへ拡大、`false` なら生成時サイズから0倍へ縮小します。 | `true` |
| Ease | サイズ変化に使用するイージング関数です。 | `EaseInOutCubic` |

このモジュールは常に `initialScale` を基準にスケールを設定します。Scaleを操作する `OverLifetimeModule` と併用する場合、CPU Effectでは後に適用されたモジュールの結果が最終値に強く影響します。

## TextureSheetAnimationModule

1枚のテクスチャを等間隔のグリッドに分割し、パーティクルの経過時間に応じてUVフレームを切り替えます。爆発、煙、魔法陣などの連番画像に使用します。フレームは左上から行方向へ進む並びとして計算されます。

| パラメータ | 内容 | 初期値 |
| --- | --- | --- |
| `Rows` | テクスチャを縦に分割する数です。 | `4` |
| `Cols` | テクスチャを横に分割する数です。 | `4` |
| `Loop` | 最終フレーム後に先頭へ戻るかを指定します。無効時は最終フレームで停止します。 | `true` |
| `Animation Speed (fps)` | 1秒間に進むフレーム数です。 | `10.0` |

コードからは任意のUV範囲を並べるカスタムフレームも設定できますが、現在のEditor UIではカスタムフレームの編集には対応していません。また、Rows/Colsには1以上の値を設定してください。

## OverLifetimeModule

寿命の開始から終了まで `Start` と `End` を補間し、指定したプロパティへ適用する汎用モジュールです。複数追加できるため、たとえば「拡大しながらフェードアウト」のような合成ができます。

| パラメータ | 内容 | 初期値 |
| --- | --- | --- |
| `Target` | 変化させる対象です。下表を参照してください。 | `Scale` |
| `Blend` | 補間値を対象へ適用する方法です。`Set`、`Add`、`Multiply` から選びます。 | `Set` |
| `Ease` | StartからEndへの補間カーブです。 | `EaseInOutCubic` |
| `Start` / `End` | 寿命の開始時と終了時の値です。Targetに応じて入力欄が変わります。 | `(0,0,0,1)` / `(1,1,1,1)` |
| `Clamp 0..1` | 寿命の進行度を0～1へ制限します。 | `true` |
| `Invert (1 - t)` | 寿命の進行方向を反転します。 | `false` |

Targetの種類:

| Target | 変化する値 | 備考 |
| --- | --- | --- |
| `Scale` | XYZスケール | Start/EndのXYZを使用します。 |
| `RotationX` | X回転 | 値の単位は度です。 |
| `RotationY` | Y回転 | 値の単位は度です。 |
| `RotationZ` | Z回転 | 値の単位は度です。 |
| `ColorRGBA` | RGBAカラー | カラーピッカーで設定します。 |
| `AlphaOnly` | Alphaのみ | Start/EndのX成分をAlpha値として使用します。 |

Blendの意味:

- `Set`: 現在値を補間値で置き換えます。
- `Add`: 現在値へ補間値を加算します。
- `Multiply`: 現在値へ補間値を乗算します。元の色やスケールを活かした変化に向いています。

代表的な構成例:

- フェードアウト: Targetを `AlphaOnly`、Startを1、Endを0、Blendを `Set` にする
- 色の遷移: Targetを `ColorRGBA` にしてStart/Endの色を設定する
- 拡大: Targetを `Scale`、Startを小さく、Endを大きくする
- 拡大しながらフェードアウト: Scale用とAlphaOnly用の2つを追加する

## GPU Effectでの対応

| モジュール | GPU対応 | 制限 |
| --- | --- | --- |
| `GravityModule` | 対応 | XYZの加速度を適用できます。 |
| `SizeOverLifetimeModule` | 対応 | 成長方向とEaseを適用できます。 |
| `TextureSheetAnimationModule` | 非対応 | GPU Emitterの設定には反映・保存されません。 |
| `OverLifetimeModule` | 一部対応 | `Scale`、`ColorRGBA`、`AlphaOnly` のみ対応します。Rotation X/Y/ZはGPUでは反映されません。GPU側で保持できるOverLifetime設定は1つです。 |

CPUとGPUの両方で同じEffect設定を使う場合は、上表の共通機能だけで構成してください。

## 新しいモジュールを追加する場合

新規モジュールは `BaseFxModule` を継承して更新処理とEditor UIを実装し、対応するConfig、Factoryの生成・変換処理、追加モジュール一覧を登録します。GPUでも使用する場合は、GPU Emitterデータとシェーダー側にも同等の処理が必要です。追加後は、このREADMEと追加ボタンのツールチップも更新してください。
