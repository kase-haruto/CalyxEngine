# Post Effect Editor / Node Graph

このフォルダには、ポストエフェクト用のプリセットファイルを配置します。
標準の編集対象は `Default.postfx` です。

ポストエフェクトは `EditMode` の `PostEffect` から編集します。
`PostEffect` モードを選ぶと、専用レイアウト `Resources/Assets/Configs/Editor/Layout/PostEfectEdit.ini` が適用されます。

## 目的

ポストエフェクトをコードだけで切り替えるのではなく、エディタ上でノードとして組み立て、保存した `.postfx` をゲーム中に読み込んで使えるようにします。

例えば、被弾時に一瞬だけ色収差を強める演出は、エディタでは `Triggered` として作成し、ゲームコードでは次のように呼び出します。

```cpp
#include <Engine/PostProcess/Manager/PostEffectManager.h>

void Player::OnDamaged() {
	PostEffectManager::Get()->PlayTriggeredEffect("ChromaticAberration");
}
```

複数の `Triggered` エフェクトを同時に再生したい場合は、次のようにします。

```cpp
PostEffectManager::Get()->PlayTriggeredEffects();
```

## 全体の流れ

1. `EditMode` から `PostEffect` を選択します。
2. `Post Effect Graph` パネルでノードを追加します。
3. `Input` から各エフェクトを通し、最後に `Output` へ接続します。
4. ノードを選択し、右側の Inspector でパラメータを調整します。
5. `Always` か `Triggered` を選びます。
6. `Save` で `.postfx` に保存します。
7. `Apply` で `PostEffectManager` に反映します。
8. ゲームコードから必要なタイミングで `PlayTriggeredEffect()` または `PlayTriggeredEffects()` を呼びます。

エディタ上の `Play` ボタンは内部で `Apply` してから `Triggered` を再生します。

```cpp
void PostEffectNodeEditorPanel::Apply() {
	Save();
	PostEffectManager::Get()->LoadPreset(pathBuffer_.data());
}

// Play button
Apply();
PostEffectManager::Get()->PlayTriggeredEffects();
```

## 主要ファイル

- `Engine/Application/UI/Panels/PostEffectNodeEditorPanel.*`
  - ノードエディタ UI です。
  - `.postfx` の保存、読み込み、適用を行います。

- `Engine/PostProcess/Manager/PostEffectManager.*`
  - ポストエフェクトのスロット管理、プリセット読み込み、Triggered 再生を行います。

- `Engine/PostProcess/Graph/PostEffectGraph.*`
  - ノード接続を実行用グラフとして解釈し、レンダーターゲットを使って順番に実行します。

- `Engine/PostProcess/Slot/PostEffectSlot.h`
  - `Always`, `Triggered`, 時間、イージング、Float Animation などの状態を持ちます。

- `Resources/Assets/PostEffects/*.postfx`
  - エディタで保存されるプリセットです。

## エディタ側の実装

`PostEffectNodeEditorPanel` は `IEngineUI` として実装されています。
初期状態では `Input` と `Output` が必ず存在するようにしています。

```cpp
PostEffectNodeEditorPanel::PostEffectNodeEditorPanel()
	: IEngineUI("Post Effect Graph"), canvas_("PostEffectGraphCanvas") {
	isShow_ = false;
	std::copy_n(kDefaultPath,
				(std::min)(strlen(kDefaultPath), pathBuffer_.size() - 1),
				pathBuffer_.data());
	EnsureIoNodes();
}
```

ノード追加時は、通常のエフェクトなら入力1つ、`Blend` だけ入力2つを持たせています。

```cpp
void PostEffectNodeEditorPanel::AddEffectNode(const std::string& type, Vector2 position) {
	Node node;
	node.id = graph_.AllocateId();
	node.type = type;
	node.title = type;
	node.position = position;

	if(type == "Blend") {
		node.inputs.push_back({graph_.AllocateId(), "A", NodePinKind::Input, NodeValueType::Color});
		node.inputs.push_back({graph_.AllocateId(), "B", NodePinKind::Input, NodeValueType::Color});
	} else {
		node.inputs.push_back({graph_.AllocateId(), "In", NodePinKind::Input, NodeValueType::Color});
	}

	node.outputs.push_back({graph_.AllocateId(), "Out", NodePinKind::Output, NodeValueType::Color});
	node.properties = {
		{"enabled", true},
		{"applyMode", "Always"},
		{"duration", 0.25f},
		{"easeIndex", static_cast<int>(EaseType::EaseOutSine)},
		{"autoDisable", true},
		{"parameters", DefaultParameters(type)},
		{"floatAnimations", nlohmann::json::array()}
	};
	graph_.nodes.push_back(std::move(node));
}
```

## 使用できるノード

現在のエディタから追加できるノードは以下です。

```cpp
constexpr const char* kEffectTypes[] = {
	"RadialBlur",
	"ChromaticAberration",
	"Vignette",
	"CRTEffect",
	"Blend"
};
```

- `RadialBlur`
  - 中心から外側へ伸びるブラーです。
  - ダッシュ、ワープ、衝撃演出に向いています。

- `ChromaticAberration`
  - RGB をずらす色収差です。
  - 被弾、警告、画面ノイズ表現に向いています。

- `Vignette`
  - 画面端を暗くします。
  - 緊張感、集中、HP 低下演出に向いています。

- `CRTEffect`
  - CRT 風の表示にします。
  - 回想、監視カメラ、レトロ UI 表現に向いています。

- `Blend`
  - 2つの入力を合成します。
  - 分岐した加工結果を合成したい場合に使います。

## Always と Triggered

各エフェクトは `Apply Mode` を持ちます。

```cpp
enum class PostEffectApplyMode {
	Always,
	Triggered
};
```

`Always` は常時適用するエフェクトです。
`enabled` が true の間、毎フレーム実行されます。

```json
{
  "type": "Vignette",
  "enabled": true,
  "applyMode": "Always",
  "parameters": {
    "strength": 0.35,
    "radius": 0.2
  }
}
```

`Triggered` は時間指定で一時的に再生するエフェクトです。
通常時は無効で、`PlayTriggeredEffect()` または `PlayTriggeredEffects()` を呼んだ時だけ有効になります。

```json
{
  "type": "ChromaticAberration",
  "enabled": false,
  "applyMode": "Triggered",
  "duration": 0.25,
  "easeIndex": 8,
  "autoDisable": true,
  "parameters": {
    "intensity": 0.0
  },
  "floatAnimations": [
    {
      "parameter": "intensity",
      "from": 0.0,
      "to": 0.7,
      "useCurrentAsFrom": false
    }
  ]
}
```

## 時間指定エフェクトの使い方

エディタで `Triggered` ノードを作成した場合、ゲーム側では再生タイミングに対応するコードを書きます。

シーン開始時にプリセットを読み込む例:

```cpp
#include <Engine/PostProcess/Manager/PostEffectManager.h>

void BattleScene::Initialize() {
	PostEffectManager::Get()->LoadPreset("Resources/Assets/PostEffects/Default.postfx");
}
```

被弾時に色収差だけを再生する例:

```cpp
void Player::OnDamaged() {
	PostEffectManager::Get()->PlayTriggeredEffect("ChromaticAberration");
}
```

ボスの攻撃開始時に、プリセット内の `Triggered` をまとめて再生する例:

```cpp
void BossAttack::StartSpecialAttack() {
	PostEffectManager::Get()->PlayTriggeredEffects();
}
```

ワープやダッシュ時に `RadialBlur` を再生する例:

```cpp
void Player::StartDash() {
	PostEffectManager::Get()->PlayTriggeredEffect("RadialBlur");
}
```

重要なのは、`.postfx` 側に `Triggered` と `floatAnimations` を保存しておくことです。
ゲームコードは「どのタイミングで再生するか」だけを担当します。

## Triggered の内部処理

`PlayTriggeredEffects()` は、プリセット内で `Triggered` に設定されたスロットを探して再生します。

```cpp
void PostEffectManager::PlayTriggeredEffects() {
	if(!initialized_) return;
	for(const auto& slot : collection_.GetSlots()) {
		if(slot.applyMode == PostEffectApplyMode::Triggered) {
			PlayTriggeredEffect(slot.name);
		}
	}
}
```

個別再生では、対象パスを有効化し、保存された `floatAnimations` を `TweenFloat()` に渡します。

```cpp
void PostEffectManager::PlayTriggeredEffect(const std::string& name) {
	if(!initialized_) return;

	const int idx = IndexOf(name);
	if(idx < 0) return;

	auto& slot = collection_.GetSlots()[idx];
	if(slot.applyMode != PostEffectApplyMode::Triggered || !slot.pass) return;

	Enable(slot.name, true);

	for(const auto& anim : slot.floatAnimations) {
		float current = 0.0f;
		const bool hasCurrent = slot.pass->GetFloatParameter(anim.parameter, current);
		const float from = anim.useCurrentAsFrom && hasCurrent ? current : anim.from;

		TweenFloat(slot.name,
				   [pass = slot.pass, param = anim.parameter]() {
					   float value = 0.0f;
					   pass->GetFloatParameter(param, value);
					   return value;
				   },
				   [pass = slot.pass, param = anim.parameter](float value) {
					   pass->SetFloatParameter(param, value);
				   },
				   from,
				   anim.to,
				   slot.duration,
				   slot.ease,
				   slot.autoDisable,
				   nullptr);
	}
}
```

`TweenFloat()` は毎フレーム `PostEffectManager::Update()` から進行します。

```cpp
void PostEffectManager::Update(float dt) {
	for(auto& tw : floatTweens_) {
		tw.t += dt;
		const float r = std::clamp(tw.t / (std::max)(0.0001f, tw.dur), 0.0f, 1.0f);
		const float k = CalyxEngine::ApplyEase(tw.ease, r);
		const float v = std::lerp(tw.start, tw.end, k);

		if(tw.setter) {
			tw.setter(v);
		}
	}
}
```

このため、時間指定エフェクトを使う場合は、`PostEffectManager::Update()` が毎フレーム呼ばれている必要があります。
現在は `CalyxCore::EndFrame()` 側で更新されています。

```cpp
PostEffectManager::Get()->Update(clock->GetDeltaTime());
```

## ランタイムへの反映

プリセット読み込みでは、JSON の `nodes` からスロットを再構築します。
`Triggered` の場合はロード時点では無効にしておきます。

```cpp
if(slot.applyMode == PostEffectApplyMode::Triggered) {
	slot.enabled = false;
}

if(slot.pass && node.contains("parameters") && node["parameters"].is_object()) {
	slot.pass->LoadParameters(node["parameters"]);
}
```

実行時は `PostEffectGraph` が必要なパスだけを実行します。
グラフ情報がある場合はノード接続を使い、ない場合は従来通り enabled なスロットを順番に実行します。

```cpp
void PostEffectManager::RebuildGraphIfDirty() {
	if(!dirty_) return;

	if(hasLoadedGraph_) {
		graph_.SetGraphFromJson(loadedPreset_, collection_.GetSlots());
	} else {
		graph_.SetPassesFromList(collection_.GetSlots());
	}

	dirty_ = false;
}
```

## グラフ実行の工夫

`PostEffectGraph` は `Output` に接続されているノードから逆向きに入力元をたどり、必要なノードだけを実行します。

```cpp
const int32_t sourceNode = FindOutputSourceNode();
if(sourceNode != 0) {
	currentSRV = ExecuteGraphNode(cmd, sourceNode, currentSRV, dxCore, cache, visiting, tempIndex);
}
```

一度実行したノードは cache に保存し、同じノードを複数回実行しないようにしています。

```cpp
if(auto cached = cache.find(nodeId); cached != cache.end()) {
	return cached->second;
}
```

`Blend` は通常の1入力エフェクトと違い、A/B の2入力を評価してから実行します。

```cpp
if(node.type == "Blend") {
	D3D12_GPU_DESCRIPTOR_HANDLE inputA = sceneSRV;
	D3D12_GPU_DESCRIPTOR_HANDLE inputB = sceneSRV;

	if(!node.inputPins.empty()) {
		inputA = ExecuteGraphNode(cmd, FindInputSourceNode(node.inputPins[0]), sceneSRV, dxCore, cache, visiting, tempIndex);
	}
	if(node.inputPins.size() >= 2) {
		inputB = ExecuteGraphNode(cmd, FindInputSourceNode(node.inputPins[1]), sceneSRV, dxCore, cache, visiting, tempIndex);
	}

	if(auto* blend = dynamic_cast<BlendEffect*>(node.pass)) {
		blend->Apply(cmd, inputA, inputB, output);
	}
}
```

## プリセット例

最小構成は `Input -> Effect -> Output` です。

```json
{
  "type": "PostEffectPreset",
  "name": "DamageFlash",
  "version": 1,
  "outline": {
    "enabled": true
  },
  "nodes": [
    {
      "type": "ChromaticAberration",
      "enabled": false,
      "applyMode": "Triggered",
      "duration": 0.2,
      "easeIndex": 8,
      "autoDisable": true,
      "parameters": {
        "intensity": 0.0
      },
      "floatAnimations": [
        {
          "parameter": "intensity",
          "from": 0.0,
          "to": 0.8,
          "useCurrentAsFrom": false
        }
      ]
    }
  ]
}
```

このプリセットを読み込んだ後、被弾時に次を呼べば時間指定エフェクトが再生されます。

```cpp
PostEffectManager::Get()->PlayTriggeredEffect("ChromaticAberration");
```

## 現在の制限

- `Apply` しないとエディタ上の変更はランタイムに反映されません。
- `Save` はファイル保存のみです。
- `Play` は内部で `Apply` してから `Triggered` を再生します。
- `Triggered` ノードはロード直後は無効です。
- `CopyImage` は最終出力コピー用の内部パスなので、通常の編集対象にはしません。
- `Blend` はグラフ接続がある場合に有効です。
- ノード実行用一時バッファは `PostEffectNodeBuffer0` から最大8個を想定しています。
