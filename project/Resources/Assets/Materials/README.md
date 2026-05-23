# Material Editor / Shader Graph

このフォルダには、マテリアル用の `.mat` ファイルを配置します。

マテリアルは `EditMode` の `Material` から編集します。
`Material Graph` パネルでは、ノードを接続して `Output` に渡すことで、エディタ上のマテリアル値と実行時に使うピクセルシェーダを生成できます。

## 目的

マテリアルを固定パラメータだけで切り替えるのではなく、ノードとして組み立てた結果から描画用の shader graph を生成できるようにします。

現在は、`Toon Master`, `Lit Master`, `Unlit Master` を中心に、色、数値、テクスチャ、基本演算を接続できます。
`Output.Surface` に Master ノードを接続した場合は、古い `Lighting Mode` 入力ではなく、Master ノードが描画モデルを決めます。

## 全体の流れ

1. `EditMode` から `Material` を選択します。
2. `Material Graph` パネルで `.mat` を選択、または `New` で作成します。
3. `Add Node` から `Toon Master`, `Lit Master`, `Unlit Master` のいずれかを追加します。
4. Master ノードの `Surface` を `Output.Surface` に接続します。
5. `Color`, `Float`, `Texture Sample`, 演算ノードを追加して Master ノードの入力へ接続します。
6. `Save` で `.mat` に保存します。
7. `Build Runtime`、または実行時描画で、グラフから HLSL と PSO が生成されます。

例として、Toon 用の基本構成は次のようになります。

```text
Color        -> Toon Master / Base Color
Float        -> Toon Master / Base Step
Float        -> Toon Master / Shade Feather
Toon Master  -> Output / Surface
```

テクスチャを使う場合は、次のように `Texture Sample` を色入力へ接続します。

```text
Object Texture -> Texture Sample -> Multiply Color -> Toon Master / Base Color
Color          -------------------/
```

UV スクロールの例:

```text
UV -> Split Float2 / Value
Time -> Multiply Float / A
Float(0.05) -> Multiply Float / B
Split.X -> Add Float / A
Multiply Float -> Add Float / B
Add Float -> Frac Float -> Combine Float2 / X
Split.Y -> Combine Float2 / Y
Combine Float2 -> Texture Sample / UV
Object Texture -> Texture Sample / Texture
Texture Sample / Color -> Toon Master / Base Color
Toon Master / Surface -> Output / Surface
```

## 主要ファイル

- `Engine/Application/UI/Panels/MaterialNodeEditorPanel.*`
  - マテリアルエディタ UI です。
  - `.mat` の保存、読み込み、ノード追加、プレビュー、Runtime Build を扱います。

- `Engine/Editor/NodeEditor/NodeGraph.*`
  - ノード、ピン、リンクのデータ構造です。

- `Engine/Editor/NodeEditor/NodeEditorCanvas.*`
  - ノードの描画、接続、選択、キャンバス操作を扱います。

- `Engine/Graphics/MaterialGraph/MaterialGraphCompiler.h`
  - ノードグラフを CPU 側の `CompiledMaterialGraph` に変換します。
  - エディタ上のプレビュー値や、古いマテリアル値への反映に使います。

- `Engine/Graphics/MaterialGraph/ShaderGraphCodeGenerator.h`
  - ノードグラフから HLSL の `EvaluateGeneratedMaterial()` を生成します。
  - Object3D 用の実行時 Pixel Shader ソースもここで生成します。

- `Engine/Graphics/MaterialGraph/MaterialGraphRuntimeShaderCache.h`
  - 生成された HLSL を DXC でコンパイルし、ハッシュごとにキャッシュします。

- `Engine/Graphics/MaterialGraph/ShaderGraphSchema.h`
  - Master ノードや Texture ノードなど、shader graph として扱うノード定義を持ちます。

- `Engine/Graphics/MaterialGraph/ShaderGraphValidator.h`
  - グラフの接続状態や型の検証を行います。

- `Engine/Graphics/Pipeline/Factory/PsoFactory.*`
  - 生成済み Pixel Shader Blob を使った PSO 作成を行います。

- `Engine/Graphics/Pipeline/Service/PipelineService.*`
  - shader graph 用の Object3D PipelineSet をキャッシュします。

- `Engine/Renderer/Model/ModelRenderer.*`
  - `Output.Surface` を持つ static model のマテリアルに対して、生成 shader graph の PSO を使用します。

- `Resources/Assets/Materials/*.mat`
  - エディタで保存されるマテリアルアセットです。

## 使用できるノード

### 値ノード

- `Color`
  - RGBA の色を出力します。

- `Float`
  - 1つの float 値を出力します。

- `Shininess`
  - 旧仕様互換の光沢値です。

- `Roughness`
  - 旧仕様互換の粗さです。

- `Reflect`
  - 環境反射の有効、無効を出力します。

### Master ノード

- `Toon Master`
  - アニメ調の段階影、ハイライト、スペキュラを持つ Toon 用 Master ノードです。
  - `Base Color`, `Highlight Color`, `Base Tone`, `First Shade`, `Second Shade`, `Base Step`, `Shade Step`, `Feather`, `Spec` 系の入力を持ちます。

- `Lit Master`
  - 通常ライティング用の Master ノードです。
  - `Base Color`, `Shininess`, `Roughness`, `Reflect` を入力できます。

- `Unlit Master`
  - ライティングを行わず、入力色をそのまま描画する Master ノードです。

### テクスチャノード

- `Object Texture`
  - Object3D が持つ既存のテクスチャを表す入力ノードです。

- `Texture Sample`
  - `gTexture` をサンプリングし、色として出力します。
  - `UV` 入力に `Float2` を接続した場合はその UV を使います。
  - 未接続の場合は Object3D の標準 UV を使います。

- `Noise Texture`
  - HLSL 内で生成する 2D value noise を出力します。
  - `Color` はグレースケール色、`Value` は 0..1 の float です。
  - `UV` 入力が未接続の場合は Object3D の標準 UV を使います。
  - `Scale` 入力が未接続の場合はノード内の Scale 値を使います。

### Vector ノード

- `Float2`
  - 2成分の float 値を出力します。
  - 主に UV 操作用です。

- `Combine Float2`
  - 2つの float から `float2(X, Y)` を作ります。

- `Split Float2`
  - `Float2` を `X` と `Y` の float に分解します。

### シェーダ入力ノード

- `UV X`, `UV Y`
  - Object3D の UV 座標を float として出力します。

- `UV`
  - Object3D の UV 座標を `Float2` として出力します。
  - `Texture Sample` の `UV` 入力へ直接接続できます。

- `Time`
  - 現在の経過時間を float として出力します。
  - 実装上は Object3D の `MaterialConstants` にある `pad3` を shader graph 用 time として使っています。

- `World Position X/Y/Z`
  - ピクセルのワールド座標を成分ごとに出力します。

- `World Normal X/Y/Z`
  - ピクセルの法線を成分ごとに出力します。

- `View Direction X/Y/Z`
  - ピクセルからカメラ方向へのベクトルを成分ごとに出力します。

### 演算ノード

- `Multiply Color`
  - 2つの色を乗算します。

- `Lerp Color`
  - 2つの色を float の `T` で補間します。

- `Add Float`
  - 2つの float を加算します。

- `Subtract Float`
  - 2つの float を減算します。

- `Multiply Float`
  - 2つの float を乗算します。

- `Divide Float`
  - 2つの float を除算します。
  - 0 除算を避けるため、分母は最小値で保護されます。

- `Power Float`
  - `pow(A, B)` を計算します。
  - 負値による不安定な結果を避けるため、A は 0 以上に丸められます。

- `Min Float`
  - 2つの float の小さい方を出力します。

- `Max Float`
  - 2つの float の大きい方を出力します。

- `Lerp Float`
  - 2つの float を `T` で補間します。

- `Saturate Float`
  - float を 0 から 1 に丸めます。

- `Frac Float`
  - float の小数部分だけを出力します。
  - UV スクロールで `Time` を足したあと、0..1 に戻したい場合に使います。

- `One Minus Float`
  - `1 - Value` を出力します。

## Runtime Shader Graph の実装

`Output.Surface` に Master ノードが接続されている場合、そのマテリアルは新しい shader graph 経路として扱われます。

`ShaderGraphCodeGenerator` はグラフをたどり、次のような HLSL 関数を生成します。

```hlsl
GeneratedMaterialSurface EvaluateGeneratedMaterial(
    float2 uv,
    float3 worldPosition,
    float3 normal,
    float3 viewDirection) {
    GeneratedMaterialSurface surface;
    surface.baseColor = ...;
    surface.toonBaseStep = ...;
    return surface;
}
```

その後、Object3D 用 Pixel Shader のテンプレート内で `EvaluateGeneratedMaterial()` を呼び出し、ライティング、Toon ramp、影、環境反射、alpha discard を適用します。

```hlsl
GeneratedMaterialSurface surface = EvaluateGeneratedMaterial(
    transformedUV.xy,
    input.worldPosition,
    normal,
    toEye);
float3 albedo = surface.baseColor.rgb;
```

生成された HLSL は `MaterialGraphRuntimeShaderCache` に渡され、DXC でコンパイルされます。
コンパイル済み blob は shader hash ごとにキャッシュされ、`PipelineService` が blend mode と shader hash に対応する PSO を返します。

Runtime 用の generated shader では、未接続の Master パラメータは HLSL への直埋めではなく `gMaterial` から読むようにしています。
そのため、Master ノードの通常パラメータは既存の Object3D material constant buffer に寄せられています。

DXC コンパイルに失敗した場合、エディタの `Compile HLSL` / `Build Runtime` はエラー本文をステータス表示します。
実行時は最後に成功した shader があればそれを使い、まだ成功 shader がない場合は既存の Object3D pipeline に戻ります。

## 古い Lighting Mode との関係

`Output` に残っている `Lighting Mode` は、旧仕様の互換用です。

新しい shader graph として使う場合は、基本的に `Output.Surface` を使います。
`Toon Master`, `Lit Master`, `Unlit Master` のどれを `Surface` に接続するかで描画モデルが決まるため、`Lighting Mode` ノードを別に接続する必要はありません。

`Output.Surface` が未接続の場合だけ、古い `BaseColor`, `Shininess`, `Roughness`, `Reflect`, `Lighting Mode` 入力が評価されます。

## 現在できること

- マテリアルを `.mat` として保存、読み込みできます。
- Master ノード単位で Toon、Lit、Unlit を選べます。
- Toon の色、しきい値、ぼかし幅、スペキュラをノード入力から調整できます。
- Object3D の既存テクスチャを shader graph 内でサンプリングできます。
- Procedural な `Noise Texture` で、テクスチャアセットなしのノイズマテリアルを作れます。
- Color と Float の基本演算を組み合わせられます。
- UV、Time、World Position、World Normal、View Direction の float 入力を使えます。
- static model の Object3D 描画で、生成 Pixel Shader と生成 PSO を使用できます。

## 現在の制限

- 生成 shader graph の実行時適用は static model が中心です。
  skinned model への経路はまだ整理が必要です。

- `Texture Sample` は Object3D の標準テクスチャを使います。
  任意の Texture Asset、UV 変換専用ノード、複数テクスチャスロットはまだありません。

- `Float2` は使用できます。
  `Float3`, `Float4`, `Vector`, `Normal` 型は型定義の準備だけで、実用ノードはまだありません。

- DXC のコンパイルエラーはステータス表示できますが、エラー行へのジャンプや専用ログパネルはまだありません。

- 生成 HLSL は C++ 内で文字列生成しています。
  今後は shader template と include に分離すると保守しやすくなります。

## 次に増やしたい機能

1. DXC エラー表示とフォールバック
   - Build Runtime 失敗時に、エディタ上でエラー行、エラーメッセージ、最後に成功した shader の使用状態を表示します。

2. Material Parameter Buffer
   - ノード値を HLSL に直接埋め込むだけでなく、Material Constant Buffer として渡せるようにします。
   - これにより、値変更だけなら PSO 再生成なしで反映できます。

3. 標準入力ノード
   - `UV`, `Time`, `World Position`, `World Normal`, `View Direction`, `Camera Position` を追加します。

4. 型の拡張
   - `Float2`, `Float3`, `Float4`, `Vector`, `Normal` を追加します。
   - 型変換、swizzle、combine、split も必要になります。

5. PBR / Toon の拡張
   - Normal Map、Rim Light、Fresnel、Emission、Alpha Clip、Metallic、AO を追加します。

6. グラフ検証の強化
   - 循環参照、未接続必須ピン、型不一致、Master ノード複数接続を UI 上で警告します。

7. Shader Template 化
   - Object3D 用 shader graph の共通ライティング部分を `.hlsl` include に分け、生成部分を material function だけに寄せます。

8. Hot Reload と Disk Cache
   - shader hash ごとのコンパイル結果をディスクに保存し、起動後の再コンパイルを減らします。
