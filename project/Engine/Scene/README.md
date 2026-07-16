# Scene管理

このディレクトリには、データドリブンなシーンの読み込み、実行、遷移を管理する機能があります。シーン固有のクラスに依存せず、`SceneManager`と`SceneContext`を中心にシーンを動作させます。

## シーン遷移演出

`SceneManager`はシーン交換だけでなく、交換前後の遷移演出も管理します。

遷移は次の順序で行われます。

1. 現在のシーンを表示したままFadeOutを実行
2. FadeOut完了後にシーンを交換
3. 新しいシーンを表示しながらFadeInを実行
4. FadeIn完了後に遷移演出を破棄

演出の更新には、ポーズやゲームのタイムスケールに影響されない`alwaysDt`が使用されます。演出はポストエフェクトと通常UIの描画後にBackBufferへ描画されます。

### 標準の黒フェード

通常の`RequestSceneChange`は、標準の`FadeBlackOutEffect`を自動的に使用します。遷移先はシーンパスまたはアセットGUIDで指定できます。

```cpp
#include <Engine/Scene/Utility/SceneUtility.h>

SceneAPI::RequestSceneChange("Resources/Scenes/Game.scene");

// またはGUIDで指定
SceneAPI::RequestSceneChange(nextSceneGuid);
```

初期シーンの読み込みなどで`OpenScene`を直接呼び出した場合、遷移演出は再生されません。

### ゲーム側で独自演出を実装する

独自の遷移演出は`BaseSceneTransitionEffect`を継承して実装します。`normalizedTime`には各フェーズの進行度が`0.0f`から`1.0f`の範囲で渡されます。

```cpp
#include <Engine/Scene/Fade/BaseSceneTransitionEffect.h>

class WipeTransitionEffect final
    : public CalyxEngine::BaseSceneTransitionEffect {
public:
    WipeTransitionEffect()
        : BaseSceneTransitionEffect(0.8f) {
    }

    void Draw(
        ID3D12GraphicsCommandList* cmd,
        PipelineService* pso) override {
        // Spriteや専用シェーダーを使用して最前面に描画する
    }

protected:
    void OnStartFadeOut() override {
        // FadeOutの準備
    }

    void OnFadeOut(float normalizedTime) override {
        // 旧シーンを覆う方向へ更新
    }

    void OnEndFadeOut() override {
        // この直後にシーンが交換される
    }

    void OnStartFadeIn() override {
        // 新しいシーンへの交換後に呼ばれる
    }

    void OnFadeIn(float normalizedTime) override {
        // 新しいシーンを見せる方向へ更新
    }

    void OnEndFadeIn() override {
        // この直後に演出オブジェクトが破棄される
    }
};
```

`Draw`は遷移中の各フレームで呼び出されます。描画に必要なパイプライン設定は演出側で行ってください。

### 独自演出を使用する

演出を`unique_ptr`として遷移要求へ渡します。所有権は`SceneManager`へ移動し、FadeIn完了後に自動的に解放されます。

```cpp
#include <Engine/Scene/Utility/SceneUtility.h>
#include <memory>

SceneAPI::RequestSceneChange(
    nextSceneGuid,
    std::make_unique<WipeTransitionEffect>());
```

シーンパスでも同じ形式で指定できます。

```cpp
SceneAPI::RequestSceneChange(
    "Resources/Scenes/Result.scene",
    std::make_unique<WipeTransitionEffect>());
```

演出時間は基底クラスのコンストラクタ、または`SetTime`で秒単位に設定できます。`0.0f`の場合、該当フェーズは次の更新で即座に完了します。

## 関連ファイル

- `System/SceneManager.h` — シーンと遷移状態の管理
- `Context/SceneContext.h` — シーン内のオブジェクトと各システムの実行コンテキスト
- `Serializer/SceneSerializer.h` — シーンデータの読み書き
- `Transitioner/SceneTransitionRequestor.h` — 遷移要求インターフェース
- `Utility/SceneUtility.h` — ゲーム側のシーンAPI
- `Fade/BaseSceneTransitionEffect.h` — 遷移演出の基底クラス
- `Fade/FadeBlackOutEffect.h` — 標準の黒フェード
