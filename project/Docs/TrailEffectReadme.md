# Trail Effect

## 概要

TrailはCPU Emitterの移動履歴を点列として保持し、連続形状を毎フレーム生成する機能です。従来の`Complement Trail`（補間位置へ独立Particleを生成する機能）とは併用できますが、連続したNoise、UV Scroll、Dissolveが必要な軌跡にはTrailを使用してください。

## Geometry Mode

- `Ribbon`: 点列の左右へ頂点を生成する標準リボンです。連続UVを必要とする矢、魔法弾、剣閃に適します。
- `Mesh Extrusion`: モデルのローカルXY断面を点列に沿って押し出します。ローカル+Zを進行方向として扱い、断面用の平坦で単純なモデルを推奨します。`Close Cross Section`で断面を閉じられます。
- `Mesh Instances`: モデル全体を各TrailPointへ複製し、同一Dynamic Bufferから一回で描画します。ローカル+Zを進行方向に合わせるか、記録時のEmitter回転を使えます。モデル本来のUVを維持するため、模様はインスタンス単位です。

モデルはProject Browserから`Geometry Model`へドロップします。モデルが未設定またはロード中の場合はRibbonへフォールバックします。通常のModelRendererは使わず、モデルのCPU MeshResourceをTrail頂点へ変換するため、Trail共通のNoise、Dissolve、Color、Emissive/Bloomを適用できます。

`Geometry Scale`はモデルローカル軸の倍率です。`Base Width`と`Width Over Lifetime`はExtrusionとInstancesのローカルXY断面へ適用され、後端を細くできます。

## サンプリングと寿命

`Min Sample Distance`以上移動するか、`Max Sample Interval`以上経過すると点を追加します。1フレームの移動が距離閾値を超えた場合は`ceil(distance / minSampleDistance)`で分割するため、高速移動でも大きな隙間を抑えます。

停止後も各点は`Lifetime`まで更新・描画されます。Scene切り替え時はFxSystemのClearで履歴とGPUバッファを破棄します。TrailPoint、時刻、Dynamic Bufferは保存しません。

## UVとテクスチャ

- `Distance`: 累積移動距離に`UV Tiling`を掛けます。速度が変化しても模様の密度を保ちます。
- `Stretch`: 現在の全長を0～1へ正規化します。

Base TextureとNoise Textureには個別のTilingとScroll Speedがあります。NoiseのRGはDistortion、RはMaskとDissolveへ使います。Noise未設定時は白テクスチャを安全にバインドします。Mesh InstancesだけはモデルUVを優先するため、Trail全長に連続する模様にはRibbonまたはMesh Extrusionを選択してください。

## Facing

- `Camera Facing`: カメラ方向を基準にSideを計算します。
- `Local Axis`: 点を記録した時点のEmitter回転とLocal Axisを使います。
- `World Axis`: 指定ワールド軸を基準にします。
- `Cross`: 90度異なる2枚のRibbonを同一バッファへ生成します。

Sideが縮退した場合は直前のSideを再利用し、内積が負なら反転してねじれを抑えます。

## 色、Dissolve、Bloom

Color、Alpha、Width、Emissiveは正規化寿命から毎フレーム再評価されるため、再生中の編集が既存Trailにも反映されます。Pixel ShaderはBase Mask、Noise、Distortion、Dissolve、Head/Tail Fadeの順に評価します。

通常色はScene Colorへ、Emissive成分はScene ColorとBloom Maskへ出力します。Trail PSOはHDR MRT、Depth Test有効、Depth Write無効、Cull Noneで、エンジン共通のBlendModeを使用します。

## 保存互換性

設定はEmitter JSONの`trail`へ保存します。Geometry Mode、Model GUID、Model Path、Scaleも保存対象です。旧Effectに`trail`が存在しない場合は`enabled=false`になるため、既存Particleの表示は変わりません。

## Projectileへの設定例

1. Projectileへ追従するCPU EmitterでTrailを有効にします。
2. 連続した帯はRibbon、立体断面はMesh Extrusion、数珠状のモデル列はMesh Instancesを選びます。
3. `Min Sample Distance`を0.05～0.15、`Lifetime`を0.2～0.6秒程度から調整します。
4. 流れる模様にはDistance UVとBase Scroll Xを使います。
5. Emissive IntensityとBloom Thresholdを見ながら発光量を調整します。

## パフォーマンス

バッファは容量不足時だけ2のべき乗へ拡張し、毎フレームのGPU Resource生成を避けています。Mesh Instancesは`モデル頂点数 × TrailPoint数`になるため、低ポリゴンモデルと適切なMax Point Countを使用してください。大量Trail向けのGPU Driven生成、Frustum Cull、マテリアル単位Batchは将来拡張です。
