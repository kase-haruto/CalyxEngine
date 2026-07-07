# ゲーム固有エディタ拡張

## 目的

エンジン標準のエディタを変更せず、ゲーム DLL からカメラ編集などのゲーム固有ツールを追加するための仕組みです。

拡張 API は `Engine/Editor/Extension/EditorToolAPI.h` にあります。ゲーム DLL は `RegisterCalyxEditorTools` を export し、`IEditorHost::RegisterTool` へツールを登録します。

## ライフサイクル

1. Editor がゲーム DLL をロードする
2. `RegisterCalyxEditorTools` を呼ぶ
3. Tools メニューからツールを開くと、ゲーム DLL の `create` が呼ばれる
4. 毎フレーム `Update` と `Draw` が呼ばれる
5. `IsOpen` が false を返すと、Engine が `OnClose` を呼んだ後、ゲーム DLL の `destroy` で破棄する
6. プロジェクト切り替え時は全ツールを破棄してからゲーム DLL をアンロードする

生成と破棄を同じ DLL 内で行うため、Engine とゲーム DLL の間で C++ ヒープ所有権を移動しません。

## EditorToolContext

API v2 の Context は次のサービスを提供します。

- 現在のプロジェクト
- SceneManager
- Primary Selection
- Main Camera
- Play 状態
- 編集シーンの保存要求

Context が返す `SceneObject` と `BaseCamera` は非所有ポインタです。ツールは保持せず、必要なタイミングで Context から再取得してください。シーン切り替えや Play 開始により、以前取得したポインタは無効になる可能性があります。

## 登録例

```cpp
extern "C" __declspec(dllexport) bool RegisterCalyxEditorTools(
    std::uint32_t apiVersion,
    CalyxEditor::IEditorHost* host) {
    if (!host || apiVersion != CalyxEditor::kEditorToolApiVersion) {
        return false;
    }

    CalyxEditor::EditorToolDescriptor descriptor;
    descriptor.id = "Game.CameraEditor";
    descriptor.displayName = "Game Camera Editor";
    descriptor.menuPath = "Game/Camera";
    descriptor.workspaceId = "Game.Camera";
    descriptor.layoutPath = "GameCameraEditor.ini";
    descriptor.create = &CreateGameCameraEditor;
    descriptor.destroy = &DestroyGameCameraEditor;
    return host->RegisterTool(descriptor);
}
```

`menuPath` は Tools メニュー以下の完全な階層です。上の例は `Tools > Game > Camera` に表示されます。

## 既存プロジェクトへの導入

既存の `.calyxproj` には移行スクリプトを使用できます。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File `
  Tools/Add-GameEditorExtension.ps1 `
  -ProjectFile "C:\Projects\MyGame\MyGame.calyxproj"
```

スクリプトは `Game/Editor/GameEditorExtension.cpp` を作成し、同名の `.vcxproj` と `.vcxproj.filters` にコンパイル対象として追加します。既存ファイルは既定では上書きしません。空の雛形などを意図的に置き換える場合だけ `-Force` を指定してください。

生成される互換ツールは API v1 と v2 の両方でビルドできるよう、CameraManager経由でメインカメラを取得します。API v2専用のSelection、Play状態、Scene保存サービスを利用する場合は、プロジェクトのEngine SDKもAPI v2を含む版へ更新してください。

## ABIルール

- API バージョンは完全一致させる
- `create` と `destroy` は必ず登録元 DLL の関数にする
- Context のポインタを delete しない
- STL コンテナや `unique_ptr` の所有権を DLL 境界で渡さない
- ツールインスタンスを残したままゲーム DLL をアンロードしない

## 現在の制限

- 汎用 Undo/Redo トランザクション API は未公開
- `workspaceId` は予約フィールドであり、現在はレイアウト識別には使われない
- Viewport overlay、Asset picker、Dirty 通知は今後 Context のサービスとして追加する
