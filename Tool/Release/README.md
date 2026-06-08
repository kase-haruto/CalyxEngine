# Calyx Release Tool

This tool is for engine developers. It creates the zip uploaded to GitHub Releases.

## Usage

From the engine root:

```powershell
.\Tool\Release\PackageRelease.cmd -Version 1.2.0 -Configuration Release
```

With the final GitHub Release asset URL:

```powershell
.\Tool\Release\PackageRelease.cmd -Version 1.2.0 -Configuration Release -DownloadUrl "https://github.com/<owner>/<repo>/releases/download/v1.2.0/CalyxEngine-1.2.0.zip"
```

Outputs are written to:

```txt
generated/packages/CalyxEngine-<version>.zip
generated/packages/latest.json
```

Upload both files to the GitHub Release. `CalyxLauncher` should live outside this engine repository and consume `latest.json` from GitHub Releases.
