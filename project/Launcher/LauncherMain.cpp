#include <externals/nlohmann/json.hpp>

#include <Windows.h>
#include <shellapi.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

	struct ProjectInfo {
		std::string name;
		std::string engineVersion;
	};

	std::vector<std::wstring> SplitCommandLineArguments() {
		int argc = 0;
		LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
		std::vector<std::wstring> args;
		if(!argv) {
			return args;
		}

		for(int i = 1; i < argc; ++i) {
			args.emplace_back(argv[i]);
		}
		::LocalFree(argv);
		return args;
	}

	std::wstring QuoteArgument(const std::wstring& value) {
		std::wstring result = L"\"";
		for(wchar_t c : value) {
			if(c == L'"') {
				result += L"\\\"";
			} else {
				result += c;
			}
		}
		result += L"\"";
		return result;
	}

	bool HasFlag(const std::vector<std::wstring>& args, const std::wstring& flagName) {
		for(const auto& arg : args) {
			if(arg == flagName) {
				return true;
			}
		}
		return false;
	}

	std::filesystem::path DefaultEngineInstallRoot() {
		wchar_t* localAppData = nullptr;
		size_t length = 0;
		if(_wdupenv_s(&localAppData, &length, L"LOCALAPPDATA") == 0 && localAppData) {
			std::filesystem::path path = std::filesystem::path(localAppData) / L"CalyxEngine" / L"Engines";
			std::free(localAppData);
			return path;
		}

		return std::filesystem::path(L"CalyxEngine") / L"Engines";
	}

	std::filesystem::path DefaultDownloadDirectory() {
		wchar_t* localAppData = nullptr;
		size_t length = 0;
		if(_wdupenv_s(&localAppData, &length, L"LOCALAPPDATA") == 0 && localAppData) {
			std::filesystem::path path = std::filesystem::path(localAppData) / L"CalyxEngine" / L"Downloads";
			std::free(localAppData);
			return path;
		}

		return std::filesystem::path(L"CalyxEngine") / L"Downloads";
	}

	bool ReadProjectInfo(const std::filesystem::path& projectFile, ProjectInfo& outProject) {
		std::ifstream file(projectFile);
		if(!file) {
			return false;
		}

		nlohmann::json root;
		try {
			file >> root;
		} catch(const nlohmann::json::exception&) {
			return false;
		}

		// The launcher is intentionally driven by .calyxproj only. This keeps the
		// game repository independent from the engine repository and lets the
		// project pin the exact engine package it needs.
		outProject.name = root.value("name", projectFile.stem().string());
		outProject.engineVersion = root.value("engineVersion", std::string{});
		return !outProject.engineVersion.empty();
	}

	bool IsSdkDirectory(const std::filesystem::path& path) {
		// These files are the minimum SDK surface a generated game project needs
		// before MSBuild can compile user code.
		return std::filesystem::exists(path / L"Include" / L"CalyxEngine" / L"Application.h") &&
			   std::filesystem::exists(path / L"Include" / L"Data" / L"Engine") &&
			   std::filesystem::exists(path / L"Include" / L"externals" / L"nlohmann" / L"json.hpp") &&
			   std::filesystem::exists(path / L"Lib");
	}

	bool IsEnginePackageDirectory(const std::filesystem::path& path) {
		// CalyxLauncher updates packages only. CalyxGame.exe is the game host that
		// developers run from the generated solution, while SDK is used by the
		// generated game DLL project.
		return std::filesystem::exists(path / L"CalyxGame.exe") && IsSdkDirectory(path / L"SDK");
	}

	std::filesystem::path ResolveEnginePackageDirectory(const std::string& engineVersion) {
		if(engineVersion.empty()) {
			return {};
		}

		const auto cachedPackage = DefaultEngineInstallRoot() / std::filesystem::path(engineVersion);
		if(IsEnginePackageDirectory(cachedPackage)) {
			return cachedPackage;
		}

		// CALYX_ENGINE_SDK_DIR remains a developer override for local package
		// testing. It may point either at SDK itself or at the package root.
		wchar_t* sdkDir = nullptr;
		size_t length = 0;
		if(_wdupenv_s(&sdkDir, &length, L"CALYX_ENGINE_SDK_DIR") == 0 && sdkDir) {
			std::filesystem::path path = sdkDir;
			std::free(sdkDir);
			if(IsEnginePackageDirectory(path)) {
				return path;
			}
			if(IsEnginePackageDirectory(path.parent_path())) {
				return path.parent_path();
			}
		}

		return {};
	}

	int RunProcessAndWait(const std::wstring& commandLine) {
		std::wstring mutableCommandLine = commandLine;

		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		PROCESS_INFORMATION processInfo{};

		if(!::CreateProcessW(nullptr, mutableCommandLine.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startupInfo, &processInfo)) {
			return static_cast<int>(::GetLastError());
		}

		::WaitForSingleObject(processInfo.hProcess, INFINITE);

		DWORD exitCode = 1;
		::GetExitCodeProcess(processInfo.hProcess, &exitCode);
		::CloseHandle(processInfo.hThread);
		::CloseHandle(processInfo.hProcess);
		return static_cast<int>(exitCode);
	}

	bool WriteDownloadScript(const std::filesystem::path& scriptPath) {
		std::ofstream stream(scriptPath, std::ios::binary);
		if(!stream) {
			return false;
		}

		stream << R"ps1(param(
	[Parameter(Mandatory = $true)]
	[string]$VersionValue,
	[Parameter(Mandatory = $true)]
	[string]$InstallRoot
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

# The release is expected at:
# https://github.com/kase-haruto/CalyxEngine/releases/tag/<engineVersion>
# If CALYX_ENGINE_GITHUB_TOKEN is present, private release assets can also be used.
$headers = @{ 'User-Agent' = 'CalyxLauncher' }
if ($env:CALYX_ENGINE_GITHUB_TOKEN) {
	$headers['Authorization'] = "Bearer $env:CALYX_ENGINE_GITHUB_TOKEN"
}

$tags = @($VersionValue)
if ($VersionValue.StartsWith('v')) {
	$tags += $VersionValue.Substring(1)
} else {
	$tags += "v$VersionValue"
}

$release = $null
$releaseErrors = @()
foreach ($tag in $tags | Select-Object -Unique) {
	try {
		$release = Invoke-RestMethod -Headers $headers -Uri "https://api.github.com/repos/kase-haruto/CalyxEngine/releases/tags/$tag"
		break
	} catch {
		$statusCode = $null
		if ($null -ne $_.Exception.Response) {
			$statusCode = [int]$_.Exception.Response.StatusCode
		}

		$releaseErrors += "tag '$tag': $($_.Exception.Message)"
		if ($statusCode -ne 404) {
			throw "GitHub Releases API request failed while looking for CalyxEngine $VersionValue ($($releaseErrors -join '; ')). If this is an API rate limit or the release is private, set CALYX_ENGINE_GITHUB_TOKEN and retry."
		}
	}
}

if ($null -eq $release) {
	throw "CalyxEngine GitHub Release was not found for engineVersion: $VersionValue. Checked tags: $($tags -join ', '). A git tag alone is not enough; create a GitHub Release for this tag and attach the CalyxGamePackage zip asset."
}

# Prefer package-like zip names, but keep the rule flexible so release assets can
# be named CalyxGamePackage-<version>.zip, CalyxSDK-<version>.zip, etc.
# Do not stop at the first zip. A GitHub Release can contain source archives or
# other support zips, so each candidate is downloaded and validated before it is
# accepted as the engine package used by generated game projects.
$assets = @($release.assets |
	Where-Object { $_.name -match '\.zip$' } |
	Sort-Object @{
		Expression = {
			$score = 0
			if ($_.name -match 'Calyx') { $score -= 10 }
			if ($_.name -match 'Game|Package|SDK|Runtime|Engine') { $score -= 5 }
			if ($_.name -match 'Source|src') { $score += 20 }
			if ($_.name -match [regex]::Escape($VersionValue)) { $score -= 2 }
			$score
		}
	}, name)

if ($assets.Count -eq 0) {
	throw "No zip asset was found in release: $($release.tag_name)"
}

function Download-ReleaseAsset {
	param(
		[Parameter(Mandatory = $true)]
		$Asset,
		[Parameter(Mandatory = $true)]
		[string]$DestinationPath
	)

	if (Test-Path $DestinationPath) {
		Remove-Item -LiteralPath $DestinationPath -Force
	}

	Invoke-WebRequest -UseBasicParsing -Headers $headers -Uri $Asset.browser_download_url -OutFile $DestinationPath -TimeoutSec 600

	$file = Get-Item -LiteralPath $DestinationPath
	if ($null -ne $Asset.size -and [int64]$Asset.size -gt 0 -and $file.Length -ne [int64]$Asset.size) {
		throw "Downloaded asset size mismatch for '$($Asset.name)'. Expected $($Asset.size) bytes, got $($file.Length) bytes."
	}
}

$versionDir = Join-Path $InstallRoot $VersionValue
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("CalyxEngine-" + [System.Guid]::NewGuid().ToString("N"))

try {
	Write-Host "CalyxLauncher: release '$($release.tag_name)' was found."
	Add-Type -AssemblyName System.IO.Compression.FileSystem

	$package = $null
	foreach ($asset in $assets) {
		# Use a separate extraction directory per asset so a failed candidate
		# cannot leave files that make the next candidate look valid.
		$assetRoot = Join-Path $tempRoot ([System.IO.Path]::GetFileNameWithoutExtension($asset.name))
		$extractDir = Join-Path $assetRoot "extract"
		New-Item -ItemType Directory -Force -Path $extractDir | Out-Null

		$zipPath = Join-Path $assetRoot $asset.name
		Write-Host "CalyxLauncher: downloading '$($asset.name)'..."
		Download-ReleaseAsset -Asset $asset -DestinationPath $zipPath
		Write-Host "CalyxLauncher: download completed: $zipPath"

		Write-Host "CalyxLauncher: extracting package..."
		[System.IO.Compression.ZipFile]::ExtractToDirectory($zipPath, $extractDir)
		Write-Host "CalyxLauncher: extraction completed: $extractDir"

		# A valid game-development package must contain the runtime host exe and
		# the SDK headers/libs that the generated game DLL links against.
		$candidates = @((Get-Item $extractDir)) + @(Get-ChildItem $extractDir -Directory -Recurse)
		$package = $candidates |
			Where-Object {
				(Test-Path (Join-Path $_.FullName 'CalyxGame.exe')) -and
				(Test-Path (Join-Path $_.FullName 'SDK\Include\CalyxEngine\Application.h')) -and
				(Test-Path (Join-Path $_.FullName 'SDK\Lib'))
			} |
			Select-Object -First 1

		if ($null -ne $package) {
			Write-Host "CalyxLauncher: valid engine package found in '$($asset.name)'."
			break
		}

		Write-Host "CalyxLauncher: '$($asset.name)' is not a game runtime SDK package. Trying next zip asset."
	}

	if ($null -eq $package) {
		throw "No valid CalyxEngine game runtime SDK package was found in release '$($release.tag_name)'. The package must contain CalyxGame.exe, SDK\Include\CalyxEngine\Application.h, and SDK\Lib."
	}

	New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
	if (Test-Path $versionDir) {
		Write-Host "CalyxLauncher: removing existing package: $versionDir"
		Remove-Item -LiteralPath $versionDir -Recurse -Force
	}
	Write-Host "CalyxLauncher: installing package to: $versionDir"
	Move-Item -LiteralPath $package.FullName -Destination $versionDir
	Write-Host "CalyxLauncher: package installation completed."
} finally {
	if (Test-Path $tempRoot) {
		Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
	}
}
)ps1";
		return true;
	}

	bool DownloadEnginePackageFromGitHub(const std::string& engineVersion) {
		const auto downloadDirectory = DefaultDownloadDirectory();
		std::error_code ec;
		std::filesystem::create_directories(downloadDirectory, ec);
		if(ec) {
			std::wcerr << L"Failed to create download directory: " << downloadDirectory.wstring() << L"\n";
			return false;
		}

		const std::wstring scriptName = std::wstring(engineVersion.begin(), engineVersion.end()) + L"-install.ps1";
		const auto scriptPath = downloadDirectory / scriptName;
		if(!WriteDownloadScript(scriptPath)) {
			std::wcerr << L"Failed to write download script: " << scriptPath.wstring() << L"\n";
			return false;
		}

		const auto installRoot = DefaultEngineInstallRoot();
		const std::wstring version(engineVersion.begin(), engineVersion.end());
		std::wstring commandLine = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File ";
		commandLine += QuoteArgument(scriptPath.wstring());
		commandLine += L" -VersionValue ";
		commandLine += QuoteArgument(version);
		commandLine += L" -InstallRoot ";
		commandLine += QuoteArgument(installRoot.wstring());

		std::wcerr << L"Installing CalyxEngine " << version << L" from GitHub Releases...\n";
		const int exitCode = RunProcessAndWait(commandLine);
		if(exitCode != 0) {
			std::wcerr << L"CalyxEngine package installation failed. PowerShell exit code: " << exitCode << L"\n";
			return false;
		}

		return true;
	}

} // namespace

int wmain() {
	const auto args = SplitCommandLineArguments();
	if(args.empty()) {
		std::wcerr << L"Usage: CalyxLauncher.exe <project.calyxproj> [--force]\n";
		return 1;
	}

	const std::filesystem::path projectFile = args.front();
	const bool forceUpdate = HasFlag(args, L"--force") || HasFlag(args, L"--force-update");

	ProjectInfo project;
	if(!ReadProjectInfo(projectFile, project)) {
		std::wcerr << L"Failed to read project file or engineVersion: " << projectFile.wstring() << L"\n";
		return 2;
	}

	auto enginePackageDirectory = ResolveEnginePackageDirectory(project.engineVersion);
	if(enginePackageDirectory.empty() || forceUpdate) {
		if(!DownloadEnginePackageFromGitHub(project.engineVersion)) {
			return 3;
		}
		enginePackageDirectory = ResolveEnginePackageDirectory(project.engineVersion);
	}

	if(enginePackageDirectory.empty()) {
		std::wcerr << L"CalyxEngine package is invalid after installation: "
				   << (DefaultEngineInstallRoot() / std::filesystem::path(project.engineVersion)).wstring() << L"\n";
		return 4;
	}

	std::wcout << L"CalyxEngine package is ready.\n";
	std::wcout << L"Package: " << enginePackageDirectory.wstring() << L"\n";
	std::wcout << L"SDK: " << (enginePackageDirectory / L"SDK").wstring() << L"\n";
	return 0;
}
