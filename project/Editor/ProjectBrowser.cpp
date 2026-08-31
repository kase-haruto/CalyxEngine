#include "ProjectBrowser.h"

#include <Engine/Assets/Manager/AssetManager.h>

#include <externals/imgui/ImGuiFileDialog.h>
#include <externals/imgui/imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <random>
#include <span>
#include <sstream>
#include <vector>

namespace CalyxEditor {

	namespace {

		// 定義されているプロジェクトテンプレートの一覧
		const ProjectBrowser::TemplateInfo kTemplates[] = {
			{ProjectTemplateType::Blank, "Blank", "空のプロジェクト", "Game", ""},
			{ProjectTemplateType::Demo, "Demo", "デモプロジェクト", "Game", "Resources/Assets/Scenes/DemoScene.scene"},
		};
		constexpr const char* kDefaultEngineVersion = "v1.2.21";

		// 新規プロジェクトの初期作成先のディレクトリパスを取得（ユーザーのドキュメントフォルダを指す）
		std::filesystem::path DefaultUserProjectDirectory() {
			char*  userProfile = nullptr;
			size_t length		= 0;
			// WindowsのUSERPROFILE環境変数からパスを取得
			if(_dupenv_s(&userProfile, &length, "USERPROFILE") == 0 && userProfile) {
				std::filesystem::path path = std::filesystem::path(userProfile) / "Documents" / "Calyx Projects";
				std::free(userProfile);
				return path;
			}
			// 取得に失敗した場合は相対パスのフォルダ名を返す
			return std::filesystem::path("Calyx Projects");
		}

		// std::array<char> 等の固定長バッファへの安全な文字列コピーを行うヘルパー
		bool CopyText(std::span<char> buffer, const std::string& text) {
			if(buffer.empty()) return false;
			// バッファ終端用のヌル文字スペースを除いた最大長でコピー
			const size_t length = (std::min)(buffer.size() - 1, text.size());
			std::copy_n(text.data(), length, buffer.data());
			buffer[length] = '\0'; // 確実にヌル終端する
			return length == text.size();
		}

		// 入力欄の未入力判定
		bool IsBlank(const char* text) {
			if(!text) return true;
			while(*text) {
				if(!std::isspace(static_cast<unsigned char>(*text))) return false;
				++text;
			}
			return true;
		}

		// Visual Studio用GUID（UUID v4に準拠）を生成するヘルパー関数
		std::string MakeGuid() {
			std::random_device randomDevice;
			std::mt19937 generator(randomDevice());
			std::uniform_int_distribution<unsigned int> dist(0, 15);
			std::uniform_int_distribution<unsigned int> variantDist(8, 11);

			// GUID形式: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX のフォーマットで書き出し
			std::stringstream stream;
			stream << std::uppercase << std::hex;
			for(int i = 0; i < 8; ++i) {
				stream << dist(generator);
			}
			stream << "-";
			for(int i = 0; i < 4; ++i) {
				stream << dist(generator);
			}
			stream << "-";
			stream << "4"; // バージョン4フラグ
			for(int i = 0; i < 3; ++i) {
				stream << dist(generator);
			}
			stream << "-";
			stream << variantDist(generator); // バリアントビット
			for(int i = 0; i < 3; ++i) {
				stream << dist(generator);
			}
			stream << "-";
			for(int i = 0; i < 12; ++i) {
				stream << dist(generator);
			}
			return stream.str();
		}

		// vcxproj等のXMLタグ属性へ書き込める安全なエスケープ文字列に変換
		std::string EscapeXml(const std::string& text) {
			std::string result;
			result.reserve(text.size());
			for(char c : text) {
				switch(c) {
				case '&': result += "&amp;"; break;
				case '<': result += "&lt;"; break;
				case '>': result += "&gt;"; break;
				case '"': result += "&quot;"; break;
				default: result += c; break;
				}
			}
			return result;
		}

		// ソリューションやプロジェクトファイルをディスクへ書き出すヘルパー
		bool WriteTextFile(const std::filesystem::path& path, const std::string& text) {
			std::error_code ec;
			// 保存先ディレクトリが存在しない場合は親階層を含め自動作成
			std::filesystem::create_directories(path.parent_path(), ec);
			if(ec) return false;

			std::ofstream file(path);
			if(!file) return false;
			file << text;
			return true;
		}

		// ディレクトリ以下のファイルを再帰的にまとめてコピー（フォルダ構造維持）
		bool CopyDirectoryTree(const std::filesystem::path& source, const std::filesystem::path& destination) {
			if(!std::filesystem::exists(source)) {
				return false;
			}

			std::error_code ec;
			std::filesystem::create_directories(destination, ec);
			if(ec) return false;

			// ソースディレクトリ以下のファイルを再帰的に巡回コピー
			for(const auto& entry : std::filesystem::recursive_directory_iterator(source, ec)) {
				if(ec) return false;

				// 相対パスを求めて出力先パスを構成
				const auto relativePath = std::filesystem::relative(entry.path(), source, ec);
				if(ec) return false;

				const auto outputPath = destination / relativePath;
				if(entry.is_directory()) {
					std::filesystem::create_directories(outputPath, ec);
					if(ec) return false;
					continue;
				}

				std::filesystem::create_directories(outputPath.parent_path(), ec);
				if(ec) return false;

				// ファイルを上書きモードでコピー
				std::filesystem::copy_file(entry.path(), outputPath, std::filesystem::copy_options::overwrite_existing, ec);
				if(ec) return false;
			}

			return true;
		}

		// 現在のアプリケーション実行位置から上に向かって探索し、配布済みエンジンルートを発見する
		std::filesystem::path FindInstalledEngineDirectory() {
			std::error_code ec;
			std::filesystem::path path = std::filesystem::weakly_canonical(std::filesystem::current_path(), ec);
			if(ec) {
				path = std::filesystem::current_path();
			}

			// 親ディレクトリへ順に遡り、配布物に含まれるResourcesの存在を確認
			while(!path.empty()) {
				if(std::filesystem::exists(path / "Resources")) {
					return path;
				}
				if(std::filesystem::exists(path / "project" / "Resources")) {
					return path / "project";
				}

				const auto parent = path.parent_path();
				if(parent == path) break;
				path = parent;
			}

			return {};
		}

		std::filesystem::path FindLauncherExecutable(const std::filesystem::path& engineDirectory) {
			// Project Browser is engine-side, but generated game projects need a
			// project-local updater. Search both packaged and local build output
			// locations without requiring the game developer to have the engine repo.
			const std::filesystem::path candidates[] = {
				engineDirectory / "CalyxLauncher.exe",
				engineDirectory.parent_path() / "generated" / "outputs" / "Develop" / "CalyxLauncher.exe",
				engineDirectory.parent_path() / "generated" / "outputs" / "Debug" / "CalyxLauncher.exe",
				engineDirectory.parent_path() / "generated" / "outputs" / "Release" / "CalyxLauncher.exe",
			};
			for(const auto& candidate : candidates) {
				if(std::filesystem::exists(candidate)) {
					return candidate;
				}
			}
			return {};
		}

		std::filesystem::path FindDemoSourceDirectory(const std::filesystem::path& engineDirectory) {
			const auto gameDemoDirectory = engineDirectory / "Game" / "Demo";
			if(std::filesystem::exists(gameDemoDirectory)) {
				return gameDemoDirectory;
			}

			const auto legacyDemoDirectory = engineDirectory / "Demo";
			if(std::filesystem::exists(legacyDemoDirectory)) {
				return legacyDemoDirectory;
			}

			return {};
		}

		// 基準フォルダ(from)から対象ファイル(to)への相対パスを取得し、Visual Studio形式（ジェネリック表記スラッシュ）に変換
		std::string ToVisualStudioPath(const std::filesystem::path& from, const std::filesystem::path& to) {
			std::error_code ec;
			auto relative = std::filesystem::relative(to, from, ec);
			if(ec) {
				relative = to;
			}
			return relative.generic_string();
		}

		// Source以下のcppをvcxprojへ登録するために集める
		std::vector<std::filesystem::path> CollectSourceFiles(const Calyx::ProjectInfo& project) {
			std::vector<std::filesystem::path> sourceFiles;
			const auto sourceDirectory = Calyx::ResolveProjectPath(project, project.sourceDirectory);
			if(!std::filesystem::exists(sourceDirectory)) {
				return sourceFiles;
			}

			std::error_code ec;
			for(const auto& entry : std::filesystem::recursive_directory_iterator(sourceDirectory, ec)) {
				if(ec) break;
				if(entry.is_regular_file() && entry.path().extension() == ".cpp") {
					sourceFiles.push_back(entry.path());
				}
			}

			std::sort(sourceFiles.begin(), sourceFiles.end());
			return sourceFiles;
		}

		std::string MakeGameApplicationHeader() {
			std::stringstream stream;
			stream << "#pragma once\n\n";
			stream << "#include <CalyxEngine/Application.h>\n\n";
			stream << "class GameApplication : public Calyx::Application {\n";
			stream << "public:\n";
			stream << "\tvoid OnInitialize() override;\n";
			stream << "\tvoid OnUpdate() override;\n";
			stream << "\tvoid OnRender() override;\n";
			stream << "\tvoid OnFinalize() override;\n";
			stream << "};\n";
			return stream.str();
		}

		std::string MakeGameApplicationSource(const ProjectBrowser::TemplateInfo&) {
			std::stringstream stream;
			stream << "#include \"GameApplication.h\"\n\n";
			stream << "void GameApplication::OnInitialize() {}\n\n";
			stream << "void GameApplication::OnUpdate() {}\n\n";
			stream << "void GameApplication::OnRender() {}\n\n";
			stream << "void GameApplication::OnFinalize() {}\n";
			return stream.str();
		}

		// 生成されたゲームが最初に持つエントリポイント
		std::string MakeGameMainSource() {
			std::stringstream stream;
			stream << "#include <CalyxEngine/CalyxEngine.h>\n";
			stream << "\n";
			stream << "#include <Generated/Foundation/Reflection/CalyxGameObjectRegistry.generated.h>\n";
			stream << "#include \"GameApplication.h\"\n\n";
			stream << "extern \"C\" __declspec(dllexport) Calyx::Application* CreateCalyxApplication() {\n";
			stream << "\tCalyxEngine::RegisterGeneratedGameSceneObjects();\n";
			stream << "\n";
			stream << "\treturn new GameApplication();\n";
			stream << "}\n\n";
			stream << "extern \"C\" __declspec(dllexport) void DestroyCalyxApplication(Calyx::Application* application) {\n";
			stream << "\tdelete application;\n";
			stream << "}\n";
			return stream.str();
		}

		std::string MakeGameEditorExtensionSource() {
			std::stringstream stream;
			stream << "#include <CalyxEngine/EditorExtension.h>\n";
			stream << "#include <Engine/Graphics/Camera/Base/BaseCamera.h>\n";
			stream << "#include <externals/imgui/imgui.h>\n\n";
			stream << "#include <numbers>\n\n";
			stream << "namespace {\n";
			stream << "\tclass GameCameraEditor final : public CalyxEditor::IEditorTool {\n";
			stream << "\tpublic:\n";
			stream << "\t\texplicit GameCameraEditor(const CalyxEditor::EditorToolContext& context) : context_(context) {}\n";
			stream << "\t\tvoid OnOpen() override { open_ = true; SyncFromCamera(); }\n";
			stream << "\t\tvoid Draw() override {\n";
			stream << "\t\t\tif(!open_) return;\n";
			stream << "\t\t\tImGui::Begin(\"Game Camera Editor###Game.CameraEditor\", &open_);\n";
			stream << "\t\t\tauto* camera = context_.GetMainCamera();\n";
			stream << "\t\t\tif(!camera) { ImGui::TextDisabled(\"No main camera is available.\"); ImGui::End(); return; }\n";
			stream << "\t\t\tif(context_.IsPlaying()) ImGui::TextDisabled(\"Editing the runtime camera\");\n";
			stream << "\t\t\tbool changed = ImGui::DragFloat3(\"Position\", position_, 0.1f);\n";
			stream << "\t\t\tchanged |= ImGui::DragFloat3(\"Rotation\", rotation_, 0.01f);\n";
			stream << "\t\t\tchanged |= ImGui::DragFloat(\"Field of View\", &fieldOfView_, 0.1f, 1.0f, 179.0f);\n";
			stream << "\t\t\tif(changed) {\n";
			stream << "\t\t\t\tcamera->SetCamera({position_[0], position_[1], position_[2]}, {rotation_[0], rotation_[1], rotation_[2]});\n";
			stream << "\t\t\t\tcamera->SetFovY(fieldOfView_ * std::numbers::pi_v<float> / 180.0f);\n";
			stream << "\t\t\t\tcamera->UpdateMatrix();\n";
			stream << "\t\t\t}\n";
			stream << "\t\t\tif(!context_.IsPlaying() && ImGui::Button(\"Save Scene\")) context_.RequestSaveScene();\n";
			stream << "\t\t\tImGui::End();\n";
			stream << "\t\t}\n";
			stream << "\t\tbool IsOpen() const override { return open_; }\n";
			stream << "\tprivate:\n";
			stream << "\t\tvoid SyncFromCamera() {\n";
			stream << "\t\t\tif(auto* camera = context_.GetMainCamera()) {\n";
			stream << "\t\t\t\tconst auto& p = camera->GetTranslate(); const auto& r = camera->GetRotate();\n";
			stream << "\t\t\t\tposition_[0] = p.x; position_[1] = p.y; position_[2] = p.z;\n";
			stream << "\t\t\t\trotation_[0] = r.x; rotation_[1] = r.y; rotation_[2] = r.z;\n";
			stream << "\t\t\t\tfieldOfView_ = camera->GetFovY() * 180.0f / std::numbers::pi_v<float>;\n";
			stream << "\t\t\t}\n";
			stream << "\t\t}\n";
			stream << "\t\tCalyxEditor::EditorToolContext context_;\n";
			stream << "\t\tbool open_ = true;\n";
			stream << "\t\tfloat position_[3]{};\n";
			stream << "\t\tfloat rotation_[3]{};\n";
			stream << "\t\tfloat fieldOfView_ = 60.0f;\n";
			stream << "\t};\n\n";
			stream << "\tCalyxEditor::IEditorTool* CreateGameCameraEditor(const CalyxEditor::EditorToolContext& context) {\n";
			stream << "\t\treturn new GameCameraEditor(context);\n";
			stream << "\t}\n";
			stream << "\tvoid DestroyGameCameraEditor(CalyxEditor::IEditorTool* tool) { delete tool; }\n";
			stream << "}\n\n";
			stream << "extern \"C\" __declspec(dllexport) bool RegisterCalyxEditorTools(\n";
			stream << "\tstd::uint32_t apiVersion, CalyxEditor::IEditorHost* host) {\n";
			stream << "\tif(!host || apiVersion != CalyxEditor::kEditorToolApiVersion) return false;\n";
			stream << "\tCalyxEditor::EditorToolDescriptor descriptor;\n";
			stream << "\tdescriptor.id = \"Game.CameraEditor\";\n";
			stream << "\tdescriptor.displayName = \"Game Camera Editor\";\n";
			stream << "\tdescriptor.menuPath = \"Game/Camera\";\n";
			stream << "\tdescriptor.workspaceId = \"Game.Camera\";\n";
			stream << "\tdescriptor.layoutPath = \"GameCameraEditor.ini\";\n";
			stream << "\tdescriptor.create = &CreateGameCameraEditor;\n";
			stream << "\tdescriptor.destroy = &DestroyGameCameraEditor;\n";
			stream << "\treturn host->RegisterTool(descriptor);\n";
			stream << "}\n";
			return stream.str();
		}

		std::string MakeGameObjectRegistryHeader() {
			std::stringstream stream;
			stream << "#pragma once\n\n";
			stream << "namespace CalyxEngine {\n";
			stream << "\tvoid RegisterGeneratedGameSceneObjects();\n";
			stream << "}\n";
			return stream.str();
		}

		std::string MakeGameObjectRegistrySource() {
			std::stringstream stream;
			stream << "// This file is generated by Tools/Reflection/generate_reflection.ps1.\n";
			stream << "// Do not edit by hand.\n\n";
			stream << "#include <Game/GameObjectRegistry.generated.h>\n\n";
			stream << "#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>\n\n";
			stream << "namespace CalyxEngine {\n";
			stream << "\tvoid RegisterGeneratedGameSceneObjects() {\n";
			stream << "\t}\n";
			stream << "}\n";
			return stream.str();
		}

		std::string MakeGameReflectionScript() {
			return R"ps1(param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [string[]]$ScanRoots = @("Game"),
    [string]$OutputDir = "Game",
    [string]$OutputName = "GameObjectRegistry.generated",
    [string]$Namespace = "CalyxEngine",
    [string]$FunctionName = "RegisterGeneratedGameSceneObjects"
)

$ErrorActionPreference = "Stop"

$mutexName = "Global\CalyxGameReflectionGenerator_" + [Convert]::ToBase64String(
    [System.Text.Encoding]::UTF8.GetBytes((Resolve-Path $Root).Path)
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
        'CALYX_(?<kind>OBJECT|PLACEABLE_OBJECT)\s*\((?<meta>.*?)\)\s*class\s+(?:(?:[A-Z_][A-Z0-9_]*|__declspec\s*\([^)]*\))\s+)*(?<class>[A-Za-z_][A-Za-z0-9_]*)(?:\s+(?:final|abstract))*\s*:',
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
                $markerKind = $match.Groups["kind"].Value
                $typeName = if ($meta.ContainsKey("TypeName")) { $meta["TypeName"] } else { $className }
                $displayName = if ($meta.ContainsKey("DisplayName")) { $meta["DisplayName"] } else { $typeName }
                $category = if ($meta.ContainsKey("Category")) { $meta["Category"] } else { "None" }
                $icon = if ($meta.ContainsKey("Icon")) { $meta["Icon"] } else { "UI/Tool/AssetPanel/generic.png" }
                $defaultPlaceable = $true
                $defaultSceneSerializable = $true
                $defaultPrefabSerializable = $true
                $defaultPrefabEditable = $false
                $defaultPrefabRoot = $false
                $placeable = if ($meta.ContainsKey("Placeable")) { $meta["Placeable"].ToLower() -ne "false" } else { $defaultPlaceable }
                $prefabEditable = if ($meta.ContainsKey("PrefabEditable")) { $meta["PrefabEditable"].ToLower() -eq "true" } else { $defaultPrefabEditable }
                $prefabRoot = if ($meta.ContainsKey("PrefabRoot")) { $meta["PrefabRoot"].ToLower() -eq "true" } else { $defaultPrefabRoot }
                $sceneSerializable = if ($meta.ContainsKey("SceneSerializable")) { $meta["SceneSerializable"].ToLower() -ne "false" } else { $defaultSceneSerializable }
                $prefabSerializable = if ($meta.ContainsKey("PrefabSerializable")) { $meta["PrefabSerializable"].ToLower() -ne "false" } else { $defaultPrefabSerializable }
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

    $header = @"
#pragma once

namespace $Namespace {
	void $FunctionName();
}
"@

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
)ps1";
		}

		// ゲームリポジトリ用の除外設定
		std::string MakeGameGitIgnore() {
			return
				"Generated/\n"
				".vs/\n"
				"*.user\n"
				"*.suo\n"
				"*.VC.db\n"
				"*.VC.opendb\n"
				"*.pdb\n"
				"*.ilk\n"
				"*.obj\n"
				"*.log\n";
		}

		// ゲームプロジェクト側の初期README
		std::string MakeGameReadme(const Calyx::ProjectInfo& project) {
			{
				std::stringstream stream;
				stream << "# " << project.name << "\n\n";
				stream << "## Build and run\n\n";
				stream << "This repository contains only the game project. Engine binaries and SDK files are installed by CalyxLauncher from the CalyxEngine GitHub release that matches `.calyxproj` `engineVersion`.\n\n";
				stream << "1. Open `" << project.name << ".sln` in Visual Studio.\n";
				stream << "2. Build `CalyxLauncher` when you want to install or update the engine package.\n";
				stream << "3. Set `" << project.name << "` as the startup project and run Debug, Develop, or Release.\n\n";
				stream << "`" << project.name << "` builds the game code as a DLL under `Generated/Outputs/<Configuration>`. `CalyxGame` is the engine-side host executable that loads that DLL from the paths stored in `.calyxproj`.\n\n";
				stream << "Default SDK path: `%LOCALAPPDATA%\\\\CalyxEngine\\\\Engines\\\\" << project.engineVersion << "\\\\SDK`.\n";
				stream << "`CALYX_ENGINE_SDK_DIR` can override this path for local SDK testing.\n\n";
				stream << "Debug and Develop builds are intended for editor/development UI. Release builds are intended to run without that GUI.\n";
				return stream.str();
			}
			std::stringstream stream;
			stream << "# " << project.name << "\n\n";
			stream << "## ビルドとデバッグ\n\n";
			stream << "このプロジェクトはゲームコードを EXE としてビルドし、CalyxLauncher 経由で実行します。\n\n";
			stream << "1. `" << project.name << ".sln` を Visual Studio で開きます。\n";
			stream << "2. `" << project.name << "` をスタートアッププロジェクトにします。\n";
			stream << "3. Debug/Develop/Release を選んで F5 実行します。\n\n";
			stream << "既定では `.calyxproj` の `engineVersion` に対応する SDK を `%LOCALAPPDATA%\\\\CalyxEngine\\\\Engines\\\\" << project.engineVersion << "\\\\SDK` から参照します。\n";
			stream << "初回ビルド時にローカルに対応バージョンが無い場合、`Tools\\\\CalyxLauncher.exe` が CalyxEngine の GitHub Releases から同じ tag の zip を取得します。\n";
			stream << "CalyxLauncher も起動時に同じバージョン解決を行います。\n";
			stream << "別の SDK を使いたい場合だけ `CALYX_ENGINE_SDK_DIR` を設定してください。\n\n";
			stream << "PowerShell の override 設定例:\n\n";
			stream << "```powershell\n";
			stream << "[Environment]::SetEnvironmentVariable(\"CALYX_ENGINE_SDK_DIR\", \"C:\\\\Calyx\\\\Engines\\\\" << project.engineVersion << "\\\\SDK\", \"User\")\n";
			stream << "```\n\n";
			stream << "ブレークポイントはゲーム側の `.cpp` に置けます。\n\n";
			stream << "Engine の更新はリードが検証後に `.calyxproj` の `engineVersion` を更新し、チームへ共有してください。\n";
			return stream.str();
		}

		std::string MakeGameInstallCalyxSdkScript() {
			return R"ps1(param(
	[Parameter(Mandatory = $true)]
	[string]$VersionValue,
	[Parameter(Mandatory = $true)]
	[string]$InstallRoot
)

$ErrorActionPreference = 'Stop'

$versionDir = Join-Path $InstallRoot $VersionValue
$sdkHeader = Join-Path $versionDir 'SDK\Include\CalyxEngine\Application.h'
$sdkData = Join-Path $versionDir 'SDK\Include\Data\Engine'
$sdkNlohmann = Join-Path $versionDir 'SDK\Include\externals\nlohmann\json.hpp'
$sdkReflectionTool = Join-Path $versionDir 'SDK\Tools\Reflection\generate_reflection.ps1'
if ((Test-Path $sdkHeader) -and (Test-Path $sdkData) -and (Test-Path $sdkNlohmann) -and (Test-Path $sdkReflectionTool)) {
	exit 0
}

$headers = @{ 'User-Agent' = 'CalyxGameBuild' }
if ($env:CALYX_ENGINE_GITHUB_TOKEN) {
	$headers['Authorization'] = "Bearer $env:CALYX_ENGINE_GITHUB_TOKEN"
}

$releaseTag = if ($VersionValue.StartsWith('v')) { $VersionValue } else { "v$VersionValue" }
$assetName = "CalyxGamePackage-$releaseTag.zip"
$downloadUrl = "https://github.com/kase-haruto/CalyxEngine/releases/download/$releaseTag/$assetName"

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("CalyxEngine-" + [System.Guid]::NewGuid().ToString("N"))
$extractDir = Join-Path $tempRoot "extract"
New-Item -ItemType Directory -Force -Path $extractDir | Out-Null

try {
	$zipPath = Join-Path $tempRoot $assetName
	Invoke-WebRequest -UseBasicParsing -Headers $headers -Uri $downloadUrl -OutFile $zipPath -TimeoutSec 600
	Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force

	$candidates = @((Get-Item $extractDir)) + @(Get-ChildItem $extractDir -Directory -Recurse)
	$package = $candidates |
		Where-Object {
			(Test-Path (Join-Path $_.FullName 'CalyxEditor.exe')) -and
			(Test-Path (Join-Path $_.FullName 'SDK\Include\CalyxEngine\Application.h')) -and
			(Test-Path (Join-Path $_.FullName 'SDK\Tools\Reflection\generate_reflection.ps1'))
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
		}

		// ゲーム用vcxprojを生成
		std::string MakeGameVcxproj(
			const Calyx::ProjectInfo& project,
			const std::vector<std::filesystem::path>& sourceFiles,
			const std::string& projectGuid) {

			const std::string projectName = EscapeXml(project.name);
			const std::string sourceDir = project.sourceDirectory.generic_string();
			const std::string sdk = "$(CalyxEngineSdkDir)";

			std::stringstream stream;
			stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
			stream << "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
			stream << "  <ItemGroup Label=\"ProjectConfigurations\">\n";
			stream << "    <ProjectConfiguration Include=\"Debug|x64\"><Configuration>Debug</Configuration><Platform>x64</Platform></ProjectConfiguration>\n";
			stream << "    <ProjectConfiguration Include=\"Release|x64\"><Configuration>Release</Configuration><Platform>x64</Platform></ProjectConfiguration>\n";
			stream << "    <ProjectConfiguration Include=\"Develop|x64\"><Configuration>Develop</Configuration><Platform>x64</Platform></ProjectConfiguration>\n";
			stream << "  </ItemGroup>\n";
			stream << "  <PropertyGroup Label=\"Globals\">\n";
			stream << "    <VCProjectVersion>17.0</VCProjectVersion>\n";
			stream << "    <Keyword>Win32Proj</Keyword>\n";
			stream << "    <ProjectGuid>{" << projectGuid << "}</ProjectGuid>\n";
			stream << "    <RootNamespace>" << projectName << "</RootNamespace>\n";
			stream << "    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>\n";
			stream << "    <ProjectName>" << projectName << "</ProjectName>\n";
			stream << "  </PropertyGroup>\n";
			stream << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\" Label=\"Configuration\"><ConfigurationType>DynamicLibrary</ConfigurationType><UseDebugLibraries>true</UseDebugLibraries><PlatformToolset>v145</PlatformToolset><CharacterSet>Unicode</CharacterSet></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\" Label=\"Configuration\"><ConfigurationType>DynamicLibrary</ConfigurationType><UseDebugLibraries>false</UseDebugLibraries><PlatformToolset>v145</PlatformToolset><WholeProgramOptimization>true</WholeProgramOptimization><CharacterSet>Unicode</CharacterSet></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Develop|x64'\" Label=\"Configuration\"><ConfigurationType>DynamicLibrary</ConfigurationType><UseDebugLibraries>false</UseDebugLibraries><PlatformToolset>v145</PlatformToolset><WholeProgramOptimization>true</WholeProgramOptimization><CharacterSet>Unicode</CharacterSet></PropertyGroup>\n";
			stream << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n";
			stream << "  <ImportGroup Label=\"ExtensionSettings\" />\n";
			stream << "  <ImportGroup Label=\"Shared\" />\n";
			stream << "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\"><Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" /></ImportGroup>\n";
			stream << "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\"><Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" /></ImportGroup>\n";
			stream << "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Develop|x64'\"><Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" /></ImportGroup>\n";
			stream << "  <PropertyGroup Label=\"UserMacros\" />\n";
			stream << "  <PropertyGroup Label=\"CalyxEngineSdk\">\n";
			stream << "    <CalyxEngineVersion>" << EscapeXml(project.engineVersion) << "</CalyxEngineVersion>\n";
			stream << "    <CalyxEngineSdkDir Condition=\"'$(CalyxEngineSdkDir)'=='' and Exists('$(LOCALAPPDATA)\\CalyxEngine\\Engines\\$(CalyxEngineVersion)\\SDK\\Include\\CalyxEngine\\Application.h') and Exists('$(LOCALAPPDATA)\\CalyxEngine\\Engines\\$(CalyxEngineVersion)\\SDK\\Include\\Data\\Engine') and Exists('$(LOCALAPPDATA)\\CalyxEngine\\Engines\\$(CalyxEngineVersion)\\SDK\\Include\\externals\\nlohmann\\json.hpp')\">$(LOCALAPPDATA)\\CalyxEngine\\Engines\\$(CalyxEngineVersion)\\SDK</CalyxEngineSdkDir>\n";
			stream << "    <CalyxEngineSdkDir Condition=\"'$(CalyxEngineSdkDir)'=='' and '$(CALYX_ENGINE_SDK_DIR)'!='' and Exists('$(CALYX_ENGINE_SDK_DIR)\\Include\\CalyxEngine\\Application.h') and Exists('$(CALYX_ENGINE_SDK_DIR)\\Include\\Data\\Engine') and Exists('$(CALYX_ENGINE_SDK_DIR)\\Include\\externals\\nlohmann\\json.hpp')\">$(CALYX_ENGINE_SDK_DIR)</CalyxEngineSdkDir>\n";
			stream << "    <CalyxEngineSdkDir Condition=\"'$(CalyxEngineSdkDir)'==''\">$(LOCALAPPDATA)\\CalyxEngine\\Engines\\$(CalyxEngineVersion)\\SDK</CalyxEngineSdkDir>\n";
			stream << "    <CalyxProjectReflectionTool>$(ProjectDir)Tools\\Reflection\\generate_reflection.ps1</CalyxProjectReflectionTool>\n";
			stream << "    <CalyxReflectionTool Condition=\"Exists('$(CalyxProjectReflectionTool)')\">$(CalyxProjectReflectionTool)</CalyxReflectionTool>\n";
			stream << "    <CalyxReflectionTool Condition=\"'$(CalyxReflectionTool)'==''\">$(CalyxEngineSdkDir)\\Tools\\Reflection\\generate_reflection.ps1</CalyxReflectionTool>\n";
			stream << "  </PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\"><IntDir>Generated\\Obj\\$(ProjectName)\\$(Configuration)\\</IntDir><OutDir>Generated\\Outputs\\$(Configuration)\\</OutDir><LinkIncremental>true</LinkIncremental></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\"><IntDir>Generated\\Obj\\$(ProjectName)\\$(Configuration)\\</IntDir><OutDir>Generated\\Outputs\\$(Configuration)\\</OutDir></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Develop|x64'\"><IntDir>Generated\\Obj\\$(ProjectName)\\$(Configuration)\\</IntDir><OutDir>Generated\\Outputs\\$(Configuration)\\</OutDir></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\"><LocalDebuggerCommand>$(CalyxEngineSdkDir)\\Bin\\$(Configuration)\\CalyxGame.exe</LocalDebuggerCommand><LocalDebuggerWorkingDirectory>$(ProjectDir)</LocalDebuggerWorkingDirectory><LocalDebuggerCommandArguments>&quot;$(ProjectDir)" << EscapeXml(project.name) << ".calyxproj&quot; --config &quot;$(Configuration)&quot;</LocalDebuggerCommandArguments></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\"><LocalDebuggerCommand>$(CalyxEngineSdkDir)\\Bin\\$(Configuration)\\CalyxGame.exe</LocalDebuggerCommand><LocalDebuggerWorkingDirectory>$(ProjectDir)</LocalDebuggerWorkingDirectory><LocalDebuggerCommandArguments>&quot;$(ProjectDir)" << EscapeXml(project.name) << ".calyxproj&quot; --config &quot;$(Configuration)&quot;</LocalDebuggerCommandArguments></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Develop|x64'\"><LocalDebuggerCommand>$(CalyxEngineSdkDir)\\Bin\\$(Configuration)\\CalyxGame.exe</LocalDebuggerCommand><LocalDebuggerWorkingDirectory>$(ProjectDir)</LocalDebuggerWorkingDirectory><LocalDebuggerCommandArguments>&quot;$(ProjectDir)" << EscapeXml(project.name) << ".calyxproj&quot; --config &quot;$(Configuration)&quot;</LocalDebuggerCommandArguments></PropertyGroup>\n";

			const char* configs[] = {"Debug", "Release", "Develop"};
			for(const char* config : configs) {
				const bool debug = std::string(config) == "Debug";
				stream << "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='" << config << "|x64'\">\n";
				stream << "    <ClCompile>\n";
				stream << "      <PreprocessorDefinitions>" << (debug ? "_DEBUG" : (std::string(config) == "Develop" ? "DEVELOP" : "NDEBUG")) << ";_WINDOWS;IMGUI_API=__declspec(dllimport);IGFD_API=__declspec(dllimport);%(PreprocessorDefinitions)</PreprocessorDefinitions>\n";
				stream << "      <ConformanceMode>true</ConformanceMode>\n";
				stream << "      <LanguageStandard>stdcpp20</LanguageStandard>\n";
				stream << "      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>\n";
				stream << "      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n";
				stream << "      <TreatWarningAsError>true</TreatWarningAsError>\n";
				stream << "      <AdditionalIncludeDirectories>$(ProjectDir);$(ProjectDir)" << sourceDir << ";" << sdk << "\\Include;" << sdk << "\\Include\\Engine\\Application;" << sdk << "\\ThirdParty;" << sdk << "\\ThirdParty\\DirectXTex;" << sdk << "\\ThirdParty\\assimp\\include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>\n";
				stream << "      <RuntimeLibrary>" << (debug ? "MultiThreadedDebug" : "MultiThreaded") << "</RuntimeLibrary>\n";
				stream << "      <BasicRuntimeChecks>Default</BasicRuntimeChecks>\n";
				if(debug) {
					stream << "      <DebugInformationFormat>ProgramDatabase</DebugInformationFormat>\n";
				}
				stream << "    </ClCompile>\n";
				stream << "    <Link>\n";
				stream << "      <SubSystem>Windows</SubSystem>\n";
				if(!debug) {
					stream << "      <EnableCOMDATFolding>true</EnableCOMDATFolding>\n";
					stream << "      <OptimizeReferences>true</OptimizeReferences>\n";
				}
				stream << "      <GenerateDebugInformation>true</GenerateDebugInformation>\n";
				stream << "      <AdditionalDependencies>CalyxEngine.lib;DirectXTex.lib;" << (debug ? "assimp-vc143-mtd.lib" : "assimp-vc143-mt.lib") << ";%(AdditionalDependencies)</AdditionalDependencies>\n";
				stream << "      <AdditionalLibraryDirectories>$(CalyxEngineSdkDir)\\Lib\\$(Configuration);$(CalyxEngineSdkDir)\\Lib\\DirectXTex\\x64\\" << (debug ? "Debug" : "Release") << ";$(CalyxEngineSdkDir)\\Lib\\assimp\\" << (debug ? "Debug" : "Release") << ";%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>\n";
				stream << "      <AdditionalOptions>/IGNORE:4099 /FORCE:MULTIPLE %(AdditionalOptions)</AdditionalOptions>\n";
				stream << "    </Link>\n";
				stream << "    <PostBuildEvent><Command>if exist \"$(CalyxEngineSdkDir)\\Bin\\$(Configuration)\\CalyxEngine.dll\" copy \"$(CalyxEngineSdkDir)\\Bin\\$(Configuration)\\CalyxEngine.dll\" \"$(TargetDir)CalyxEngine.dll\"\n";
				stream << "if not exist \"$(TargetDir)CalyxEngine.dll\" if exist \"$(CalyxEngineSdkDir)\\..\\CalyxEngine.dll\" copy \"$(CalyxEngineSdkDir)\\..\\CalyxEngine.dll\" \"$(TargetDir)CalyxEngine.dll\"\n";
				stream << "if exist \"$(WindowsSdkDir)bin\\$(TargetPlatformVersion)\\x64\\dxcompiler.dll\" copy \"$(WindowsSdkDir)bin\\$(TargetPlatformVersion)\\x64\\dxcompiler.dll\" \"$(TargetDir)dxcompiler.dll\"\n";
				stream << "if exist \"$(WindowsSdkDir)bin\\$(TargetPlatformVersion)\\x64\\dxil.dll\" copy \"$(WindowsSdkDir)bin\\$(TargetPlatformVersion)\\x64\\dxil.dll\" \"$(TargetDir)dxil.dll\"</Command></PostBuildEvent>\n";
				stream << "  </ItemDefinitionGroup>\n";
			}

			stream << "  <ItemGroup>\n";
			for(const auto& sourceFile : sourceFiles) {
				stream << "    <ClCompile Include=\"" << EscapeXml(ToVisualStudioPath(project.rootDirectory, sourceFile)) << "\" />\n";
			}
			stream << "    <ClCompile Include=\"Generated\\Foundation\\Reflection\\CalyxGameObjectRegistry.generated.cpp\" />\n";
			stream << "  </ItemGroup>\n";
			stream << "  <ItemGroup>\n";
			stream << "    <ClInclude Include=\"Generated\\Foundation\\Reflection\\CalyxGameObjectRegistry.generated.h\" />\n";
			stream << "  </ItemGroup>\n";
			stream << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n";
			stream << "  <Target Name=\"EnsureCalyxEngineSdk\" BeforeTargets=\"PrepareForBuild\" Condition=\"!Exists('$(CalyxEngineSdkDir)\\Include\\CalyxEngine\\Application.h') or !Exists('$(CalyxEngineSdkDir)\\Include\\Data\\Engine') or !Exists('$(CalyxEngineSdkDir)\\Include\\externals\\nlohmann\\json.hpp') or !Exists('$(CalyxReflectionTool)') or !Exists('$(CalyxEngineSdkDir)\\Lib\\$(Configuration)\\CalyxEngine.lib') or !Exists('$(CalyxEngineSdkDir)\\Bin\\$(Configuration)\\CalyxGame.exe')\">\n";
			stream << "    <Message Importance=\"high\" Text=\"Installing Calyx SDK $(CalyxEngineVersion) from GitHub Releases...\" />\n";
			stream << "    <Exec Command=\"&quot;$(ProjectDir)Tools\\CalyxLauncher.exe&quot; &quot;$(ProjectDir)" << projectName << ".calyxproj&quot;\" />\n";
			stream << "  </Target>\n";
			stream << "  <Target Name=\"ValidateCalyxEngineSdkDir\" BeforeTargets=\"PrepareForBuild\" DependsOnTargets=\"EnsureCalyxEngineSdk\">\n";
			stream << "    <Error Condition=\"'$(CalyxEngineSdkDir)'==''\" Text=\"CalyxEngineSdkDir is empty. Set CALYX_ENGINE_SDK_DIR or install the SDK under %LOCALAPPDATA%\\CalyxEngine\\Engines\\$(CalyxEngineVersion)\\SDK.\" />\n";
			stream << "    <Error Condition=\"'$(CalyxEngineSdkDir)'!='' and !Exists('$(CalyxEngineSdkDir)\\Include\\CalyxEngine\\Application.h')\" Text=\"CalyxEngineSdkDir does not point to a Calyx SDK directory: $(CalyxEngineSdkDir)\" />\n";
			stream << "    <Error Condition=\"'$(CalyxEngineSdkDir)'!='' and !Exists('$(CalyxEngineSdkDir)\\Include\\Data\\Engine')\" Text=\"CalyxEngineSdkDir is missing Data headers. Update or reinstall the Calyx engine SDK: $(CalyxEngineSdkDir)\" />\n";
			stream << "    <Error Condition=\"'$(CalyxEngineSdkDir)'!='' and !Exists('$(CalyxEngineSdkDir)\\Include\\externals\\nlohmann\\json.hpp')\" Text=\"CalyxEngineSdkDir is missing external headers. Update or reinstall the Calyx engine SDK: $(CalyxEngineSdkDir)\" />\n";
			stream << "    <Error Condition=\"'$(CalyxEngineSdkDir)'!='' and !Exists('$(CalyxReflectionTool)')\" Text=\"Calyx reflection generator was not found. Update or reinstall the Calyx engine SDK: $(CalyxReflectionTool)\" />\n";
			stream << "    <Error Condition=\"'$(CalyxEngineSdkDir)'!='' and !Exists('$(CalyxEngineSdkDir)\\Lib\\$(Configuration)\\CalyxEngine.lib')\" Text=\"CalyxEngine.lib was not found for $(Configuration). Update or reinstall the Calyx engine SDK: $(CalyxEngineSdkDir)\" />\n";
			stream << "    <Error Condition=\"'$(CalyxEngineSdkDir)'!='' and !Exists('$(CalyxEngineSdkDir)\\Bin\\$(Configuration)\\CalyxGame.exe')\" Text=\"CalyxGame.exe was not found for $(Configuration). Update or reinstall the Calyx engine SDK: $(CalyxEngineSdkDir)\" />\n";
			stream << "  </Target>\n";
			stream << "  <Target Name=\"GenerateCalyxGameReflection\" BeforeTargets=\"ClCompile\" DependsOnTargets=\"ValidateCalyxEngineSdkDir\">\n";
			stream << "    <Message Importance=\"high\" Text=\"Generating Calyx game object registry from " << EscapeXml(project.name) << ".calyxproj...\" />\n";
			stream << "    <Exec Command=\"powershell -NoProfile -ExecutionPolicy Bypass -File &quot;$(CalyxReflectionTool)&quot; -ProjectFile &quot;$(ProjectDir)" << projectName << ".calyxproj&quot; -OutputName CalyxGameObjectRegistry.generated -FunctionName RegisterGeneratedGameSceneObjects\" />\n";
			stream << "  </Target>\n";
			stream << "  <ImportGroup Label=\"ExtensionTargets\" />\n";
			stream << "</Project>\n";
			return stream.str();
		}

		std::string MakeGameFilters(const Calyx::ProjectInfo& project, const std::vector<std::filesystem::path>& sourceFiles) {

			std::stringstream stream;
			stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
			stream << "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
			stream << "  <ItemGroup>\n";
			stream << "    <Filter Include=\"Game\"><UniqueIdentifier>{" << MakeGuid() << "}</UniqueIdentifier></Filter>\n";
			stream << "  </ItemGroup>\n";
			stream << "  <ItemGroup>\n";
			for(const auto& sourceFile : sourceFiles) {
				stream << "    <ClCompile Include=\"" << EscapeXml(ToVisualStudioPath(project.rootDirectory, sourceFile)) << "\"><Filter>Game</Filter></ClCompile>\n";
			}
			stream << "    <ClCompile Include=\"Generated\\Foundation\\Reflection\\CalyxGameObjectRegistry.generated.cpp\"><Filter>Game</Filter></ClCompile>\n";
			stream << "  </ItemGroup>\n";
			stream << "  <ItemGroup>\n";
			stream << "    <ClInclude Include=\"Generated\\Foundation\\Reflection\\CalyxGameObjectRegistry.generated.h\"><Filter>Game</Filter></ClInclude>\n";
			stream << "  </ItemGroup>\n";
			stream << "</Project>\n";
			return stream.str();
		}

		// ゲーム用slnを生成
		std::string MakeGeneratedToolVcxproj(
			const Calyx::ProjectInfo& project,
			const std::string& projectGuid,
			const std::string& projectName) {
			const std::string gameName = EscapeXml(project.name);

			std::stringstream stream;
			stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
			stream << "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
			stream << "  <ItemGroup Label=\"ProjectConfigurations\">\n";
			stream << "    <ProjectConfiguration Include=\"Debug|x64\"><Configuration>Debug</Configuration><Platform>x64</Platform></ProjectConfiguration>\n";
			stream << "    <ProjectConfiguration Include=\"Develop|x64\"><Configuration>Develop</Configuration><Platform>x64</Platform></ProjectConfiguration>\n";
			stream << "    <ProjectConfiguration Include=\"Release|x64\"><Configuration>Release</Configuration><Platform>x64</Platform></ProjectConfiguration>\n";
			stream << "  </ItemGroup>\n";
			stream << "  <PropertyGroup Label=\"Globals\"><VCProjectVersion>17.0</VCProjectVersion><Keyword>MakeFileProj</Keyword><ProjectGuid>{" << projectGuid << "}</ProjectGuid><RootNamespace>" << projectName << "</RootNamespace><WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion><ProjectName>" << projectName << "</ProjectName></PropertyGroup>\n";
			stream << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\" Label=\"Configuration\"><ConfigurationType>Makefile</ConfigurationType><UseDebugLibraries>true</UseDebugLibraries><PlatformToolset>v145</PlatformToolset></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Develop|x64'\" Label=\"Configuration\"><ConfigurationType>Makefile</ConfigurationType><UseDebugLibraries>false</UseDebugLibraries><PlatformToolset>v145</PlatformToolset></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\" Label=\"Configuration\"><ConfigurationType>Makefile</ConfigurationType><UseDebugLibraries>false</UseDebugLibraries><PlatformToolset>v145</PlatformToolset></PropertyGroup>\n";
			stream << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n";
			stream << "  <PropertyGroup Label=\"UserMacros\" />\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\"><IntDir>Generated\\Obj\\$(ProjectName)\\$(Configuration)\\</IntDir><OutDir>Generated\\Outputs\\$(Configuration)\\</OutDir></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Develop|x64'\"><IntDir>Generated\\Obj\\$(ProjectName)\\$(Configuration)\\</IntDir><OutDir>Generated\\Outputs\\$(Configuration)\\</OutDir></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\"><IntDir>Generated\\Obj\\$(ProjectName)\\$(Configuration)\\</IntDir><OutDir>Generated\\Outputs\\$(Configuration)\\</OutDir></PropertyGroup>\n";
			stream << "  <PropertyGroup>\n";
			stream << "    <ProjectLauncher>$(ProjectDir)Tools\\CalyxLauncher.exe</ProjectLauncher>\n";
			stream << "    <CalyxEnginePackageDir>$(LOCALAPPDATA)\\CalyxEngine\\Engines\\" << EscapeXml(project.engineVersion) << "\\</CalyxEnginePackageDir>\n";
			stream << "    <NMakeBuildCommandLine>&quot;$(ProjectLauncher)&quot; &quot;$(ProjectDir)" << gameName << ".calyxproj&quot;</NMakeBuildCommandLine>\n";
			stream << "    <NMakeReBuildCommandLine>&quot;$(ProjectLauncher)&quot; &quot;$(ProjectDir)" << gameName << ".calyxproj&quot; --force</NMakeReBuildCommandLine>\n";
			stream << "    <NMakeCleanCommandLine>echo CalyxLauncher has no generated build outputs to clean.</NMakeCleanCommandLine>\n";
			// Intentionally omit NMakeOutput: every explicit launcher build validates
			// the complete cached package, while an already valid package exits quickly.
			stream << "    <LocalDebuggerCommand>$(ProjectLauncher)</LocalDebuggerCommand>\n";
			stream << "    <LocalDebuggerCommandArguments>&quot;$(ProjectDir)" << gameName << ".calyxproj&quot;</LocalDebuggerCommandArguments>\n";
			stream << "    <LocalDebuggerWorkingDirectory>$(ProjectDir)</LocalDebuggerWorkingDirectory>\n";
			stream << "  </PropertyGroup>\n";
			stream << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n";
			stream << "  <Target Name=\"ValidateProjectLauncher\" BeforeTargets=\"Build;ReBuild\"><Error Condition=\"!Exists('$(ProjectLauncher)')\" Text=\"CalyxLauncher was not found: $(ProjectLauncher)\" /></Target>\n";
			stream << "  <ImportGroup Label=\"ExtensionTargets\" />\n";
			stream << "</Project>\n";
			return stream.str();
		}

		std::string MakeGameSolution(
			const Calyx::ProjectInfo& project,
			const std::string& gameProjectGuid,
			const std::string& launcherProjectGuid,
			const std::string& solutionGuid) {
			const std::string projectName = project.name;
			const std::string projectFile = (project.name + ".vcxproj");

			std::stringstream stream;
			stream << "\nMicrosoft Visual Studio Solution File, Format Version 12.00\n";
			stream << "# Visual Studio Version 18\n";
			stream << "VisualStudioVersion = 18.0.0.0\n";
			stream << "MinimumVisualStudioVersion = 10.0.40219.1\n";
			stream << "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"CalyxLauncher\", \"CalyxLauncher.vcxproj\", \"{" << launcherProjectGuid << "}\"\n";
			stream << "EndProject\n";
			stream << "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"" << projectName << "\", \"" << projectFile << "\", \"{" << gameProjectGuid << "}\"\n";
			stream << "EndProject\n";
			stream << "Global\n";
			stream << "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n";
			stream << "\t\tDebug|x64 = Debug|x64\n\t\tDevelop|x64 = Develop|x64\n\t\tRelease|x64 = Release|x64\n";
			stream << "\tEndGlobalSection\n";
			stream << "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n";
			stream << "\t\t{" << launcherProjectGuid << "}.Debug|x64.ActiveCfg = Debug|x64\n";
			stream << "\t\t{" << launcherProjectGuid << "}.Debug|x64.Build.0 = Debug|x64\n";
			stream << "\t\t{" << launcherProjectGuid << "}.Develop|x64.ActiveCfg = Develop|x64\n";
			stream << "\t\t{" << launcherProjectGuid << "}.Develop|x64.Build.0 = Develop|x64\n";
			stream << "\t\t{" << launcherProjectGuid << "}.Release|x64.ActiveCfg = Release|x64\n";
			stream << "\t\t{" << launcherProjectGuid << "}.Release|x64.Build.0 = Release|x64\n";
			stream << "\t\t{" << gameProjectGuid << "}.Debug|x64.ActiveCfg = Debug|x64\n";
			stream << "\t\t{" << gameProjectGuid << "}.Debug|x64.Build.0 = Debug|x64\n";
			stream << "\t\t{" << gameProjectGuid << "}.Develop|x64.ActiveCfg = Develop|x64\n";
			stream << "\t\t{" << gameProjectGuid << "}.Develop|x64.Build.0 = Develop|x64\n";
			stream << "\t\t{" << gameProjectGuid << "}.Release|x64.ActiveCfg = Release|x64\n";
			stream << "\t\t{" << gameProjectGuid << "}.Release|x64.Build.0 = Release|x64\n";
			stream << "\tEndGlobalSection\n";
			stream << "\tGlobalSection(SolutionProperties) = preSolution\n";
			stream << "\t\tHideSolutionNode = FALSE\n";
			stream << "\tEndGlobalSection\n";
			stream << "\tGlobalSection(ExtensibilityGlobals) = postSolution\n";
			stream << "\t\tSolutionGuid = {" << solutionGuid << "}\n";
			stream << "\tEndGlobalSection\n";
			stream << "EndGlobal\n";
			return stream.str();
		}

		// 新規ゲームプロジェクト用のソリューション、プロジェクトファイル、起動用GameMain.cppなどを一式作成する
		bool CreateGameWorkspace(const Calyx::ProjectInfo& project, const ProjectBrowser::TemplateInfo& selectedTemplate) {
			// 配布済みエンジンのインストールディレクトリを取得（Resourcesやテンプレートコピーのため）
			const auto engineDirectory = FindInstalledEngineDirectory();
			if(engineDirectory.empty()) {
				return false;
			}
			const auto launcherExecutable = FindLauncherExecutable(engineDirectory);
			if(launcherExecutable.empty()) {
				return false;
			}

			// ソリューションおよびvcxprojファイルで紐付ける新規の個別GUIDを発行
			const std::string projectGuid = MakeGuid();
			const std::string launcherProjectGuid = MakeGuid();
			const std::string solutionGuid = MakeGuid();

			// ゲーム実行時にもエンジン共通の画像やシェーダーリソースを使うため、Resourcesディレクトリを丸ごとコピー
			if(!CopyDirectoryTree(engineDirectory / "Resources", project.rootDirectory / "Resources")) {
				return false;
			}
			{
				// CalyxLauncher is a project tool, not a build artifact. Keep it
				// outside Generated while still avoiding any CalyxEditor copy.
				const auto generatedLauncher = project.rootDirectory / "Tools" / "CalyxLauncher.exe";
				std::error_code ec;
				std::filesystem::create_directories(generatedLauncher.parent_path(), ec);
				if(ec) {
					return false;
				}
				std::filesystem::copy_file(launcherExecutable, generatedLauncher, std::filesystem::copy_options::overwrite_existing, ec);
				if(ec) {
					return false;
				}
			}
			{
				// Keep the code generator in the game repository so a fresh clone can
				// bootstrap even when an older engine package omitted SDK/Tools.
				const std::filesystem::path reflectionCandidates[] = {
					engineDirectory / "Tools" / "Reflection",
					engineDirectory / "SDK" / "Tools" / "Reflection"};
				bool copiedReflectionTools = false;
				for(const auto& candidate : reflectionCandidates) {
					if(std::filesystem::exists(candidate / "generate_reflection.ps1") &&
					   CopyDirectoryTree(candidate, project.rootDirectory / "Tools" / "Reflection")) {
						copiedReflectionTools = true;
						break;
					}
				}
				if(!copiedReflectionTools) {
					return false;
				}
			}
			// Demo projects use the engine's live Demo source and scene as the single source of truth.
			if(selectedTemplate.type == ProjectTemplateType::Demo) {
				const auto demoSourceDirectory = FindDemoSourceDirectory(engineDirectory);
				if(demoSourceDirectory.empty()) {
					return false;
				}

				if(!CopyDirectoryTree(demoSourceDirectory, Calyx::ResolveProjectPath(project, project.sourceDirectory) / "Demo")) {
					return false;
				}

				const auto sourceScenePath = engineDirectory / "Resources" / "Assets" / "Scenes" / "DemoScene.scene";
				if(!std::filesystem::exists(sourceScenePath)) {
					return false;
				}
				const auto outputScenePath = Calyx::ResolveProjectPath(project, selectedTemplate.startupScene);
				std::error_code ec;
				std::filesystem::create_directories(outputScenePath.parent_path(), ec);
				if(ec) {
					return false;
				}
				std::filesystem::copy_file(sourceScenePath, outputScenePath, std::filesystem::copy_options::overwrite_existing, ec);
				if(ec) {
					return false;
				}
			}

			// アプリのエントリポイントとなる初期「GameMain.cpp」ファイルを生成してSource下に書き出し
			const auto sourceDirectory = Calyx::ResolveProjectPath(project, project.sourceDirectory);
			if(!WriteTextFile(sourceDirectory / "GameMain.cpp", MakeGameMainSource())) {
				return false;
			}
			if(!WriteTextFile(sourceDirectory / "GameApplication.h", MakeGameApplicationHeader())) {
				return false;
			}
			if(!WriteTextFile(sourceDirectory / "GameApplication.cpp", MakeGameApplicationSource(selectedTemplate))) {
				return false;
			}
			if(!WriteTextFile(sourceDirectory / "GameEditorExtension.cpp", MakeGameEditorExtensionSource())) {
				return false;
			}

			// フォルダ内からソースファイル一覧を収集
			const auto sourceFiles = CollectSourceFiles(project);

			// Visual Studioでビルド可能にするための各定義ファイルをテキストとして生成し書き出す
			if(!WriteTextFile(project.rootDirectory / (project.name + ".vcxproj"), MakeGameVcxproj(project, sourceFiles, projectGuid))) {
				return false;
			}
			if(!WriteTextFile(project.rootDirectory / (project.name + ".vcxproj.filters"), MakeGameFilters(project, sourceFiles))) {
				return false;
			}
			if(!WriteTextFile(project.rootDirectory / "CalyxLauncher.vcxproj", MakeGeneratedToolVcxproj(project, launcherProjectGuid, "CalyxLauncher"))) {
				return false;
			}
			if(!WriteTextFile(project.rootDirectory / (project.name + ".sln"), MakeGameSolution(project, projectGuid, launcherProjectGuid, solutionGuid))) {
				return false;
			}
			if(!WriteTextFile(project.rootDirectory / ".gitignore", MakeGameGitIgnore())) {
				return false;
			}
			if(!WriteTextFile(project.rootDirectory / "README.md", MakeGameReadme(project))) {
				return false;
			}

			return true;
		}

	} // namespace

	////////////////////////////////////////////////////////////////////////////////////////////
	//						初期化
	////////////////////////////////////////////////////////////////////////////////////////////
	ProjectBrowser::ProjectBrowser()
		: registryPath_(Calyx::DefaultProjectRegistryPath()) {
		// 表示レイアウトやサイズ用のパラメータをロードし、存在しなければ新規作成
		if(!param_.LoadParams()) {
			param_.SaveParams();
		}

		// UI入力バッファの初期化（デフォルトプロジェクト名とユーザーごとの既定フォルダパス）
		CopyText(newProjectName_, "NewProject");
		CopyText(newProjectDirectory_, DefaultUserProjectDirectory().string());
		// 最近使ったプロジェクトリストをレジストリからロード
		ReloadRecentProjects();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						描画
	////////////////////////////////////////////////////////////////////////////////////////////
	bool ProjectBrowser::Draw(Calyx::ProjectInfo& outProject) {
		
		bool selected = false;

		// ビューポートサイズいっぱいのブラウザウインドウを固定配置で描画
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::Begin("Project Browser", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		// 描画アイコンテクスチャをアセットシステムから必要に応じて確保
		LoadIcons();
		
		ImGui::TextUnformatted("Calyx Project Browser");
		ImGui::Separator();

		// 「プロジェクトを開く」ボタンによる既存 `.calyxproj` 選択ダイアログの表示
		if(ImGui::Button("Open Project", ImVec2(param_.openButtonSize_.x, param_.openButtonSize_.y))) {
			IGFD::FileDialogConfig config;
			ImGuiFileDialog::Instance()->OpenDialog("OpenCalyxProject", "Open Calyx Project", ".calyxproj", config);
		}
		ImGui::SameLine();
		// 「更新」ボタンによる最近使ったリストの再スキャン
		if(ImGui::Button("Refresh", ImVec2(param_.refreshButtonSize_.x, param_.refreshButtonSize_.y))) {
			ReloadRecentProjects();
		}

		// ステータスエラー等があれば上部にテキストで通知
		if(!statusMessage_.empty()) {
			ImGui::SameLine();
			ImGui::TextDisabled("%s", statusMessage_.c_str());
		}

		ImGui::Spacing();
		ImGuiStyle& style = ImGui::GetStyle();

		// 新規プロジェクト作成の入力フォーム（下部フッター）の表示高さを事前計算
		const float footerHeight =
			ImGui::GetFrameHeightWithSpacing() * 2.0f +
			style.ItemSpacing.y * 2.0f +
			style.WindowPadding.y;

		// フッターエリアを除いたメイン描画領域
		if(ImGui::BeginChild(
			"ProjectBrowserMainArea",
			ImVec2(0.0f, -footerHeight),
			false,
			ImGuiWindowFlags_NoScrollbar)) {

			// 左右に「最近使ったプロジェクト / テンプレート一覧」と「選択中テンプレート詳細情報」を配置
			if(ImGui::BeginTable(
				"ProjectBrowserLayout",
				param_.tableColumnCount_,
				ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV,
				ImVec2(0.0f, 0.0f))) {

				ImGui::TableSetupColumn("Recent", ImGuiTableColumnFlags_WidthStretch, param_.recentColumnWeight_);
				ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch, param_.detailsColumnWeight_);

				// 左ペイン：最近使った＆テンプレートカード一覧
				ImGui::TableNextColumn();
				DrawRecentProjects(outProject, selected);

				// 右ペイン：選択中のテンプレートの詳細情報表示
				ImGui::TableNextColumn();
				DrawTemplateDetails();

				ImGui::EndTable();
				}
			}
		ImGui::EndChild();

		ImGui::Separator();
		// 下部フッター：新規プロジェクト作成用UI
		DrawNewProject(outProject, selected);
		// 各ポップアップファイルダイアログの遅延・更新処理
		DrawOpenProjectDialog(outProject, selected);
		DrawLocationDialog();

		ImGui::End();
		return selected;
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						最近使ったプロジェクトの再読み込み
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::ReloadRecentProjects() {
		if(!Calyx::LoadRecentProjects(registryPath_, recentProjects_)) {
			recentProjects_.clear();
			statusMessage_ = "Recent project list could not be loaded.";
			return;
		}
		statusMessage_.clear();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						最近使ったプロジェクト一覧
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawRecentProjects(Calyx::ProjectInfo& outProject, bool& selected) {
		DrawTemplateCards();

		ImGui::Spacing();
		ImGui::TextUnformatted("Recent Projects");
		ImGui::Separator();

		if(recentProjects_.empty()) {
			ImGui::TextDisabled("No recent projects.");
		} else {
			ImGui::BeginChild("RecentProjectList", ImVec2(0.0f, 0.0f), false);

			const float cardWidth  = param_.cardSize_.x;
			const float spacing	= ImGui::GetStyle().ItemSpacing.x;
			const float availableWidth = ImGui::GetContentRegionAvail().x;
			int columns = static_cast<int>(availableWidth / (cardWidth + spacing));
			columns = (std::max)(1, columns);

			if(ImGui::BeginTable("RecentProjectCards", columns)) {
				for(size_t i = 0; i < recentProjects_.size(); ++i) {
					ImGui::TableNextColumn();
					ImGui::PushID(static_cast<int>(i));
					DrawRecentProjectCard(recentProjects_[i], outProject, selected);
					ImGui::PopID();
				}
				ImGui::EndTable();
			}

			ImGui::EndChild();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						テンプレート一覧
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawTemplateCards() {
		ImGui::TextUnformatted("Project Templates");
		ImGui::Separator();

		const float cardWidth  = param_.templateCardSize_.x;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float availableWidth = ImGui::GetContentRegionAvail().x;
		int columns = static_cast<int>(availableWidth / (cardWidth + spacing));
		columns = (std::max)(1, columns);

		if(ImGui::BeginTable("TemplateCards", columns)) {
			for(size_t i = 0; i < std::size(kTemplates); ++i) {
				ImGui::TableNextColumn();
				ImGui::PushID(static_cast<int>(i));
				DrawTemplateCard(kTemplates[i]);
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						テンプレートカード
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawTemplateCard(const TemplateInfo& item) {
		const bool selected = selectedTemplate_ == item.type;
		const ImVec2 cardSize(param_.templateCardSize_.x, param_.templateCardSize_.y);
		const ImVec2 cardMin = ImGui::GetCursorScreenPos();

		if(ImGui::Selectable("##template-card", selected, 0, cardSize)) {
			selectedTemplate_ = item.type;
		}

		const ImVec2 cardMax(cardMin.x + cardSize.x, cardMin.y + cardSize.y);
		auto* drawList = ImGui::GetWindowDrawList();
		const ImU32 borderColor = selected
			? ImGui::GetColorU32(ImGuiCol_CheckMark)
			: ImGui::ColorConvertFloat4ToU32(ImVec4(
				  param_.cardBorderColor_.x,
				  param_.cardBorderColor_.y,
				  param_.cardBorderColor_.z,
				  param_.cardBorderColor_.w));
		drawList->AddRect(cardMin, cardMax, borderColor);

		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardPadding_));
		if(genericIcon_) {
			ImGui::Image(genericIcon_, ImVec2(cardSize.x - param_.cardImageWidthOffset_, param_.cardImageHeight_));
		} else {
			ImGui::Dummy(ImVec2(cardSize.x - param_.cardImageWidthOffset_, param_.cardImageHeight_));
		}

		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardNameOffsetY_));
		ImGui::TextWrapped("%s", item.name);
		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardVersionOffsetY_));
		ImGui::TextDisabled("Template");

		ImGui::SetCursorScreenPos(cardMax);
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						最近使ったプロジェクトカード
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawRecentProjectCard(const Calyx::RecentProjectEntry& entry, Calyx::ProjectInfo& outProject, bool& selected) {
		const ImVec2 cardSize(param_.cardSize_.x, param_.cardSize_.y);
		const ImVec2 cardMin = ImGui::GetCursorScreenPos();

		if(ImGui::Selectable("##recent-card", false, 0, cardSize)) {
			selected = LoadProject(entry.projectFile, outProject);
		}

		const ImVec2 cardMax(cardMin.x + cardSize.x, cardMin.y + cardSize.y);
		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddRect(
			cardMin,
			cardMax,
			ImGui::ColorConvertFloat4ToU32(ImVec4(
				param_.cardBorderColor_.x,
				param_.cardBorderColor_.y,
				param_.cardBorderColor_.z,
				param_.cardBorderColor_.w)));

		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardPadding_));
		if(genericIcon_) {
			ImGui::Image(genericIcon_, ImVec2(cardSize.x - param_.cardImageWidthOffset_, param_.cardImageHeight_));
		} else {
			ImGui::Dummy(ImVec2(cardSize.x - param_.cardImageWidthOffset_, param_.cardImageHeight_));
		}

		const std::string label = entry.name.empty() ? entry.projectFile.stem().string() : entry.name;
		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardNameOffsetY_));
		ImGui::TextWrapped("%s", label.c_str());
		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardVersionOffsetY_));
		ImGui::TextDisabled("%s", entry.engineVersion.empty() ? "Unknown" : entry.engineVersion.c_str());

		ImGui::SetCursorScreenPos(cardMax);
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						テンプレート詳細
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawTemplateDetails() {
		const TemplateInfo& item = GetSelectedTemplate();

		ImGui::TextUnformatted("Template");
		ImGui::Separator();

		const float previewWidth = ImGui::GetContentRegionAvail().x;
		const float previewHeight = (std::min)(param_.templatePreviewMaxHeight_, previewWidth * param_.templatePreviewAspect_);
		if(genericIcon_) {
			ImGui::Image(genericIcon_, ImVec2(previewWidth, previewHeight));
		} else {
			ImGui::Dummy(ImVec2(previewWidth, previewHeight));
		}

		ImGui::Spacing();
		ImGui::TextUnformatted(item.name);
		ImGui::TextWrapped("%s", item.description);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextDisabled("Language");
		ImGui::SameLine(param_.templateValueOffsetX_);
		ImGui::TextUnformatted("C++");

		ImGui::TextDisabled("Target");
		ImGui::SameLine(param_.templateValueOffsetX_);
		ImGui::TextUnformatted("Desktop");

		ImGui::TextDisabled("Startup Scene");
		ImGui::SameLine(param_.templateValueOffsetX_);
		ImGui::TextUnformatted(item.startupScene[0] == '\0' ? "None" : item.startupScene);
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						新規プロジェクト作成欄
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawNewProject(Calyx::ProjectInfo& outProject, bool& selected) {
		ImGui::BeginGroup();

		const float labelWidth	= param_.labelWidth_;
		const float buttonWidth	= param_.buttonWidth_;
		const float browseSize	= param_.browseSize_;
		const float fullWidth	= ImGui::GetContentRegionAvail().x;
		const float inputWidth	= (std::max)(param_.minInputWidth_, fullWidth - labelWidth - browseSize - ImGui::GetStyle().ItemSpacing.x * param_.inputSpacingCount_);

		ImGui::TextUnformatted("Project Location");
		ImGui::SameLine(labelWidth);
		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##ProjectDirectory", newProjectDirectory_.data(), newProjectDirectory_.size());
		ImGui::SameLine();
		if(folderIcon_) {
			if(ImGui::ImageButton("##BrowseProjectLocation", folderIcon_, ImVec2(param_.folderIconSize_, param_.folderIconSize_))) {
				IGFD::FileDialogConfig config;
				config.path = newProjectDirectory_.data();
				ImGuiFileDialog::Instance()->OpenDialog("SelectProjectDirectory", "Select Project Location", nullptr, config);
			}
		} else if(ImGui::Button("...", ImVec2(param_.fallbackBrowseButtonSize_.x, param_.fallbackBrowseButtonSize_.y))) {
			IGFD::FileDialogConfig config;
			config.path = newProjectDirectory_.data();
			ImGuiFileDialog::Instance()->OpenDialog("SelectProjectDirectory", "Select Project Location", nullptr, config);
		}

		ImGui::TextUnformatted("Project Name");
		ImGui::SameLine(labelWidth);
		ImGui::SetNextItemWidth((std::max)(param_.minInputWidth_, fullWidth - labelWidth - buttonWidth - ImGui::GetStyle().ItemSpacing.x * param_.inputSpacingCount_));
		ImGui::InputText("##ProjectName", newProjectName_.data(), newProjectName_.size());

		ImGui::SameLine();
		if(ImGui::Button("Create", ImVec2(buttonWidth, param_.createButtonHeight_))) {
			selected = CreateProjectFromSelectedTemplate(outProject);
		}

		ImGui::EndGroup();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						既存プロジェクトを開くダイアログ
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawOpenProjectDialog(Calyx::ProjectInfo& outProject, bool& selected) {
		if(ImGuiFileDialog::Instance()->Display("OpenCalyxProject")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				selected = LoadProject(ImGuiFileDialog::Instance()->GetFilePathName(), outProject);
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						プロジェクト作成先フォルダの選択
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawLocationDialog() {
		if(ImGuiFileDialog::Instance()->Display("SelectProjectDirectory")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				CopyText(newProjectDirectory_, ImGuiFileDialog::Instance()->GetCurrentPath());
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						アイコン読み込み
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::LoadIcons() {
		if(genericIcon_ && folderIcon_) {
			return;
		}

		auto* assetManager = CalyxEngine::AssetManager::GetInstance();
		if(!assetManager || !assetManager->GetTextureManager()) {
			return;
		}

		auto* textureManager = assetManager->GetTextureManager();
		if(!genericIcon_) {
			genericIcon_ = reinterpret_cast<void*>(textureManager->LoadTexture("UI/Tool/AssetPanel/generic.png").ptr);
		}
		if(!folderIcon_) {
			folderIcon_ = reinterpret_cast<void*>(textureManager->LoadTexture("UI/Tool/AssetPanel/folder.png").ptr);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						プロジェクト読み込み
	////////////////////////////////////////////////////////////////////////////////////////////
	bool ProjectBrowser::LoadProject(const std::filesystem::path& path, Calyx::ProjectInfo& outProject) {
		Calyx::ProjectInfo project;
		if(!Calyx::LoadProjectFile(path, project)) {
			statusMessage_ = "Project could not be opened.";
			return false;
		}

		outProject = std::move(project);
		statusMessage_.clear();
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						Blankプロジェクト作成
	////////////////////////////////////////////////////////////////////////////////////////////
	bool ProjectBrowser::CreateProjectFromSelectedTemplate(Calyx::ProjectInfo& outProject) {
		if(IsBlank(newProjectName_.data()) || IsBlank(newProjectDirectory_.data())) {
			statusMessage_ = "Project name and directory are required.";
			return false;
		}

		const TemplateInfo& selectedTemplate = GetSelectedTemplate();

		Calyx::ProjectInfo project;
		project.name			 = newProjectName_.data();
		project.engineVersion	 = kDefaultEngineVersion;
		project.rootDirectory	 = std::filesystem::path(newProjectDirectory_.data()) / project.name;
		project.projectFile		 = project.rootDirectory / (project.name + ".calyxproj");
		project.assetDirectory	 = "Resources/Assets";
		project.sourceDirectory	 = selectedTemplate.sourceDirectory;
		project.generatedDirectory = "Generated";
		project.startupScene		 = selectedTemplate.startupScene;
		project.gameModule		 = std::filesystem::path("Generated") / "Outputs" / "Debug" / (project.name + ".dll");
		project.gameModuleDebug	 = std::filesystem::path("Generated") / "Outputs" / "Debug" / (project.name + ".dll");
		project.gameModuleDevelop = std::filesystem::path("Generated") / "Outputs" / "Develop" / (project.name + ".dll");
		project.gameModuleRelease = std::filesystem::path("Generated") / "Outputs" / "Release" / (project.name + ".dll");
		project.templateName		 = selectedTemplate.name;

		if(!Calyx::CreateProject(project)) {
			statusMessage_ = "Project could not be created.";
			return false;
		}
		if(!CreateGameWorkspace(project, selectedTemplate)) {
			statusMessage_ = "Visual Studio project files could not be created.";
			return false;
		}

		return LoadProject(project.projectFile, outProject);
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						選択中テンプレートの取得
	////////////////////////////////////////////////////////////////////////////////////////////
	const ProjectBrowser::TemplateInfo& ProjectBrowser::GetSelectedTemplate() const {
		for(const TemplateInfo& item : kTemplates) {
			if(item.type == selectedTemplate_) {
				return item;
			}
		}
		return kTemplates[0];
	}

} // namespace CalyxEditor
