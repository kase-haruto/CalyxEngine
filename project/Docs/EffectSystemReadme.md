# Particle Effect System設計

## 1. この文書の目的

この文書は、CalyxEngineのParticle Effect Systemについて、現在の構造、データの保存形式、Moduleの実行方法、CPU/GPU Particleの違い、DirectX 12リソースの所有境界を説明する。

未実装の将来設計と、現在利用できる機能を混同しないことも目的としている。

## 2. 現在の全体構成

```text
EffectAsset (.effect)
    |
    v
EffectAssetData
    |
    +-- EffectEmitterAssetData
            |
            +-- EmitterConfig
                    |
                    +-- ModuleConfig[]
    |
    v
EffectPlayer / FxObject
    |
    +-- FxEmitter       (CPU Particle)
    +-- GpuFxEmitter    (GPU Particle)
    |
    v
FxSystem
    |
    v
ParticleRenderer
```

Effectは複数のEmitterから構成される。EmitterごとにCPU ParticleまたはGPU Particleを選択できる。

現在、Module Systemの中心となる実装はCPU用の`FxEmitter`である。GPU用の`GpuFxEmitter`はCompute Shader内に対応機能を個別実装しているため、CPUと同じModuleをすべて利用できるわけではない。

## 3. クラスごとの責務

### EffectAsset

`.effect`ファイルと`EffectAssetData`の保存・読み込みを担当する。

- Scene Objectを所有しない
- Particleを更新しない
- TextureやDescriptorを所有しない
- Asset versionを管理する

### FxObject

EffectをScene上へ配置するためのScene Objectである。

- 子Emitter Objectの管理
- 全EmitterのPlay、Stop、Restart
- Effect Assetの編集・保存
- Scene Transformとの同期

### EffectPlayer

Scene Objectを作らず、Effect AssetをRuntime再生する。

- Effect Instanceの生成
- Runtime Emitterの登録と破棄
- Effect全体のTransform適用

### FxEmitter

CPU Particleの生成とSimulationを担当する。

- `FxUnit`の生成と寿命管理
- Emitter Shapeからの生成位置計算
- Module実行
- Particle DataのGPU転送
- 固定Random Seed

現段階ではEditor UIや一部のGPU Bufferも保持している。将来的には編集データ、Runtime、Renderer Resourceをさらに分離する。

### GpuFxEmitter

Compute ShaderによるGPU Particleを管理する。

- Particle Structured Buffer
- Free List Buffer
- Initialize、Emit、Update Dispatch

CPU Moduleの完全な互換実行は未対応である。

### FxModuleContainer

Emitterへ追加されたModuleを`std::unique_ptr`で所有する。

- Moduleの追加、削除、有効無効
- ConfigからRuntime Moduleへの生成
- Runtime ModuleからConfigへの保存
- Stage別実行リストの構築

Stage別リストはModule追加、削除、Asset読み込み時にだけ再構築する。Particle Update中に編集用Module一覧を分類し直すことはない。

### ParticleRenderer

CPU/GPU Emitterを描画する。

- BlendModeに対応するPipeline取得
- Camera、Particle Buffer、TextureのBind
- Instanced Draw

Module自身はPipeline、RootSignature、RenderTargetを所有しない。

## 4. Particleデータ

CPU Particleは`FxUnit`として保持する。

主なデータは次の通りである。

```text
position / velocity
rotationEuler / spinSpeed
initialScale / scale
color
emissiveColor / emissiveIntensity
lifetime / age / lifeT
randomSeed
uvTransform
```

正規化Lifetimeは次の計算を使用する。

```cpp
lifeT = age / lifetime;
```

`lifetime <= 0`の場合は除算せず、`lifeT = 1`として扱う。通常の結果は`0`から`1`へClampする。

## 5. Module Stage

Moduleは`ParticleModuleStage`で次の段階へ分類する。

```text
Emitter
Spawn
Initialize
Update
Render
Event
```

現在のCPU Runtimeで実行されるStageはInitializeとUpdateである。

```text
Particle生成
    -> Spawn / Initialize Module
    -> Particle追加

毎フレーム
    -> normalized lifetime更新
    -> Update Module
    -> 位置更新
    -> 回転更新
    -> 寿命判定
```

Render ModuleとEvent Moduleのデータ分類は定義済みだが、独立したRender Parameter BuildとEvent Dispatchは今後実装する。

同じStage内では、`FxModuleContainer`に保存された順序を維持する。

## 6. 現在利用できるModule

### 既存Module

- Gravity Module
- Size Over Lifetime Module
- Over Lifetime Module
- Texture Sheet Animation Module

### Lifetime Module

- Color Over Lifetime
- Alpha Over Lifetime
- Size Curve Over Lifetime
- Rotation Over Lifetime
- Velocity Over Lifetime
- Emissive Over Lifetime

Lifetime ModuleはParticleの`lifeT`を使って値を評価する。

### 移動Module

- Acceleration Module
- Drag Module

Dragはフレーム単位の固定倍率ではなく、指数減衰を使用する。

```cpp
velocity *= std::exp(-drag * deltaTime);
```

## 7. CurveとGradient

CurveはParticle専用フォルダではなく、`Engine/Foundation/Curve`へ配置している。Animation、Camera、PostEffectから再利用できるデータ型にするためである。

### FloatCurve

次のModeを持つ。

- Constant
- Linear
- Curve（複数Key）
- Random Between Two Constants
- Random Between Two Curves

### Vector3Curve

X、Y、Zごとに`FloatCurve`を持つ。

### ColorGradient

複数のColor Keyを線形補間する。

Keyの時刻は原則として`0`から`1`の正規化時間を使用する。Keyが空の場合や同一時刻の場合も、不正な除算を行わない。

## 8. Randomの再現性

CPU EmitterはEmitterごとの`DeterministicRandomStream`を持つ。

固定Random Seedが有効な場合、次の値は同じSeedと同じDeltaTime列で再現される。

- Scale
- Velocity
- Lifetime
- Spin
- DirectionとDirection Speed
- Random Spin
- Emitter Shape内の生成位置
- CurveのRandom係数

ParticleごとのCurve Random値には、Particle生成時に確定した`randomSeed`を使う。Update中に乱数を引き直さない。

Reset、Restart、One Shotの再開始時にはEmitterのRandom StreamとParticle Sequenceをリセットする。

GPU ParticleはShader側に独立したRandom実装を持つため、CPUと同じ再現性保証の対象外である。

## 9. Effect Asset形式

Effect AssetはJSON形式の`.effect`ファイルとして保存する。

概略は次の通りである。

```json
{
  "type": "EffectAsset",
  "version": 1,
  "name": "SampleEffect",
  "emitters": [
    {
      "name": "MainEmitter",
      "transform": {},
      "isDrawEnable": true,
      "isGpu": false,
      "emitter": {
        "fixedRandomSeed": true,
        "randomSeed": 1,
        "modules": []
      }
    }
  ]
}
```

### Version

現在のAsset versionは`1`である。

versionキーが存在しない旧Assetはversion 0として読み込み、存在しない設定を既定値で補完した後、現行versionとして扱う。

### Module所有権

保存データは`std::vector<std::unique_ptr<BaseModuleConfig>>`でModule Configを所有する。

生ポインタによる所有は行わない。Stage別Runtimeリストのポインタは非所有参照であり、Module Container変更時に必ず再構築する。

### 未知のModule

現在、未知のModule TypeはFactoryで生成できずスキップされる。警告ログ、未知データの保持、削除済みModule向けMigrationは未実装である。

## 10. Editor操作

現在のEmitter UIでは次の操作を行える。

- Module追加
- Module削除
- Module有効無効
- Lifetime Curveの基本値編集
- Gradientの開始色と終了色の編集
- Texture GUIDの選択
- BlendModeの選択
- Billboard Modeの選択
- Camera Fade
- 固定Random Seed
- Play、Stop、Reset

専用Curve Canvas、Module複製、Dragによる並び替え、Undo/Redo、Seek、1フレーム送り、Presetは未実装である。

Particle Previewは`ParticlePreviewSession`が専用`SceneContext`を作成して実行する。通常SceneとPreview SceneのObjectを混在させない。

## 11. 描画とDirectX 12リソース

### Texture

Texture ResourceとSRVは`TextureManager`が所有する。EmitterとModuleはTexture GUIDまたはDescriptor Handleを参照するだけである。

### PipelineとRootSignature

PipelineとRootSignatureは`PipelineService`およびPipeline管理層が所有する。ModuleごとにPSOを生成しない。

### Particle Buffer

CPU EmitterはParticle DataをStructured Bufferへ転送し、Instanced Drawを行う。GPU EmitterはParticle BufferとFree Listを使ってCompute Shaderで更新する。

### Scene ColorとBloom Mask

現在のOffscreen Render Targetは次のMRT構成である。

```text
RT0: SceneColor            R16G16B16A16_FLOAT
RT1: EmissiveBloomMask     R16G16B16A16_FLOAT
Depth: R32_TYPELESS        DSV + SRV
Sample Count: 1
```

通常Object ShaderはMRTへ対応しているが、Particle Shaderは現段階では`SV_TARGET0`のみである。`EmissiveOverLifetimeModule`はCPU Particle内の発光値を更新するが、Bloom MaskへのParticle出力は未実装である。

### Scene Depth

Offscreen Render TargetはDepth SRVを所有する。Particle RootSignatureにはまだScene Depthが追加されていないため、Soft ParticleとDepth Fadeは未実装である。

### Resource解放

Moduleは以下を所有してはならない。

- `ID3D12Resource`
- `ID3D12PipelineState`
- `ID3D12RootSignature`
- 永続Descriptor Heap

GPU使用中ResourceのFence連動遅延解放はEffect固有層では未実装である。Resourceの差し替えを追加する場合は、Descriptorを即時再利用してはならない。

## 12. Module追加手順

新しいCPU Update Moduleを追加する場合は、次の順序で実装する。

1. `BaseModuleConfig`派生Configを追加する
2. `ToJson`と`FromJson`を実装する
3. `BaseFxModule`派生Runtime Moduleを追加する
4. `GetStage`が既定のUpdate以外ならOverrideする
5. `ModuleConfigFactory`へJSON Typeを登録する
6. `ModuleFactory`へConfig/Runtimeの相互変換を追加する
7. `FxModuleContainer`の追加一覧へ登録する
8. `.vcxproj`と`.vcxproj.filters`へcppを登録する
9. 保存、再読込、Reset後の挙動を確認する

Moduleは設定値とParticle処理だけを担当し、Textureのロード、Descriptor生成、Pipeline生成を行わない。

## 13. パフォーマンス上の注意

- Particle Update中にModule Listを再構築しない
- Random値を毎フレーム生成しない
- Module内でParticleごとの動的メモリ確保を行わない
- Texture GUIDをParticleごとに解決しない
- Moduleの組み合わせごとにPSOを生成しない
- Curve Keyは保存時またはCompile時に時刻順へ正規化する

現在のCPU ParticleはAoSの`std::vector<FxUnit>`である。根拠なく全面的なSoA化は行わず、Profilerでボトルネックを確認してから変更する。

現状、CPUからGPUへの転送時に一時`std::vector`を作成しているため、Particle Update中の完全な無確保はまだ達成していない。

## 14. 現在の制限

- CPU ModuleとGPU Particleの機能差がある
- Particle Bloom Mask出力は未実装
- Noise MaskとNoise Distortionは未実装
- UV ScrollとUV Rotationは未実装
- FlipbookのFrame Blendingは未実装
- Soft ParticleとDepth Fadeは未実装
- Orbit、Attractor、Vortex、Turbulence、Curl Noiseは未実装
- Event、Collision Event、Sub Emitterは未実装
- CompiledEffectによる完全な非仮想実行データ化は未実装
- Asset Reload時の安全なGPU Resource差し替えは未実装
- 未知Moduleの警告とデータ保持は未実装

## 15. 今後の実装順序

1. Particle Render ParameterをParticle Dataへ追加
2. Particle PipelineをOffscreen MRT形式へ統一
3. EmissiveとBloom Mask出力
4. Noise Mask、Noise Distortion
5. Flipbook、UV Scroll、UV Rotation
6. Scene DepthをRendererからParticle ShaderへBind
7. Soft Particle、Depth Fade、Near Camera Fade共通化
8. 残りの移動Module
9. CompiledEffectと非仮想Runtime実行列
10. GPU Particle用Module Dataへの変換
11. EventとSub Emitter
12. EditorのCurve Canvas、並び替え、Undo/Redo

## 16. 維持すべき設計条件

- Effect AssetとRuntime Instanceを同一オブジェクトにしない
- ModuleへDirectX 12 Resourceを所有させない
- Editor用データをParticleごとに解決しない
- CPU/GPU非対応機能を暗黙に別挙動へ変換しない
- 旧Assetのキーがない場合は安全な既定値を使用する
- Lifetimeが0でも除算しない
- Random Between Curvesの係数を毎フレーム変更しない
- Stage内のModule順序を保存順から勝手に変更しない
- Game固有処理をEngine側Moduleへ追加しない

## 17. 主な関連ファイル

```text
Data/Engine/Configs/Scene/Objects/Particle/
    EffectConfig.h
    EmitterConfig.h
    Module/ModuleConfig.h
    Module/ModuleConfigFactory.h

Engine/Application/Effects/
    EffectAsset.*
    EffectPlayer.*
    FxObject.*
    FxSystem.*

Engine/Application/Effects/Particle/
    FxUnit.h
    Emitter/BaseEmitter.*
    Emitter/FxEmitter.*
    Emitter/GpuFxEmitter.*
    Module/BaseFxModule.h
    Module/ParticleModuleStage.h
    Module/Container/FxModuleContainer.*
    Module/Factory/ModuleFactory.*

Engine/Foundation/Curve/ParticleCurve.h
Engine/Foundation/Utility/Random/DeterministicRandomStream.h

Engine/Renderer/Particle/ParticleRenderer.*
Engine/Graphics/Pipeline/Presets/PipelinePresets.cpp
Engine/Graphics/RenderTarget/OffscreenRT/OffscreenRenderTarget.*

Resources/shaders/Particles/
```

