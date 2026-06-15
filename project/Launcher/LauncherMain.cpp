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

	std::filesystem::path ReadProjectEngineVersion(const std::filesystem::path& projectFile, std::string& outVersion) {
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
		outVersion = root.value("engineVersion", std::string{});
		return projectFile.parent_path();
	}

	std::filesystem::path ResolveSdkDirectory(const std::string& engineVersion) {
		wchar_t* sdkDir = nullptr;
		size_t length = 0;
		if(_wdupenv_s(&sdkDir, &length, L"CALYX_ENGINE_SDK_DIR") == 0 && sdkDir) {
			std::filesystem::path path = sdkDir;
			std::free(sdkDir);
			if(std::filesystem::exists(path / L"CalyxEditor.exe")) {
				return path;
			}
			if(std::filesystem::exists(path.parent_path() / L"CalyxEditor.exe")) {
				return path.parent_path();
			}
		}

		// 環境変数が未設定の場合は、ユーザーごとのエンジンキャッシュから version 固定で探す。
		// 例: %LOCALAPPDATA%\CalyxEngine\Engines\v1.0.1\CalyxEditor.exe
		if(!engineVersion.empty()) {
			const auto cachedPackage = DefaultEngineInstallRoot() / std::filesystem::path(engineVersion);
			if(std::filesystem::exists(cachedPackage / L"CalyxEditor.exe")) {
				return cachedPackage;
			}
		}

		return {};
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
	std::string engineVersion;
	if(ReadProjectEngineVersion(projectFile, engineVersion).empty()) {
		std::wcerr << L"Failed to read project file: " << projectFile.wstring() << L"\n";
		return 2;
	}

	const auto sdkDirectory = ResolveSdkDirectory(engineVersion);
	if(sdkDirectory.empty()) {
		std::wcerr << L"Calyx SDK was not found for engineVersion: " << std::wstring(engineVersion.begin(), engineVersion.end()) << L"\n";
		std::wcerr << L"Set CALYX_ENGINE_SDK_DIR or install the SDK under %LOCALAPPDATA%\\CalyxEngine\\Engines\\<version>\\SDK.\n";
		return 3;
	}

	return LaunchEditor(sdkDirectory / L"CalyxEditor.exe", args);
}
