param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [string[]]$ScanRoots = @("Engine"),
    [string]$OutputDir = "Engine\Foundation\Reflection",
    [string]$OutputName = "CalyxObjectRegistry.generated",
    [string]$Namespace = "CalyxEngine",
    [string]$FunctionName = "RegisterGeneratedSceneObjects",
    [string]$ApiMacro = "",
    [string]$ProjectFile = ""
)

$ErrorActionPreference = "Stop"

$Root = [System.IO.Path]::GetFullPath($Root)

# A game build can derive all paths from its .calyxproj. Explicit command-line
# values remain supported for engine builds and older generated projects.
if (![string]::IsNullOrWhiteSpace($ProjectFile)) {
    $projectPath = [System.IO.Path]::GetFullPath($ProjectFile)
    if (!(Test-Path -LiteralPath $projectPath)) {
        throw "Calyx project file was not found: $projectPath"
    }
    $project = Get-Content -LiteralPath $projectPath -Raw | ConvertFrom-Json
    $Root = Split-Path -Parent $projectPath
    $sourceRoot = if ($project.sourceDirectory) { $project.sourceDirectory } elseif ($project.SourceRoot) { $project.SourceRoot } else { "Game" }
    $generatedRoot = if ($project.generatedDirectory) { $project.generatedDirectory } elseif ($project.GeneratedRoot) { $project.GeneratedRoot } else { "Generated" }
    $ScanRoots = @([string]$sourceRoot)
    $OutputDir = Join-Path ([string]$generatedRoot) "Foundation\Reflection"
}

$mutexName = "Global\CalyxReflectionGenerator_" + [Convert]::ToBase64String(
    [System.Text.Encoding]::UTF8.GetBytes($Root)
).Replace("+", "-").Replace("/", "_").TrimEnd("=")
$mutex = [System.Threading.Mutex]::new($false, $mutexName)
$hasLock = $false

try {
$hasLock = $mutex.WaitOne([TimeSpan]::FromMinutes(5))
if (!$hasLock) {
    throw "Timed out waiting for reflection generator lock: $mutexName"
}

$generatedDir = Join-Path $Root $OutputDir
$outHeader = Join-Path $generatedDir "$OutputName.h"
$outSource = Join-Path $generatedDir "$OutputName.cpp"

function Convert-ToCppString([string]$value) {
    return $value.Replace('\', '\\').Replace('"', '\"')
}

function Write-IfChanged([string]$path, [string]$text) {
    if ((Test-Path $path) -and ((Get-Content $path -Raw) -eq $text)) {
        return
    }
    $dir = Split-Path $path -Parent
    if (!(Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir | Out-Null
    }
    [System.IO.File]::WriteAllText($path, $text, [System.Text.UTF8Encoding]::new($false))
}

function Get-RelativePathCompat([string]$basePath, [string]$targetPath) {
    $baseFull = [System.IO.Path]::GetFullPath($basePath)
    if (!$baseFull.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $baseFull += [System.IO.Path]::DirectorySeparatorChar
    }
    $targetFull = [System.IO.Path]::GetFullPath($targetPath)
    $baseUri = [System.Uri]::new($baseFull)
    $targetUri = [System.Uri]::new($targetFull)
    return [System.Uri]::UnescapeDataString($baseUri.MakeRelativeUri($targetUri).ToString()).Replace('/', '\')
}

$objectRegex = [regex]::new(
    'CALYX_OBJECT\s*\((?<meta>.*?)\)\s*class\s+(?:(?:[A-Z_][A-Z0-9_]*|__declspec\s*\([^)]*\))\s+)*(?<class>[A-Za-z_][A-Za-z0-9_]*)(?:\s+(?:final|abstract))*\s*:',
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

$entries = New-Object System.Collections.Generic.List[object]

foreach ($scanRoot in $ScanRoots) {
    $resolvedScanRoot = if ([System.IO.Path]::IsPathRooted($scanRoot)) { $scanRoot } else { Join-Path $Root $scanRoot }
    if (!(Test-Path $resolvedScanRoot)) {
        continue
    }

    Get-ChildItem -Path $resolvedScanRoot -Recurse -Filter *.h | ForEach-Object {
        $path = $_.FullName
        $text = Get-Content $path -Raw
        foreach ($match in $objectRegex.Matches($text)) {
            $meta = @{}
            foreach ($part in ($match.Groups["meta"].Value -replace "`r|`n", " ").Split(",")) {
                if (!$part.Contains("=")) {
                    continue
                }
                $pair = $part.Split("=", 2)
                $meta[$pair[0].Trim()] = $pair[1].Trim().Trim('"')
            }

            $className = $match.Groups["class"].Value
            $typeName = if ($meta.ContainsKey("TypeName")) { $meta["TypeName"] } else { $className }
            $displayName = if ($meta.ContainsKey("DisplayName")) { $meta["DisplayName"] } else { $typeName }
            $category = if ($meta.ContainsKey("Category")) { $meta["Category"] } else { "None" }
            $icon = if ($meta.ContainsKey("Icon")) { $meta["Icon"] } else { "UI/Tool/event.png" }
            $placeable = if ($meta.ContainsKey("Placeable")) { $meta["Placeable"].ToLower() -ne "false" } else { $true }
            $prefabEditable = if ($meta.ContainsKey("PrefabEditable")) { $meta["PrefabEditable"].ToLower() -eq "true" } else { $false }
            $prefabRoot = if ($meta.ContainsKey("PrefabRoot")) { $meta["PrefabRoot"].ToLower() -eq "true" } else { $false }
            $sceneSerializable = if ($meta.ContainsKey("SceneSerializable")) { $meta["SceneSerializable"].ToLower() -ne "false" } else { $placeable }
            $prefabSerializable = if ($meta.ContainsKey("PrefabSerializable")) { $meta["PrefabSerializable"].ToLower() -ne "false" } else { $true }
            $include = (Get-RelativePathCompat $Root $path).Replace('\', '/')

            $entries.Add([pscustomobject]@{
                ClassName = $className
                TypeName = $typeName
                DisplayName = $displayName
                Category = $category
                Icon = $icon
                Placeable = if ($placeable) { "true" } else { "false" }
                PrefabEditable = if ($prefabEditable) { "true" } else { "false" }
                PrefabRoot = if ($prefabRoot) { "true" } else { "false" }
                SceneSerializable = if ($sceneSerializable) { "true" } else { "false" }
                PrefabSerializable = if ($prefabSerializable) { "true" } else { "false" }
                Include = $include
            })
        }
    }
}

$entries = $entries | Sort-Object TypeName

$apiInclude = if ([string]::IsNullOrWhiteSpace($ApiMacro)) { "" } else { "#include <Engine/Foundation/Export/CalyxAPI.h>`n`n" }
$apiPrefix = if ([string]::IsNullOrWhiteSpace($ApiMacro)) { "" } else { "$ApiMacro " }

$header = @"
#pragma once

$apiInclude
namespace CalyxEngine {
	${apiPrefix}void RegisterGeneratedSceneObjects();
}
"@
$header = $header.Replace("namespace CalyxEngine", "namespace $Namespace").Replace("RegisterGeneratedSceneObjects", $FunctionName)

$includeLines = ($entries | ForEach-Object { "#include <$($_.Include)>" }) -join "`n"
$registrationBlocks = ($entries | ForEach-Object {
    $typeName = Convert-ToCppString $_.TypeName
    $displayName = Convert-ToCppString $_.DisplayName
    $icon = Convert-ToCppString $_.Icon
@"
		SceneObjectRegistry::Get().Register(
			"$typeName",
			"$displayName",
			ObjectType::$($_.Category),
			"$icon",
			$($_.Placeable),
			$($_.PrefabEditable),
			$($_.PrefabRoot),
			$($_.SceneSerializable),
			$($_.PrefabSerializable),
			&CreateSceneObject<$($_.ClassName)>);
"@
}) -join "`n`n"

$source = @"
// This file is generated by Tools/Reflection/generate_reflection.ps1.
// Do not edit by hand.

#include <$($OutputDir.Replace('\', '/'))/$OutputName.h>

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

$includeLines

namespace $Namespace {
	void $FunctionName() {
$registrationBlocks
	}
}
"@

Write-IfChanged $outHeader $header
Write-IfChanged $outSource $source
}
finally {
    if ($hasLock) {
        $mutex.ReleaseMutex()
    }
    $mutex.Dispose()
}
