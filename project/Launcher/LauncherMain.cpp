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

# VersionValue comes directly from the opened .calyxproj. ReleasePackage.yml
# publishes CalyxGamePackage-<tag>.zip, so the asset URL is deterministic and
# does not require a GitHub REST API request.
$headers = @{ 'User-Agent' = 'CalyxLauncher' }
if ($env:CALYX_ENGINE_GITHUB_TOKEN) {
	$headers['Authorization'] = "Bearer $env:CALYX_ENGINE_GITHUB_TOKEN"
}

$releaseTag = if ($VersionValue.StartsWith('v')) { $VersionValue } else { "v$VersionValue" }
$assetName = "CalyxGamePackage-$releaseTag.zip"
$downloadUrl = "https://github.com/kase-haruto/CalyxEngine/releases/download/$releaseTag/$assetName"

$versionDir = Join-Path $InstallRoot $VersionValue
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("CalyxEngine-" + [System.Guid]::NewGuid().ToString("N"))

try {
	Write-Host "CalyxLauncher: installing the version requested by .calyxproj: '$VersionValue'."
	Add-Type -AssemblyName System.IO.Compression.FileSystem

	$extractDir = Join-Path $tempRoot "extract"
	New-Item -ItemType Directory -Force -Path $extractDir | Out-Null
	$zipPath = Join-Path $tempRoot $assetName
	Write-Host "CalyxLauncher: downloading '$downloadUrl'..."
	Invoke-WebRequest -UseBasicParsing -Headers $headers -Uri $downloadUrl -OutFile $zipPath -TimeoutSec 600
	[System.IO.Compression.ZipFile]::ExtractToDirectory($zipPath, $extractDir)

	$candidates = @((Get-Item $extractDir)) + @(Get-ChildItem $extractDir -Directory -Recurse)
	$package = $candidates |
		Where-Object {
			(Test-Path (Join-Path $_.FullName 'CalyxGame.exe')) -and
			(Test-Path (Join-Path $_.FullName 'SDK\Include\CalyxEngine\Application.h')) -and
			(Test-Path (Join-Path $_.FullName 'SDK\Lib'))
		} |
		Select-Object -First 1

	if ($null -eq $package) {
		throw "CalyxEngine package '$assetName' is invalid. It must contain CalyxGame.exe, SDK\Include\CalyxEngine\Application.h, and SDK\Lib."
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
