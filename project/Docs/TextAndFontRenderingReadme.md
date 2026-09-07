# Text・Font描画機能

## 1. 概要

NexusEngine（CalyxEngine）のText描画機能は、FreeTypeで`.ttf`／`.otf`を読み込み、必要になったUnicode Glyphだけを動的Atlasへ追加してDirectX 12で描画する汎用基盤です。

ゲームUI、字幕、デバッグ表示、ノベルゲームのセリフに共通利用できます。会話の分岐やクリック待ちなどのゲーム固有処理はText Rendererへ含めません。

現在は次の2通りで利用できます。

- C++から`TextService`へ直接描画要求を提出する
- Editorで`TextSceneObject2d`をSceneへ配置する

## 2. 対応機能

- `.ttf`／`.otf`読み込み（FreeType）
- UTF-8入力とUnicode CodePoint単位の処理
- 日本語Glyphの動的生成とキャッシュ
- Replacement Glyph（`U+FFFD`、`?`）へのフォールバック
- 1024×1024 Glyph Atlasの複数ページ対応
- Glyphのadvance、bearing、baselineを使った配置
- 明示的な改行（`\n`）
- テキストボックス幅による自動折り返し
- Font Size、Color、文字間、行間
- Unicode CodePoint単位のTypewriter表示
- Atlas Page単位のバッチ描画
- Depth TestなしのAlpha Blend描画
- Font数、Glyph数、Atlas使用量のDebug統計

## 3. EditorでText 2Dを作成する

1. Hierarchyの空白部分を右クリックする。
2. `Create > 2D > Text 2D`を選択する。
3. 作成された`Text2D`を選択する。
4. Inspectorの`Text`へUTF-8文字列を入力する。
5. Assetsに登録された`.ttf`または`.otf`をFont欄へドラッグ＆ドロップする。
6. Viewportで位置とテキストボックスサイズを調整する。

`TextSceneObject2d`は`ObjectType::Object2D`なので、Spriteと同じ2Dギズモを使用します。

- Translate: X／Y位置を変更する
- Scale X: 自動折り返しに使用するボックス幅を変更する
- Scale Y: Editor上のテキストボックス表示サイズを変更する
- Rotate Z: 2D Transformとして保存される（Text Rendererによる文字回転は現在未対応）

### InspectorのText設定

| 項目 | 説明 |
|---|---|
| Text | UTF-8の複数行文字列。最大入力バッファは現在8191 byte |
| Font | AssetsからFontアセットをドロップして指定 |
| Font Size | Rasterizeするピクセルサイズ |
| Color | Glyphへ乗算するRGBA色 |
| Letter Spacing | Glyph間の追加距離 |
| Line Spacing | Fontの標準行高に掛ける倍率 |
| Auto Wrap | Scale Xを最大幅として自動改行する |

### Inspectorの再生設定

| 項目 | 説明 |
|---|---|
| Typewriter | 一文字ずつ表示する |
| Auto Play |初期化後に自動再生する |
| Loop |全文表示後に先頭から再生する |
| Start Delay |文字送り開始までの待機時間 |
| Use Duration |文字速度ではなく全文表示時間を使う |
| Reveal Duration |全文が表示されるまでの秒数 |
| Characters / Sec |1秒あたりに表示するUnicode文字数 |
| Visible Characters |Editor上で表示位置を手動確認する |
| Play／Pause |現在位置から再生／一時停止する |
| Restart |表示文字数と経過時間を0へ戻して再生する |
| Stop |再生を停止して初期表示状態へ戻す |

空白と改行もTypewriterの文字数に含まれます。UTF-8の1 byte単位では処理しないため、日本語1文字が複数文字として数えられることはありません。

## 4. C++から描画する

```cpp
#include <Engine/Renderer/Text/TextService.h>

auto* textService = CalyxEngine::TextService::GetInstance();

FontHandle font = textService->LoadFont(
    "Fonts/NotoSansJP-Regular.ttf");

CalyxEngine::TextStyle style{};
style.fontSize_ = 32.0f;
style.color_ = {1.0f, 1.0f, 1.0f, 1.0f};
style.letterSpacing_ = 0.0f;
style.lineSpacing_ = 1.0f;
style.wordWrap_ = true;
style.maxWidth_ = 600.0f;

textService->Draw(
    font,
    u8"これは自動折り返しに対応した日本語テキストです。",
    {100.0f, 500.0f},
    style);
```

一文字送りでは、最後の引数へ表示するUnicode文字数を指定します。

```cpp
const size_t visibleCharacters = static_cast<size_t>(
    elapsedTime * charactersPerSecond);

textService->Draw(
    font,
    u8"一文字ずつ表示します。",
    {100.0f, 500.0f},
    style,
    visibleCharacters);
```

`LoadFont`は解決済みPathをキャッシュします。同じFontを毎フレーム指定しても、Font Fileや`FT_Face`を毎回作り直しません。ただし、通常は初期化時に`FontHandle`を保持してください。

## 5. TextSceneObject2dをゲームコードから操作する

```cpp
textObject->SetText("新しい文章です。");
textObject->Restart();

if(textObject->IsComplete()) {
    // 次のセリフへ進むなど、ゲーム固有の処理を行う。
}
```

主な操作APIは次の通りです。

```cpp
void Play();
void Pause();
void Restart();
void Stop();
void SetText(std::string text);
void SetVisibleCharacterCount(size_t count);
bool IsComplete() const;
```

Dialogueのページ制御、選択肢、クリック待ち、話者名、分岐はゲーム側の`DialogueSystem`などから操作してください。

## 6. 自動折り返し

自動折り返しは`TextLayout`が担当します。`TextRenderer`や`TextSceneObject2d`は改行位置を計算しません。

```text
UTF-8
  ↓ Utf8Decoder
Unicode CodePoint列
  ↓ TextLayout
Glyph metrics + maxWidth
  ↓
PositionedGlyph列
```

`wordWrap_ == true`かつ`maxWidth_ > 0`の場合、次のGlyphのadvanceが最大幅を超える前に次の行へ移動します。

現在は文字単位の折り返しです。英単語単位のWord Wrap、行頭・行末禁則、句読点のぶら下げには未対応です。

## 7. Typewriterの文字数

`PositionedGlyph::sourceIndex_`には元のUnicode CodePoint位置が保存されます。描画時はこの値と`visibleGlyphCount_`を比較します。

これにより、Bitmapを持たない空白や改行も再生時間へ含められます。

```text
「A B」

sourceIndex 0: A
sourceIndex 1: 空白（描画Glyphなし）
sourceIndex 2: B
```

## 8. 内部構成と責務

```text
Assets/Font
  FreeTypeLibrary             FT_LibraryのRAII所有
  FontLoader                  Font FileとFT_Faceの生成
  FontAsset                   Fontリソース情報の所有
  FreeTypeBitmapRasterizer    Glyph Bitmapとmetricsの生成
  FontRepository              FontHandleとFontキャッシュ

Renderer/Text
  GlyphCache                  Font/CodePoint/PixelSize単位のキャッシュ
  GlyphAtlas                  Atlas配置、Texture更新、SRV所有
  TextLayout                  Glyph位置、baseline、改行、折り返し
  TextRenderer                頂点生成、Atlas Page単位のGPU描画
  TextService                 Game/Runtime/Editor向けFacade

Objects/2D/Object2d
  TextSceneObject2d           Scene保存、Inspector、再生設定
  ITextRenderable             現在描画するSceneからの提出境界
```

責務上の制約は次の通りです。

- `FontLoader`はGPU描画しない
- `TextRenderer`はFont FileやFreeType APIを扱わない
- `GlyphAtlas`は文字配置や再生時間を計算しない
- `TextLayout`はDirectX 12へ依存しない
- `TextSceneObject2d`はGlyph RasterizeやAtlasを所有しない

## 9. フレーム処理

```text
CalyxCore::BeginFrame
  TextService::BeginFrame
    前フレームの描画要求をクリア
    Fence完了済みUpload Resourceを解放

BaseScene::DrawSpritesOnly
  Spriteを描画
  Scene内のITextRenderableを収集
  TextSceneObject2d::SubmitText

SceneManager
  TextService::Render
    UTF-8 Decode
    TextLayout
    未生成GlyphだけRasterize
    dirty Atlas PageだけUpload
    Atlas Page単位でDrawCall
```

描画要求をSceneの描画時に提出することで、Editor Preview SceneとRuntime SceneのTextが混在しないようにしています。

## 10. Glyph CacheとAtlas

Glyph Cacheのキーは次の組み合わせです。

```text
FontHandle + Unicode CodePoint + Pixel Size
```

同じ文字を同じFont・Sizeで繰り返し描画しても、再Rasterizeされません。日本語Fontの全Glyphを起動時に展開する処理はありません。

AtlasはShelf Packingを使用します。既存Pageへ入らない場合は、新しい1024×1024 Pageを作成します。各PageのSRVは共通`DescriptorAllocator`から確保され、独自Descriptor Heapは作成しません。

Atlas更新時だけ以下の遷移を行います。

```text
PIXEL_SHADER_RESOURCE
  → COPY_DEST
  → CopyTextureRegion
  → PIXEL_SHADER_RESOURCE
```

Upload Resourceはフレームをまたいで保持し、次の安全な`BeginFrame`で解放します。

## 11. GPU描画

1文字につき6頂点のQuadを生成しますが、1文字ごとのDrawCallは発行しません。同じAtlas PageにあるGlyphをまとめ、Atlas Pageごとに1回描画します。

Text専用PSOは次の設定です。

- `Text.VS.hlsl`／`Text.PS.hlsl`
- Alpha Blend
- Cullなし
- Depth Testなし
- Glyph AtlasのRチャンネルをcoverageとして使用

## 12. Scene保存項目

`TextSceneObject2d`では以下をScene／Prefabへ保存します。

- Text、Font GUID、Font Path
- Transform（位置とテキストボックスサイズ）
- Font Size、Color、Letter Spacing、Line Spacing
- Auto Wrap
- Typewriter、Auto Play、Loop
- Start Delay
- Duration方式の有無とReveal Duration
- Characters Per Second

`elapsedTime_`、`playing_`、現在の表示文字数などの実行中状態は保存しません。

## 13. Debug統計

```cpp
const CalyxEngine::TextDebugStats stats =
    CalyxEngine::TextService::GetInstance()->GetDebugStats();
```

取得できる情報：

- Font Count
- Glyph Count
- Atlas Page Count
- Atlas Page Size
- Atlas Usage

## 14. 表示されない場合

次を確認してください。

1. `.ttf`／`.otf`がAssetsに登録され、Inspectorへドロップされているか
2. Font PathがプロジェクトのAssets Rootから解決可能か
3. `Draw Enable`が有効か
4. Text ColorのAlphaが0になっていないか
5. Typewriterが有効なまま停止し、Visible Charactersが0になっていないか
6. Font Sizeが1以上か
7. Text BoxのScale Xが極端に小さくないか
8. Fontに対象文字が含まれるか
9. Logに`FontLoader`、`GlyphCache`、`GlyphAtlas`のエラーがないか

新規Text 2Dの既定Font Pathは`Fonts/NotoSansJP-Regular.ttf`です。このFontが存在しないプロジェクトでは、FontアセットをInspectorへ明示的にドロップしてください。

## 15. 現在の制約と拡張ポイント

未対応：

- 左・中央・右Alignment
- 縦方向Alignment
- Text Box外のクリッピング
- 英単語単位の折り返し
- 日本語禁則処理
- Outline、Shadow、Gradient、Glow
- SDF／MSDF
- Rich Text、ルビ、縦書き
- World Space Text
- Textの回転描画
- TextとSpriteを共通Sorting Layer内で完全に混在させる描画順

現在Textは通常Sprite描画後に描画されます。将来Sorting Layerへ統合する場合は、TextをSpriteへ継承させるのではなく、Sprite/Text共通の2D Draw QueueへSort Keyを提出する構成を推奨します。

SDF／MSDF対応時は`FreeTypeBitmapRasterizer`相当のRasterize BackendとText Shaderを差し替え、`TextLayout`と`TextSceneObject2d`のAPIを維持します。

## 16. 関連ファイル

| ファイル | 責務 |
|---|---|
| `Engine/Assets/Font/FontRepository.*` | FontHandleとFont Cache |
| `Engine/Assets/Font/FontLoader.*` | Font File読み込み |
| `Engine/Assets/Font/FreeTypeBitmapRasterizer.*` | Glyph Rasterize |
| `Engine/Foundation/Utility/Text/Utf8Decoder.*` | UTF-8 Decode |
| `Engine/Renderer/Text/GlyphCache.*` | Dynamic Glyph Cache |
| `Engine/Renderer/Text/GlyphAtlas.*` | GPU Atlasと部分更新 |
| `Engine/Renderer/Text/TextLayout.*` | 改行、自動折り返し、配置 |
| `Engine/Renderer/Text/TextRenderer.*` | GPU Batch描画 |
| `Engine/Renderer/Text/TextService.*` | 公開Facade |
| `Engine/Objects/2D/Object2d/TextSceneObject2d.*` | Scene／Inspector／Typewriter |
| `Resources/shaders/Core/Text/*` | Text専用Shader |

## 17. 動作確認状況

2026-09-07時点で、Develop／x64の以下を確認しています。

- `CalyxEngine.dll`ビルド成功
- `CalyxGame.exe`ビルド成功
- `CalyxEditor.exe`ビルド成功
- Editor起動後8秒間、起動時クラッシュなし

