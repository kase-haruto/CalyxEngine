param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [string]$Configuration = "Develop",
    [string]$Platform = "x64",
    [string]$DownloadUrl = "",
    [switch]$SkipBuild,
    [switch]$IncludeSymbols
)
& 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1'

$ErrorActionPreference = "Stop"

function Resolve-EngineRoot {
    $scriptPath = if (-not [string]::IsNullOrWhiteSpace($PSCommandPath)) { $PSCommandPath } else { $MyInvocation.MyCommand.Path }
    $scriptDir = Split-Path -Parent $scriptPath
    return (Resolve-Path (Join-Path $scriptDir "..\..")).Path
}

function Find-MSBuild {
    if (-not [string]::IsNullOrWhiteSpace($env:MSBUILD_EXE_PATH) -and (Test-Path $env:MSBUILD_EXE_PATH)) {
        return $env:MSBUILD_EXE_PATH
    }

    $pathMsBuild = Get-Command msbuild.exe -ErrorAction SilentlyContinue
    if ($pathMsBuild) {
        return $pathMsBuild.Source
    }

    $vsWhereCandidates = @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
    )

    foreach ($vsWhere in $vsWhereCandidates) {
        if (Test-Path $vsWhere) {
            $installPath = & $vsWhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
            if (-not [string]::IsNullOrWhiteSpace($installPath)) {
                $candidate = Join-Path $installPath "MSBuild\Current\Bin\amd64\MSBuild.exe"
                if (Test-Path $candidate) {
                    return $candidate
                }

                $candidate = Join-Path $installPath "MSBuild\Current\Bin\MSBuild.exe"
                if (Test-Path $candidate) {
                    return $candidate
                }
            }
        }
    }

    $candidates = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2026\Community\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2026\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2026\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2026\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2017\Professional\MSBuild\15.0\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin\amd64\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2017\BuildTools\MSBuild\15.0\Bin\amd64\MSBuild.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "MSBuild was not found. Install Visual Studio or Build Tools, or add MSBuild to PATH."
}

function Copy-RequiredItem {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    if (-not (Test-Path $Source)) {
        throw "Required package input was not found: $Source"
    }

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
    Copy-Item -Force -Path $Source -Destination $Destination
}

function Copy-RequiredDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    if (-not (Test-Path $Source)) {
        throw "Required package input directory was not found: $Source"
    }

    if (Test-Path $Destination) {
        Remove-Item -Recurse -Force -Path $Destination
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
    Copy-Item -Recurse -Force -Path $Source -Destination $Destination
}

function Copy-HeaderTree {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    if (-not (Test-Path $Source)) {
        throw "Header source directory was not found: $Source"
    }

    $extensions = @(".h", ".hpp", ".hxx", ".hh", ".inl", ".ipp")
    $sourceRoot = (Resolve-Path $Source).Path.TrimEnd("\") + "\"
    Get-ChildItem -Recurse -File -Path $Source | Where-Object { $extensions -contains $_.Extension.ToLowerInvariant() } | ForEach-Object {
        $relativePath = $_.FullName.Substring($sourceRoot.Length)
        $outputPath = Join-Path $Destination $relativePath
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $outputPath) | Out-Null
        Copy-Item -Force -Path $_.FullName -Destination $outputPath
    }
}

function Copy-IfExists {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    if (Test-Path $Source) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
        Copy-Item -Force -Path $Source -Destination $Destination
    }
}

$engineRoot = Resolve-EngineRoot
$projectDir = Join-Path $engineRoot "project"
$outputRoot = Join-Path $engineRoot "generated\packages"
$stagingParent = Join-Path $engineRoot "generated\package-staging"
$packageName = "CalyxEngine-$Version"
$stageRoot = Join-Path $stagingParent $packageName
$zipPath = Join-Path $outputRoot "$packageName.zip"
$manifestPath = Join-Path $outputRoot "latest.json"

if (-not (Test-Path $projectDir)) {
    throw "Calyx project directory was not found: $projectDir"
}

if (-not $SkipBuild) {
    $msbuild = Find-MSBuild
    & $msbuild (Join-Path $projectDir "CalyxEngine.sln") /p:Configuration=$Configuration /p:Platform=$Platform /m
    if ($LASTEXITCODE -ne 0) {
        throw "MSBuild failed with exit code $LASTEXITCODE"
    }
}

if (Test-Path $stageRoot) {
    Remove-Item -Recurse -Force -Path $stageRoot
}
New-Item -ItemType Directory -Force -Path $stageRoot | Out-Null
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

$runtimeOutput = Join-Path $engineRoot "generated\outputs\$Configuration"
Copy-RequiredItem (Join-Path $runtimeOutput "CalyxEditor.exe") (Join-Path $stageRoot "CalyxEditor.exe")
Copy-RequiredItem (Join-Path $runtimeOutput "CalyxLauncher.exe") (Join-Path $stageRoot "SDK\CalyxLauncher.exe")
Copy-IfExists (Join-Path $runtimeOutput "dxcompiler.dll") (Join-Path $stageRoot "dxcompiler.dll")
Copy-IfExists (Join-Path $runtimeOutput "dxil.dll") (Join-Path $stageRoot "dxil.dll")
Copy-IfExists (Join-Path $runtimeOutput "assimp-vc143-mt.dll") (Join-Path $stageRoot "assimp-vc143-mt.dll")
Copy-IfExists (Join-Path $runtimeOutput "assimp-vc143-mtd.dll") (Join-Path $stageRoot "assimp-vc143-mtd.dll")

if ($IncludeSymbols) {
    Copy-IfExists (Join-Path $runtimeOutput "CalyxEditor.pdb") (Join-Path $stageRoot "CalyxEditor.pdb")
    Copy-IfExists (Join-Path $runtimeOutput "CalyxEngineLib.pdb") (Join-Path $stageRoot "CalyxEngineLib.pdb")
}

Copy-RequiredDirectory (Join-Path $projectDir "Resources") (Join-Path $stageRoot "Resources")
Copy-RequiredDirectory (Join-Path $projectDir "Templates") (Join-Path $stageRoot "Templates")

$sdkRoot = Join-Path $stageRoot "SDK"
$includeRoot = Join-Path $sdkRoot "Include"
$thirdPartyRoot = Join-Path $sdkRoot "ThirdParty"

Copy-HeaderTree (Join-Path $projectDir "Engine") (Join-Path $includeRoot "Engine")
Copy-HeaderTree (Join-Path $projectDir "Data") (Join-Path $includeRoot "Data")
Copy-HeaderTree (Join-Path $projectDir "externals") (Join-Path $thirdPartyRoot "externals")
Copy-HeaderTree (Join-Path $projectDir "externals\DirectXTex") (Join-Path $thirdPartyRoot "DirectXTex")
Copy-RequiredDirectory (Join-Path $projectDir "externals\assimp\include") (Join-Path $thirdPartyRoot "assimp\include")
Copy-RequiredDirectory (Join-Path $projectDir "Engine\Application\CalyxEngine") (Join-Path $includeRoot "CalyxEngine")

Copy-RequiredItem (Join-Path $runtimeOutput "CalyxEngineLib.lib") (Join-Path $sdkRoot "Lib\$Configuration\CalyxEngineLib.lib")

$directXTexConfig = if ($Configuration -eq "Debug") { "Debug" } else { "Release" }
Copy-RequiredItem (Join-Path $projectDir "externals\DirectXTex\generated\bin\DirectXTex\x64\$directXTexConfig\DirectXTex.lib") (Join-Path $sdkRoot "Lib\DirectXTex\x64\$directXTexConfig\DirectXTex.lib")

$assimpConfig = if ($Configuration -eq "Debug") { "Debug" } else { "Release" }
$assimpLibName = if ($Configuration -eq "Debug") { "assimp-vc143-mtd.lib" } else { "assimp-vc143-mt.lib" }
Copy-RequiredItem (Join-Path $projectDir "externals\assimp\lib\$assimpConfig\$assimpLibName") (Join-Path $sdkRoot "Lib\assimp\$assimpConfig\$assimpLibName")

$engineManifest = [ordered]@{
    version = $Version
    configuration = $Configuration
    platform = $Platform
    editorExe = "CalyxEditor.exe"
    sdkDir = "SDK"
    resourcesDir = "Resources"
    templatesDir = "Templates"
}
$engineManifest | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 -Path (Join-Path $stageRoot "engine.json")

if ([string]::IsNullOrWhiteSpace($DownloadUrl)) {
    $DownloadUrl = "https://github.com/<owner>/<repo>/releases/download/v$Version/$packageName.zip"
}

$latestManifest = [ordered]@{
    version = $Version
    downloadUrl = $DownloadUrl
    editorExe = "CalyxEditor.exe"
    sdkDir = "SDK"
    packageFile = "$packageName.zip"
}
$latestManifest | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 -Path $manifestPath

if (Test-Path $zipPath) {
    Remove-Item -Force -Path $zipPath
}
Compress-Archive -Path (Join-Path $stageRoot "*") -DestinationPath $zipPath -Force

Write-Host "Package created: $zipPath"
Write-Host "Manifest created: $manifestPath"


