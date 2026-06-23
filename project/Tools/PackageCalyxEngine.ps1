param(
	# 作成するパッケージのバージョン。
	# 例: 1.0.8 または v1.0.8
	[Parameter(Mandatory = $true)]
	[string]$Version,

	# ルートに配置する実行ファイルの構成。
	# SDK 内には Debug / Develop / Release をすべて含める。
	[string]$Configuration = "Develop",

	# zip の出力先。
	# 未指定の場合は generated\packages に出力する。
	[string]$OutputRoot = ""
)

$ErrorActionPreference = 'Stop'

$validConfigurations = @('Debug', 'Develop', 'Release')
if ($Configuration -notin $validConfigurations) {
	throw "Configuration must be one of: $($validConfigurations -join ', ')"
}

# このスクリプトは、生成されたゲームプロジェクトが利用する
# CalyxEngine のランタイム SDK パッケージを作成する。
#
# この zip はソースコード一式ではなく、ゲーム側がビルド・実行するために必要な
# exe / dll / lib / header / resources をまとめたリリース用パッケージである。
# CalyxLauncher は、このパッケージ構成を前提に検証・インストールを行う。

# project フォルダとリポジトリルートを取得する。
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$repoRoot = (Resolve-Path (Join-Path $projectRoot '..')).Path

# 出力先が指定されていない場合は、リポジトリ配下の generated\packages を使う。
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
	$OutputRoot = Join-Path $repoRoot 'generated\packages'
}

# バージョン表記を v 始まりに統一する。
# 例: 1.0.8 -> v1.0.8
$normalizedVersion = $Version
if (-not $normalizedVersion.StartsWith('v')) {
	$normalizedVersion = "v$normalizedVersion"
}

# ビルド成果物、ステージング先、zip 出力先のパスを定義する。
$outputsRoot = Join-Path $repoRoot 'generated\outputs'
$stagingRoot = Join-Path $repoRoot 'generated\package-staging'
$packageName = "CalyxGamePackage-$normalizedVersion"
$stagingRunRoot = Join-Path $stagingRoot ($packageName + '-' + [System.Guid]::NewGuid().ToString('N'))
$packageRoot = Join-Path $stagingRunRoot $packageName
$zipPath = Join-Path $OutputRoot "$packageName.zip"

function Copy-DirectoryClean {
	param(
		[Parameter(Mandatory = $true)][string]$Source,
		[Parameter(Mandatory = $true)][string]$Destination
	)

	# 必須ディレクトリが存在しない場合は、パッケージ不完全としてエラーにする。
	if (-not (Test-Path $Source)) {
		throw "Required directory was not found: $Source"
	}

	# コピー先が既に存在する場合は、一度削除してからコピーする。
	# 古いファイルが残らないようにするため。
	if (Test-Path $Destination) {
		Remove-Item -LiteralPath $Destination -Recurse -Force
	}

	New-Item -ItemType Directory -Force -Path (Split-Path $Destination -Parent) | Out-Null
	Copy-Item -LiteralPath $Source -Destination $Destination -Recurse -Force
}

function Remove-DirectoryClean {
	param(
		[Parameter(Mandatory = $true)][string]$Path
	)

	if (-not (Test-Path $Path)) {
		return
	}

	Get-ChildItem -LiteralPath $Path -Recurse -Force -ErrorAction SilentlyContinue |
		ForEach-Object {
			try {
				$_.Attributes = 'Normal'
			} catch {
			}
		}

	Remove-Item -LiteralPath $Path -Recurse -Force
}

function Copy-FileRequired {
	param(
		[Parameter(Mandatory = $true)][string]$Source,
		[Parameter(Mandatory = $true)][string]$Destination
	)

	# 必須ファイルが存在しない場合は、リリースパッケージを作成しない。
	if (-not (Test-Path $Source)) {
		throw "Required file was not found: $Source"
	}

	New-Item -ItemType Directory -Force -Path (Split-Path $Destination -Parent) | Out-Null
	Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

function Copy-FileIfExists {
	param(
		[Parameter(Mandatory = $true)][string]$Source,
		[Parameter(Mandatory = $true)][string]$Destination
	)

	# 任意ファイルは、存在する場合のみコピーする。
	# 例: dxcompiler.dll / dxil.dll / libcurl.dll など。
	if (Test-Path $Source) {
		New-Item -ItemType Directory -Force -Path (Split-Path $Destination -Parent) | Out-Null
		Copy-Item -LiteralPath $Source -Destination $Destination -Force
	}
}

function Copy-HeadersOnly {
	param(
		[Parameter(Mandatory = $true)][string]$Source,
		[Parameter(Mandatory = $true)][string]$Destination
	)

	# ヘッダーのコピー元が存在しない場合はエラーにする。
	if (-not (Test-Path $Source)) {
		throw "Required header directory was not found: $Source"
	}

	# SDK には cpp などの実装ファイルを含めず、
	# 公開に必要な .h / .hpp / .inl のみをコピーする。
	Get-ChildItem -LiteralPath $Source -Recurse -File |
		Where-Object { $_.Extension -in '.h', '.hpp', '.inl' } |
		ForEach-Object {
			$relativePath = $_.FullName.Substring($Source.Length).TrimStart('\', '/')
			Copy-FileRequired $_.FullName (Join-Path $Destination $relativePath)
		}
}

function Copy-HeadersIfExists {
	param(
		[Parameter(Mandatory = $true)][string]$Source,
		[Parameter(Mandatory = $true)][string]$Destination
	)

	# 任意のヘッダーディレクトリは、存在する場合のみ SDK に含める。
	# 例: Runtime はプロジェクト構成によって存在しないことがある。
	if (Test-Path $Source) {
		Copy-HeadersOnly $Source $Destination
	}
}

# パッケージ作成に必要なディレクトリを用意する。
New-Item -ItemType Directory -Force -Path $packageRoot | Out-Null
New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

# ルートに配置するランタイムは、指定された Configuration の成果物を使う。
$runtimeOutput = Join-Path $outputsRoot $Configuration

# パッケージ直下には、ランチャー検証用および手動動作確認用の
# 最小限の実行ファイル・DLL を配置する。
#
# 生成されたゲームプロジェクトは、実際には
# SDK\Bin\<Configuration>\CalyxGame.exe を参照する。
# これにより Develop ビルドで誤って Release 用のホストを実行することを防ぐ。
Copy-FileRequired (Join-Path $runtimeOutput 'CalyxGame.exe') (Join-Path $packageRoot 'CalyxGame.exe')
Copy-FileRequired (Join-Path $runtimeOutput 'CalyxEngine.dll') (Join-Path $packageRoot 'CalyxEngine.dll')
Copy-FileIfExists (Join-Path $runtimeOutput 'dxcompiler.dll') (Join-Path $packageRoot 'dxcompiler.dll')
Copy-FileIfExists (Join-Path $runtimeOutput 'dxil.dll') (Join-Path $packageRoot 'dxil.dll')
Copy-FileIfExists (Join-Path $runtimeOutput 'libcurl.dll') (Join-Path $packageRoot 'libcurl.dll')

# エンジン標準のシェーダ、マテリアル、エディタ設定、初期シーンなどを
# ゲームプロジェクト側から参照できるように Resources を同梱する。
Copy-DirectoryClean (Join-Path $projectRoot 'Resources') (Join-Path $packageRoot 'Resources')

# SDK 配下の主要ディレクトリを定義する。
$sdkRoot = Join-Path $packageRoot 'SDK'
$sdkInclude = Join-Path $sdkRoot 'Include'
$sdkLib = Join-Path $sdkRoot 'Lib'
$sdkBin = Join-Path $sdkRoot 'Bin'
$sdkThirdParty = Join-Path $sdkRoot 'ThirdParty'

# ゲーム側が include するためのエンジン公開ヘッダーをコピーする。
# 既存の include パスを壊さないように Engine / Data / Runtime の構造を維持する。
Copy-HeadersOnly (Join-Path $projectRoot 'Engine') (Join-Path $sdkInclude 'Engine')
Copy-HeadersOnly (Join-Path $projectRoot 'Data') (Join-Path $sdkInclude 'Data')
Copy-HeadersIfExists (Join-Path $projectRoot 'Runtime') (Join-Path $sdkInclude 'Runtime')

# 安定した公開 API として使う CalyxEngine ラッパーヘッダーを、
# SDK\Include\CalyxEngine に配置する。
Copy-DirectoryClean (Join-Path $sdkInclude 'Engine\Application\CalyxEngine') (Join-Path $sdkInclude 'CalyxEngine')

# エンジンの公開ヘッダー内で externals/imgui/imgui.h などを参照しているため、
# SDK の Include 配下にもサードパーティのヘッダーを配置する。
Copy-HeadersOnly (Join-Path $projectRoot 'externals') (Join-Path $sdkInclude 'externals')

# 生成されるゲームプロジェクトでは、明示的なサードパーティ include パスとして
# SDK\ThirdParty を使用するため、必要なヘッダーを別名として配置する。
Copy-HeadersOnly (Join-Path $projectRoot 'externals\nlohmann') (Join-Path $sdkThirdParty 'nlohmann')
Copy-HeadersOnly (Join-Path $projectRoot 'externals\DirectXTex') (Join-Path $sdkThirdParty 'DirectXTex')
Copy-DirectoryClean (Join-Path $projectRoot 'externals\assimp\include') (Join-Path $sdkThirdParty 'assimp\include')

# ゲームプロジェクトが各構成でリンク・実行できるように、
# Debug / Develop / Release それぞれの lib / dll / exe を SDK に含める。
foreach ($config in @('Debug', 'Develop', 'Release')) {
	$configOutput = Join-Path $outputsRoot $config

	Copy-FileRequired (Join-Path $configOutput 'CalyxEngine.lib') (Join-Path $sdkLib "$config\CalyxEngine.lib")
	Copy-FileRequired (Join-Path $configOutput 'CalyxEngine.dll') (Join-Path $sdkBin "$config\CalyxEngine.dll")
	Copy-FileRequired (Join-Path $configOutput 'CalyxGame.exe') (Join-Path $sdkBin "$config\CalyxGame.exe")

	# DirectX Shader Compiler などのランタイム DLL は、存在する場合のみ同梱する。
	Copy-FileIfExists (Join-Path $configOutput 'dxcompiler.dll') (Join-Path $sdkBin "$config\dxcompiler.dll")
	Copy-FileIfExists (Join-Path $configOutput 'dxil.dll') (Join-Path $sdkBin "$config\dxil.dll")
	Copy-FileIfExists (Join-Path $configOutput 'libcurl.dll') (Join-Path $sdkBin "$config\libcurl.dll")
}

# 生成されたゲーム vcxproj が参照するサードパーティ static library をコピーする。
Copy-FileRequired (Join-Path $projectRoot 'externals\DirectXTex\generated\bin\DirectXTex\x64\Debug\DirectXTex.lib') (Join-Path $sdkLib 'DirectXTex\x64\Debug\DirectXTex.lib')
Copy-FileRequired (Join-Path $projectRoot 'externals\DirectXTex\generated\bin\DirectXTex\x64\Release\DirectXTex.lib') (Join-Path $sdkLib 'DirectXTex\x64\Release\DirectXTex.lib')
Copy-DirectoryClean (Join-Path $projectRoot 'externals\assimp\lib\Debug') (Join-Path $sdkLib 'assimp\Debug')
Copy-DirectoryClean (Join-Path $projectRoot 'externals\assimp\lib\Release') (Join-Path $sdkLib 'assimp\Release')

# パッケージの情報を記録するマニフェストを作成する。
# ランチャーや将来の検証処理で、パッケージ内容を確認するために使用する。
$manifest = [ordered]@{
	name = 'CalyxEngine'
	version = $normalizedVersion
	packageKind = 'GameRuntimeSdk'
	runtimeConfiguration = $Configuration
	createdAtUtc = (Get-Date).ToUniversalTime().ToString('o')
	requiredFiles = @(
		'CalyxGame.exe',
		'CalyxEngine.dll',
		'SDK\Bin\Debug\CalyxGame.exe',
		'SDK\Bin\Debug\CalyxEngine.dll',
		'SDK\Bin\Develop\CalyxGame.exe',
		'SDK\Bin\Develop\CalyxEngine.dll',
		'SDK\Bin\Release\CalyxGame.exe',
		'SDK\Bin\Release\CalyxEngine.dll',
		'SDK\Include\CalyxEngine\Application.h',
		'SDK\Lib\Debug\CalyxEngine.lib',
		'SDK\Lib\Develop\CalyxEngine.lib',
		'SDK\Lib\Release\CalyxEngine.lib'
	)
}
$manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $packageRoot 'engine-package.json') -Encoding UTF8

# zip 化する前に、最低限必要なファイルが揃っているか検証する。
# ここで検出することで、ゲーム開発者がプロジェクト作成・ビルド時に
# 不完全な SDK を受け取ってしまうことを防ぐ。
$requiredPackageFiles = @(
	'CalyxGame.exe',
	'CalyxEngine.dll',
	'SDK\Bin\Debug\CalyxGame.exe',
	'SDK\Bin\Debug\CalyxEngine.dll',
	'SDK\Bin\Develop\CalyxGame.exe',
	'SDK\Bin\Develop\CalyxEngine.dll',
	'SDK\Bin\Release\CalyxGame.exe',
	'SDK\Bin\Release\CalyxEngine.dll',
	'SDK\Include\CalyxEngine\Application.h',
	'SDK\Include\Data\Engine',
	'SDK\Include\externals\nlohmann\json.hpp',
	'SDK\Lib\Debug\CalyxEngine.lib',
	'SDK\Lib\Develop\CalyxEngine.lib',
	'SDK\Lib\Release\CalyxEngine.lib'
)

foreach ($relativePath in $requiredPackageFiles) {
	$absolutePath = Join-Path $packageRoot $relativePath
	if (-not (Test-Path $absolutePath)) {
		throw "Package validation failed. Missing: $relativePath"
	}
}

# 既に同名の zip が存在する場合は削除してから作り直す。
if (Test-Path $zipPath) {
	Remove-Item -LiteralPath $zipPath -Force
}

# ステージングしたパッケージを zip に圧縮する。
Compress-Archive -Path $packageRoot -DestinationPath $zipPath -CompressionLevel Optimal

Remove-Item -LiteralPath $stagingRunRoot -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "CalyxEngine game runtime SDK package created: $zipPath"
