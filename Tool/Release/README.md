# Calyx Release Tool

このツールはエンジン開発者向けの Release パッケージ作成ツールです。GitHub Releases にアップロードする zip を作成します。

## 使い方

エンジンルートから実行します。

```powershell
.\Tool\Release\PackageRelease.cmd -Version 1.2.0 -Configuration Release
```

最終的な GitHub Release asset URL を指定する場合:

```powershell
.\Tool\Release\PackageRelease.cmd -Version 1.2.0 -Configuration Release -DownloadUrl "https://github.com/<owner>/<repo>/releases/download/v1.2.0/CalyxEngine-1.2.0.zip"
```

出力先:

```txt
generated/packages/CalyxEngine-<version>.zip
generated/packages/latest.json
```

この 2 ファイルを GitHub Release にアップロードします。`CalyxLauncher` はこのエンジンリポジトリの外側に配置し、GitHub Releases 上の `latest.json` を参照する想定です。

## GitHub Release 自動化

`.github/workflows/ReleasePackage.yml` workflow により、Release パッケージの作成と GitHub Release へのアップロードを自動化できます。

タグから公開する場合:

```powershell
git tag v1.2.0
git push origin v1.2.0
```

workflow は次のファイルをアップロードします。

```txt
generated/packages/CalyxEngine-1.2.0.zip
generated/packages/latest.json
```

GitHub Actions から `ReleasePackage` を手動実行することもできます。その場合は、先頭の `v` を除いた version を入力してください。
