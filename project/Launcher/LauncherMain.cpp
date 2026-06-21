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
		std::filesystem::path directory;
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

	std::filesystem::path TemporaryScriptPath() {
		wchar_t tempPath[MAX_PATH]{};
		if(::GetTempPathW(MAX_PATH, tempPath) == 0) {
			return std::filesystem::temp_directory_path() / L"CalyxEngineDownload.ps1";
		}

		std::wstringstream name;
		name << L"CalyxEngineDownload-" << ::GetCurrentProcessId() << L".ps1";
		return std::filesystem::path(tempPath) / name.str();
	}

	std::filesystem::path ReadProjectInfo(const std::filesystem::path& projectFile, ProjectInfo& outProject) {
		std::ifstream file(projectFile);
		if(!file) {
			return {};
		}

		nlohmann::json root;
		try {
			file >> root;
		} catch(const nlohmann::json::exception&) {
			return {};
		}

		// .calyxproj に保存された engineVersion を使用する。
		// チームではリードがこの値を更新することで、全員が同じ Editor/SDK を使う。
		outProject.directory = projectFile.parent_path();
		outProject.name = root.value("name", projectFile.stem().string());
		outProject.engineVersion = root.value("engineVersion", std::string{});
		return outProject.directory;
	}

	bool IsSdkDirectory(const std::filesystem::path& path) {
		return std::filesystem::exists(path / L"Include" / L"CalyxEngine" / L"Application.h") &&
			   std::filesystem::exists(path / L"Include" / L"Data" / L"Engine") &&
			   std::filesystem::exists(path / L"Include" / L"externals" / L"nlohmann" / L"json.hpp");
	}

	bool IsEnginePackageDirectory(const std::filesystem::path& path) {
		return std::filesystem::exists(path / L"CalyxEditor.exe") && IsSdkDirectory(path / L"SDK");
	}

	std::filesystem::path ResolveEnginePackageDirectory(const std::string& engineVersion) {
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

		// 環境変数が未設定の場合は、ユーザーごとのエンジンキャッシュから version 固定で探す。
		// 例: %LOCALAPPDATA%\CalyxEngine\Engines\v1.0.1\CalyxEditor.exe
		if(!engineVersion.empty()) {
			const auto cachedPackage = DefaultEngineInstallRoot() / std::filesystem::path(engineVersion);
			if(IsEnginePackageDirectory(cachedPackage)) {
				return cachedPackage;
			}
		}

		return {};
	}

	bool SetSdkDirectoryForChildProcesses(const std::filesystem::path& enginePackageDirectory) {
		const auto sdkDirectory = enginePackageDirectory / L"SDK";
		if(!IsSdkDirectory(sdkDirectory)) {
			return false;
		}

		return ::SetEnvironmentVariableW(L"CALYX_ENGINE_SDK_DIR", sdkDirectory.c_str()) != FALSE;
	}

	int RunProcessAndWait(const std::wstring& commandLine, const std::filesystem::path& workingDirectory) {
		std::wstring mutableCommandLine = commandLine;

		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		PROCESS_INFORMATION processInfo{};

		if(!::CreateProcessW(
			   nullptr,
			   mutableCommandLine.data(),
			   nullptr,
			   nullptr,
			   TRUE,
			   0,
			   nullptr,
			   workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
			   &startupInfo,
			   &processInfo)) {
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
	[string]$VersionValue,
	[string]$InstallRoot
)

$ErrorActionPreference = 'Stop'
$headers = @{ 'User-Agent' = 'CalyxLauncher' }
$tags = @($VersionValue)
if (-not $VersionValue.StartsWith('v')) {
	$tags += "v$VersionValue"
}

$release = $null
foreach ($tag in $tags) {
	try {
		$release = Invoke-RestMethod -Headers $headers -Uri "https://api.github.com/repos/kase-haruto/CalyxEngine/releases/tags/$tag"
		break
	} catch {
	}
}

if ($null -eq $release) {
	throw "CalyxEngine release was not found for engineVersion: $VersionValue"
}

$asset = $release.assets | Where-Object { $_.name -match '\.zip$' } | Select-Object -First 1
if ($null -eq $asset) {
	throw "No zip asset was found in release: $($release.tag_name)"
}

$versionDir = Join-Path $InstallRoot $VersionValue
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("CalyxEngine-" + [System.Guid]::NewGuid().ToString("N"))
$extractDir = Join-Path $tempRoot "extract"
New-Item -ItemType Directory -Force -Path $extractDir | Out-Null

try {
	$zipPath = Join-Path $tempRoot $asset.name
	Invoke-WebRequest -Headers $headers -Uri $asset.browser_download_url -OutFile $zipPath
	Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force

	$candidates = @((Get-Item $extractDir)) + @(Get-ChildItem $extractDir -Directory -Recurse)
	$package = $candidates |
		Where-Object {
			(Test-Path (Join-Path $_.FullName 'CalyxEditor.exe')) -and
			(Test-Path (Join-Path $_.FullName 'SDK\Include\CalyxEngine\Application.h'))
		} |
		Select-Object -First 1

	if ($null -eq $package) {
		throw "Downloaded package does not contain CalyxEditor.exe and SDK\Include\CalyxEngine\Application.h"
	}

	New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
	if (Test-Path $versionDir) {
		Remove-Item -LiteralPath $versionDir -Recurse -Force
	}
	Move-Item -LiteralPath $package.FullName -Destination $versionDir
} finally {
	if (Test-Path $tempRoot) {
		Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
	}
}
)ps1";
		return true;
	}

	bool DownloadEnginePackageFromGitHub(const std::string& engineVersion) {
		if(engineVersion.empty()) {
			return false;
		}

		const auto scriptPath = TemporaryScriptPath();
		if(!WriteDownloadScript(scriptPath)) {
			std::wcerr << L"Failed to write temporary download script: " << scriptPath.wstring() << L"\n";
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

		std::wcerr << L"Calyx SDK is not installed. Downloading engineVersion " << version << L" from GitHub Releases...\n";
		const int exitCode = RunProcessAndWait(commandLine, {});
		std::error_code ec;
		std::filesystem::remove(scriptPath, ec);

		if(exitCode != 0) {
			std::wcerr << L"Failed to download Calyx SDK. PowerShell exit code: " << exitCode << L"\n";
			return false;
		}

		return true;
	}

	std::wstring GetOptionValue(const std::vector<std::wstring>& args, const std::wstring& name, const std::wstring& fallback) {
		for(size_t i = 0; i + 1 < args.size(); ++i) {
			if(args[i] == name) {
				return args[i + 1];
			}
		}
		return fallback;
	}

	bool HasTarget(const std::vector<std::wstring>& args, const std::wstring& targetName) {
		for(size_t i = 0; i + 1 < args.size(); ++i) {
			if(args[i] == L"--target" && args[i + 1] == targetName) {
				return true;
			}
		}
		return false;
	}

	bool HasFlag(const std::vector<std::wstring>& args, const std::wstring& flagName) {
		for(const auto& arg : args) {
			if(arg == flagName) {
				return true;
			}
		}
		return false;
	}

	std::filesystem::path FindMSBuild() {
		const std::filesystem::path candidates[] = {
			L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe",
			L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe",
			L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe",
			L"C:\\Program Files\\Microsoft Visual Studio\\2022\\BuildTools\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe",
		};

		for(const auto& candidate : candidates) {
			if(std::filesystem::exists(candidate)) {
				return candidate;
			}
		}

		return L"MSBuild.exe";
	}

	int BuildGameProject(const ProjectInfo& project, const std::wstring& config) {
		const std::filesystem::path solutionPath = project.directory / (project.name + ".sln");
		if(!std::filesystem::exists(solutionPath)) {
			std::wcerr << L"Game solution was not found: " << solutionPath.wstring() << L"\n";
			return 5;
		}

		std::wstring commandLine = QuoteArgument(FindMSBuild().wstring());
		commandLine += L" ";
		commandLine += QuoteArgument(solutionPath.wstring());
		commandLine += L" /p:Configuration=";
		commandLine += QuoteArgument(config);
		commandLine += L" /p:Platform=x64 /m /v:minimal";

		return RunProcessAndWait(commandLine, project.directory);
	}

	int LaunchEditor(const std::filesystem::path& editorExe, const std::vector<std::wstring>& forwardedArgs);

	int LaunchGameExecutable(const ProjectInfo& project, const std::wstring& config) {
		const std::wstring projectName(project.name.begin(), project.name.end());
		const std::filesystem::path gameExe = project.directory / L"Generated" / L"Outputs" / config / (projectName + L".exe");
		if(!std::filesystem::exists(gameExe)) {
			std::wcerr << L"Game executable was not found: " << gameExe.wstring() << L"\n";
			return 6;
		}

		std::vector<std::wstring> gameArgs;
		gameArgs.push_back((project.directory / (projectName + L".calyxproj")).wstring());
		gameArgs.push_back(L"--config");
		gameArgs.push_back(config);
		return LaunchEditor(gameExe, gameArgs);
	}

	int LaunchEditor(const std::filesystem::path& editorExe, const std::vector<std::wstring>& forwardedArgs) {
		std::wstring commandLine = QuoteArgument(editorExe.wstring());
		for(const auto& arg : forwardedArgs) {
			commandLine += L" ";
			commandLine += QuoteArgument(arg);
		}

		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		PROCESS_INFORMATION processInfo{};

		// Launcher は Editor を子プロセスとして起動する。
		// Visual Studio から Launcher を F5 実行した場合でも、Editor プロセスへデバッグが継続される。
		const std::wstring workingDirectory = editorExe.parent_path().wstring();
		if(!::CreateProcessW(
			   editorExe.c_str(),
			   commandLine.data(),
			   nullptr,
			   nullptr,
			   FALSE,
			   0,
			   nullptr,
			   workingDirectory.c_str(),
			   &startupInfo,
			   &processInfo)) {
			return static_cast<int>(::GetLastError());
		}

		::CloseHandle(processInfo.hThread);
		::CloseHandle(processInfo.hProcess);
		return 0;
	}

} // namespace

int wmain() {
	const auto args = SplitCommandLineArguments();
	if(args.empty()) {
		std::wcerr << L"Usage: CalyxLauncher.exe <project.calyxproj> [--config Debug|Develop|Release]\n";
		return 1;
	}

	const std::filesystem::path projectFile = args.front();
	ProjectInfo project;
	if(ReadProjectInfo(projectFile, project).empty()) {
		std::wcerr << L"Failed to read project file: " << projectFile.wstring() << L"\n";
		return 2;
	}

	auto enginePackageDirectory = ResolveEnginePackageDirectory(project.engineVersion);
	if(enginePackageDirectory.empty() && DownloadEnginePackageFromGitHub(project.engineVersion)) {
		enginePackageDirectory = ResolveEnginePackageDirectory(project.engineVersion);
	}

	if(enginePackageDirectory.empty()) {
		std::wcerr << L"Calyx SDK was not found for engineVersion: " << std::wstring(project.engineVersion.begin(), project.engineVersion.end()) << L"\n";
		std::wcerr << L"Set CALYX_ENGINE_SDK_DIR, install the SDK under %LOCALAPPDATA%\\CalyxEngine\\Engines\\<version>, or publish a matching GitHub Release zip.\n";
		return 3;
	}

	if(!SetSdkDirectoryForChildProcesses(enginePackageDirectory)) {
		std::wcerr << L"Calyx SDK is invalid: " << (enginePackageDirectory / L"SDK").wstring() << L"\n";
		return 4;
	}

	if(HasTarget(args, L"game")) {
		const std::wstring config = GetOptionValue(args, L"--config", L"Debug");
		if(!HasFlag(args, L"--skip-build")) {
			const int buildExitCode = BuildGameProject(project, config);
			if(buildExitCode != 0) {
				std::wcerr << L"Game build failed. MSBuild exit code: " << buildExitCode << L"\n";
				return buildExitCode;
			}
		}
		return LaunchGameExecutable(project, config);
	}

	return LaunchEditor(enginePackageDirectory / L"CalyxEditor.exe", args);
}
