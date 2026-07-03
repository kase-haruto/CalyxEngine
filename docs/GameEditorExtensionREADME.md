# ゲーム固有 Editor Tool 拡張

## 概要

CalyxEditor は、ゲーム DLL からゲーム固有の Editor Tool を登録できます。
エンジン側はゲーム固有クラスを参照せず、ゲーム DLL が公開する
`RegisterCalyxEditorTools` 関数を実行して Tool を登録します。

用途の例:

- ゲーム固有のカメラ演出エディタ
- イベント／クエストエディタ
- 敵配置やスポーンテーブル編集
- ゲーム固有データのプレビュー

```text
CalyxEditor.exe
    |
    | LoadLibrary
    v
Game.dll
    |
    | RegisterCalyxEditorTools(apiVersion, host)
    v
EditorToolRegistry
    |
    +-- Tools メニュー
    +-- Tool の生成／更新／描画／破棄
    +-- ImGui レイアウト切替
```

## 現在の実装範囲

- `IEditorTool` によるゲーム固有 Tool の実装
- ゲーム DLL 単位の登録と登録解除
- Tools メニューへの動的追加
- Tool ごとの生成、更新、描画、破棄
- Tool ごとの ImGui `.ini` 読み込みと自動保存先切替
- API バージョン検証
- 新規ゲームプロジェクトへの Camera Editor サンプル生成

`menuPath` は登録情報として保持されますが、現在のメニュー表示は Tools 直下です。
階層メニュー対応は今後の拡張項目です。

## ゲームプロジェクト側の構成

Runtime と Editor のコードを分離します。

```text
Game/
  Runtime/
    Camera/
      CameraSequence.h
      CameraSequence.cpp
  Editor/
    GameCameraEditor.h
    GameCameraEditor.cpp
    GameEditorExtension.cpp
```

`CameraSequence` のデータ定義と再生処理は Runtime に置き、ImGui UI や
プレビュー操作は Editor に置きます。これにより、ゲーム実行時の処理から
Editor 依存を分離できます。

## 1. Editor Tool を実装する

`Game/Editor/GameCameraEditor.h`:

```cpp
#pragma once

#include <CalyxEngine/EditorExtension.h>

class GameCameraEditor final : public CalyxEditor::IEditorTool {
public:
    explicit GameCameraEditor(
        const CalyxEditor::EditorToolContext& context);

    void OnOpen() override;
    void OnClose() override;
    void Update() override;
    void Draw() override;

private:
    CalyxEditor::EditorToolContext context_;
    bool open_ = true;
    char eventId_[128]{};
    float position_[3]{};
    float rotation_[3]{};
    float fieldOfView_ = 60.0f;
};
```

`Game/Editor/GameCameraEditor.cpp`:

```cpp
#include "GameCameraEditor.h"

#include <externals/imgui/imgui.h>

GameCameraEditor::GameCameraEditor(
    const CalyxEditor::EditorToolContext& context)
    : context_(context) {
}

void GameCameraEditor::OnOpen() {
    open_ = true;
}

void GameCameraEditor::OnClose() {
    // プレビュー停止、未保存状態の処理などを行う。
}

void GameCameraEditor::Update() {
    // 必要ならプレビュー用カメラなどを更新する。
}

void GameCameraEditor::Draw() {
    if (!open_) {
        return;
    }

    ImGui::Begin(
        "Game Camera Editor###MyGame.CameraEditor",
        &open_);

    ImGui::InputText("Event ID", eventId_, sizeof(eventId_));
    ImGui::DragFloat3("Position", position_, 0.1f);
    ImGui::DragFloat3("Rotation", rotation_, 0.1f);
    ImGui::DragFloat(
        "Field of View", &fieldOfView_, 0.1f, 1.0f, 179.0f);

    if (ImGui::Button("Preview")) {
        // ゲーム固有のプレビュー処理を開始する。
    }

    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        // CameraSequence を保存する。
    }

    ImGui::End();
}
```

`###MyGame.CameraEditor` は ImGui が永続化に使用する固定 ID です。
`###`より前の表示名を変更しても、保存済みレイアウトとの対応を維持できます。

## 2. Tool をゲーム DLL へ登録する

`Game/Editor/GameEditorExtension.cpp`:

```cpp
#include "GameCameraEditor.h"

#include <CalyxEngine/EditorExtension.h>

namespace {

CalyxEditor::IEditorTool* CreateCameraEditor(
    const CalyxEditor::EditorToolContext& context) {
    return new GameCameraEditor(context);
}

void DestroyCameraEditor(CalyxEditor::IEditorTool* tool) {
    delete tool;
}

} // namespace

extern "C" __declspec(dllexport)
bool RegisterCalyxEditorTools(
    std::uint32_t apiVersion,
    CalyxEditor::IEditorHost* host) {
    if (!host ||
        apiVersion != CalyxEditor::kEditorToolApiVersion) {
        return false;
    }

    CalyxEditor::EditorToolDescriptor descriptor;
    descriptor.id = "MyGame.CameraEditor";
    descriptor.displayName = "Camera Editor";
    descriptor.menuPath = "Game/Camera";
    descriptor.workspaceId = "MyGame.Camera";
    descriptor.layoutPath =
        "Saved/Editor/Layouts/MyGame.Camera.ini";
    descriptor.create = &CreateCameraEditor;
    descriptor.destroy = &DestroyCameraEditor;

    return host->RegisterTool(descriptor);
}
```

登録後は、CalyxEditor の `Tools -> Camera Editor` から開けます。

### Descriptor の項目

| 項目 | 内容 |
| --- | --- |
| `id` | Toolを一意に識別する固定ID。`ゲーム名.Tool名`を推奨 |
| `displayName` | Toolsメニューに表示する名前 |
| `menuPath` | 将来の階層メニュー用パス |
| `workspaceId` | レイアウトを識別する固定ID |
| `layoutPath` | Toolを開いたときにロードし、自動保存先にする`.ini` |
| `create` | ゲームDLL内でToolを生成する関数 |
| `destroy` | ゲームDLL内でToolを破棄する関数 |

文字列は登録時にEngine側へコピーされます。`id`と`workspaceId`は保存済み設定との
対応に使うため、リリース後は不用意に変更しないでください。

## 3. 既存ゲーム開発プロジェクトへ追加する

既存プロジェクトにはテンプレート変更が自動反映されません。以下の手順で追加します。

1. ゲームのソースディレクトリに`Editor`フォルダを作成する。
2. `GameCameraEditor.h/.cpp`を追加する。
3. `GameEditorExtension.cpp`を追加する。
4. ゲームプロジェクトのVisual Studioプロジェクトを再生成する。
5. 再生成しない場合は、`.vcxproj`へファイルを手動追加する。
6. ゲームDLLをEditorと同じ構成（Debug／Developなど）でビルドする。
7. `.calyxproj`の`gameModules`が生成DLLを指していることを確認する。
8. CalyxEditorでプロジェクトを開き、Toolsメニューを確認する。

手動で`.vcxproj`へ追加する例:

```xml
<ItemGroup>
  <ClCompile Include="Game\Editor\GameCameraEditor.cpp" />
  <ClCompile Include="Game\Editor\GameEditorExtension.cpp" />
</ItemGroup>
<ItemGroup>
  <ClInclude Include="Game\Editor\GameCameraEditor.h" />
</ItemGroup>
```

生成されたゲームプロジェクトはソースディレクトリ以下の`.cpp`を走査するため、
通常はプロジェクト再生成で追加できます。

`.calyxproj`の例:

```json
{
  "gameModules": {
    "Debug": "Generated/Outputs/Debug/MyGame.dll",
    "Develop": "Generated/Outputs/Develop/MyGame.dll",
    "Release": "Generated/Outputs/Release/MyGame.dll"
  }
}
```

EditorとゲームDLLで異なる構成を混在させないでください。特にDebugとDevelopでは
CRT、アサート、リンク対象が異なる可能性があります。

## 4. 新規ゲームプロジェクトの場合

Project Browserから新規作成したプロジェクトには、次のファイルが自動生成されます。

```text
GameEditorExtension.cpp
```

このファイルには、最低限の`GameCameraEditor`と
`RegisterCalyxEditorTools`が含まれています。まず生成サンプルがToolsメニューから
開くことを確認し、その後クラスを`.h/.cpp`へ分割してください。

## 5. DLL登録処理の意味

```cpp
extern "C" __declspec(dllexport)
```

- `extern "C"`はC++の名前修飾を無効にし、固定名で関数を検索可能にします。
- `__declspec(dllexport)`は関数をDLLのExport Tableへ公開します。

Editor側は次の処理を行います。

```cpp
auto function = reinterpret_cast<CalyxEditor::RegisterEditorToolsFn>(
    GetProcAddress(gameModule, "RegisterCalyxEditorTools"));
```

`GetProcAddress`はDLLから関数のアドレスを取得します。関数が存在しないゲームDLLは
Editor拡張なしの通常ゲームDLLとして扱われます。

## 6. Create／Destroyを対にする理由

現在のCalyxEngineとゲームDLLは`/MT`構成です。DLLごとに異なるCRTヒープを使用する
可能性があるため、ゲームDLLで`new`したToolをEngine DLLで直接`delete`してはいけません。

```text
Game.dll:        new GameCameraEditor
Game.dll:        delete GameCameraEditor
```

Engineは`descriptor.destroy`を呼び、生成元DLLに破棄を戻します。

## 7. DLLのライフサイクル

DLLアンロード時は、必ず次の順序になります。

```text
Tool::OnClose
    -> ゲームDLLのdestroy
    -> Registryから登録解除
    -> Game Applicationを破棄
    -> FreeLibrary
```

`FreeLibrary`を先に呼ぶと、`Draw`や`destroy`の関数アドレスが無効になり、
アクセス違反になります。

この構造はUnreal Engineにおける`StartupModule`での登録と
`ShutdownModule`での登録解除に相当します。

## 8. レイアウト管理

現在は`descriptor.layoutPath`で指定した`.ini`をTool起動時に読み込み、同じパスを
ImGuiの自動保存先に設定します。

推奨パス:

```text
Saved/Editor/Layouts/MyGame.Camera.ini
```

複数ユーザーで初期レイアウトを共有する場合は、将来的に次の2種類へ分離します。

```text
Config/Editor/DefaultLayouts/MyGame.Camera.ini
Saved/Editor/Layouts/MyGame.Camera.ini
```

現在使用しているImGui版には`ClearIniSettings()`がないため、レイアウトロードは
既存設定へマージされます。ウィンドウ名には必ず安定した`###ID`を付けてください。

## 9. 複数Toolの登録

同じ関数から複数のToolを登録できます。

```cpp
bool RegisterCalyxEditorTools(
    std::uint32_t apiVersion,
    CalyxEditor::IEditorHost* host) {
    if (apiVersion != CalyxEditor::kEditorToolApiVersion) {
        return false;
    }

    if (!host->RegisterTool(cameraDescriptor)) {
        return false;
    }
    if (!host->RegisterTool(eventDescriptor)) {
        return false;
    }
    return true;
}
```

途中で登録に失敗した場合、EngineはそのDLLが登録したToolを一括解除します。

## 10. トラブルシューティング

### Toolsメニューに表示されない

- `RegisterCalyxEditorTools`に`extern "C"`と`dllexport`があるか確認する。
- ゲームDLLが最新状態でビルドされているか確認する。
- `.calyxproj`のDLLパスと起動構成を確認する。
- Tool IDが別のToolと重複していないか確認する。
- APIバージョン判定で`false`を返していないか確認する。

Windows Developer Command Promptでは次のコマンドでExportを確認できます。

```powershell
dumpbin /exports MyGame.dll | Select-String RegisterCalyxEditorTools
```

### Toolを閉じた後に再表示できない

`OnOpen()`でウィンドウ表示フラグを`true`へ戻してください。

```cpp
void GameCameraEditor::OnOpen() {
    open_ = true;
}
```

### 終了時にアクセス違反になる

- Engine側でToolを直接`delete`していないか確認する。
- `destroy`関数がゲームDLL内にあるか確認する。
- DLLアンロード前にTool登録を解除しているか確認する。

