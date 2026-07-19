# Effect Module Guide

EffectのEmitterにはモジュールを追加でき、パーティクルの更新処理を組み合わせて動きや見た目を作れます。EditorではEmitterの `Modules` セクションにある `Add Modules` から追加します。追加済みモジュールはチェックボックスで一時的に無効化でき、`Remove` で削除できます。

## モジュールの適用ルール

- モジュールはSpawn/Initialize/Updateの実行ステージ別に整理されます。現在Editorから追加できるモジュールはUpdateステージで、有効なものだけがパーティクルごとに毎フレーム実行されます。
- 同じステージ内ではEditorに表示されている追加順（内部のモジュール配列順）に適用されます。同じ値を複数モジュールが変更する場合は順序によって結果が変わります。
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

## AccelerationModule

指定した一定加速度を速度へ毎フレーム加算します。更新式は `velocity += acceleration * deltaTime` で、XYZ軸ごとに値を設定できます。

| パラメータ | 内容 | 初期値 |
| --- | --- | --- |
| `Acceleration` | XYZ方向の加速度です。正負の値で加速方向を指定します。 | `(0, 0, 0)` |

動作はGravityModuleと同じ積分方式ですが、重力という用途に限定しない移動制御として使用できます。GravityModuleと併用した場合は両方の加速度が速度へ加算されます。

## DragModule

空気抵抗のように速度を指数減衰させます。フレームレートに依存しにくい `velocity *= exp(-drag * deltaTime)` で計算されます。

| パラメータ | 内容 | 初期値 |
| --- | --- | --- |
| `Drag` | 0以上の減衰係数です。0では減速せず、大きいほど短時間で速度が0へ近づきます。 | `0.0` |

AccelerationやGravityと併用すると、加速しながら抵抗を受ける動きになります。Dragを後に配置すると、そのフレームで加算された速度にも減衰が適用されます。

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

## Lifetimeカーブモジュール共通仕様

以下の6モジュールは寿命進行度 `lifeT`（0～1）で値を評価します。数値を扱う項目では次のModeを選択できます。

| Mode | 内容 |
| --- | --- |
| `Constant` | 寿命全体で `Value`（XYZでは各軸の値）を使用します。 |
| `Linear` | 登録されたキー間を線形補間します。 |
| `Curve` | 現在はLinearと同じキー間線形補間です。 |
| `Random Constants` | パーティクルごとにMin～Maxから決定した値を使用します。 |
| `Random Curves` | MinカーブとMaxカーブを評価し、その間からパーティクルごとに決定します。 |

ランダム値はパーティクルのSeedから決定されるため、毎フレーム無秩序に変化するものではありません。現在のEditor UIはMode、Value、Min、Maxと開始・終了色を編集でき、カーブのキー列を直接編集するUIはまだありません。

## ColorOverLifetimeModule

寿命に合わせてRGBAカラーを開始色から終了色へ補間し、パーティクルの現在色を置き換えます。

| パラメータ | 内容 | 初期値 |
| --- | --- | --- |
| `Start Color` | 寿命開始時のRGBAカラーです。 | 白 `(1, 1, 1, 1)` |
| `End Color` | 寿命終了時のRGBAカラーです。 | 白 `(1, 1, 1, 1)` |

## AlphaOverLifetimeModule

寿命に合わせてAlphaのみを評価し、現在色のAlphaを置き換えます。RGBは変更しません。ModeとValue/Min/Maxは「Lifetimeカーブモジュール共通仕様」に従います。開始値1、終了値0のフェードアウトカーブなど、キー編集が必要な設定は現在JSONまたはコードから指定します。

## SizeCurveOverLifetimeModule

寿命に合わせてXYZそれぞれの倍率を評価し、`initialScale * 評価値` を現在スケールへ設定します。

- X/Y/ZごとにMode、Value、Min、Maxを設定できます。
- `(1, 1, 1)` は生成時サイズ、`(0, 0, 0)` は完全に縮小した状態です。
- `SizeOverLifetimeModule` やScale対象の `OverLifetimeModule` と値が競合するため、併用時は追加順に注意してください。

## RotationOverLifetimeModule

寿命に合わせてXYZの回転値を評価し、現在のオイラー回転を置き換えます。各軸でMode、Value、Min、Maxを設定できます。回転値は度として扱われます。

## VelocityOverLifetimeModule

寿命に合わせてXYZの速度を評価し、現在速度を置き換えます。各軸でMode、Value、Min、Maxを設定できます。

Gravity、Acceleration、Dragも速度を操作します。このモジュールをそれらより後に置くと、それ以前の計算結果を評価値で上書きします。逆に先へ置くと、評価した速度へ後続の加速や減衰を合成できます。

## EmissiveOverLifetimeModule

寿命に合わせて発光強度と発光色を設定します。

| パラメータ | 内容 | 初期値 |
| --- | --- | --- |
| Mode / `Value` / `Min` / `Max` | 発光強度の評価方法と値です。 | Constant / `1.0` |
| `Emissive Start` | 寿命開始時の発光色です。 | 白 `(1, 1, 1, 1)` |
| `Emissive End` | 寿命終了時の発光色です。 | 白 `(1, 1, 1, 1)` |

見た目への反映には、使用している描画経路・マテリアルが `emissiveIntensity` と `emissiveColor` に対応している必要があります。

## GPU Effectでの対応

| モジュール | GPU対応 | 制限 |
| --- | --- | --- |
| `GravityModule` | 対応 | XYZの加速度を適用できます。 |
| `AccelerationModule` | 非対応 | GPU Emitterの設定には反映・保存されません。 |
| `DragModule` | 非対応 | GPU Emitterの設定には反映・保存されません。 |
| `SizeOverLifetimeModule` | 対応 | 成長方向とEaseを適用できます。 |
| `TextureSheetAnimationModule` | 非対応 | GPU Emitterの設定には反映・保存されません。 |
| `OverLifetimeModule` | 一部対応 | `Scale`、`ColorRGBA`、`AlphaOnly` のみ対応します。Rotation X/Y/ZはGPUでは反映されません。GPU側で保持できるOverLifetime設定は1つです。 |
| Lifetimeカーブモジュール6種 | 非対応 | Color、Alpha、Size、Rotation、Velocity、Emissiveの各設定はGPU Emitterへ反映・保存されません。 |

CPUとGPUの両方で同じEffect設定を使う場合は、上表の共通機能だけで構成してください。

## Noiseの使用について

Particle EmitterのMaterialセクションにある `Noise Mask` を有効にすると、Proceduralな2D value noiseでパーティクルのAlphaをマスクできます。CPU EffectとGPU Effectの両方で利用でき、Effect JSONへ設定が保存されます。追加のNoise Textureアセットは必要ありません。

| パラメータ | 内容 | 初期値 |
| --- | --- | --- |
| `Enabled` | Noise Maskの有効・無効を切り替えます。 | `false` |
| `Scale` | ノイズ模様の細かさです。小さいほど大きな模様、大きいほど細かな模様になります。 | `8.0` |
| `Strength` | 元のAlphaへマスクを混ぜる強さです。0では変化せず、1で完全にマスクします。 | `1.0` |
| `Threshold` | 表示されるノイズ領域の境界です。値を上げるほど表示領域が減ります。 | `0.5` |
| `Softness` | Threshold境界のぼかし幅です。小さいほど輪郭が硬く、大きいほど滑らかになります。 | `0.1` |
| `Scroll Speed` | ノイズUVを1秒あたりに移動させるX/Y速度です。0なら模様は静止します。 | `(0, 0)` |

基本的な作成手順:

1. EmitterのMaterialで通常のParticle TextureとColorを設定します。
2. `Noise Mask > Enabled` を有効にします。
3. `Scale` で模様の大きさを決めます。
4. `Threshold` を動かして残す領域を調整します。
5. 輪郭が硬すぎる場合は `Softness` を上げます。
6. 煙、炎、魔法など動く模様には `Scroll Speed` を設定します。

構成例:

- ちぎれた煙: Scale `5`、Threshold `0.45`、Softness `0.2`、Scroll Speed `(0, -0.15)`
- 荒い炎: Scale `10`、Threshold `0.55`、Softness `0.08`、Scroll Speed `(0, -0.5)`
- ノイズ状の点滅: Strengthを低めにして、元のテクスチャ形状を残しながらAlphaへむらを加える

Noise Maskは変換後のParticle UVを使用するため、`TextureSheetAnimationModule` と併用した場合は現在のフリップブックフレーム内のUVへノイズが適用されます。マスクはTexture、Emitter Color、Lifetime Color/Alphaを合成した後のAlphaへ乗算されます。

現在、位置や速度へノイズを加える `NoiseModule`、`Turbulence`、`Curl Noise` と、UVを歪ませるNoise Distortionは未実装です。Noise Maskは見た目のAlphaだけを変更し、パーティクルの移動には影響しません。

通常の3D Materialで見た目にノイズを使用する場合はMaterial Node Editorの `Noise Texture` も利用できます。具体的な接続方法は [Material README](../Resources/Assets/Materials/README.md#noise-textureの使用方法) を参照してください。

## 新しいモジュールを追加する場合

新規モジュールは `BaseFxModule` を継承して更新処理とEditor UIを実装し、対応するConfig、Factoryの生成・変換処理、追加モジュール一覧を登録します。GPUでも使用する場合は、GPU Emitterデータとシェーダー側にも同等の処理が必要です。追加後は、このREADMEと追加ボタンのツールチップも更新してください。
