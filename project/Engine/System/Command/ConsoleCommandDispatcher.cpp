#include "ConsoleCommandDispatcher.h"

// engine
#include <CalyxEngine/Project.h>
#include <Engine/Application/System/EngineMode.h>
#include <Engine/Application/System/PlaySession.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Editor/LevelEditor.h>
#include <Engine/Foundation/Log/EngineLogger.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/System/SceneManager.h>

// c++
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>

namespace CalyxEngine {

	namespace {
		ConsoleCommandResult ErrorResult(const std::string& message) {
			return {false, {{ConsoleOutputLevel::Error, message}}};
		}

		ConsoleCommandResult InfoResult(const std::string& message) {
			return {false, {{ConsoleOutputLevel::Info, message}}};
		}

		std::shared_ptr<SceneObject> ResolveSceneObject(
			const ConsoleCommandContext& context,
			const std::string& identifier,
			std::string& outError) {
			outError.clear();
			if(!context.sceneManager) {
				outError = "SceneManager is unavailable.";
				return nullptr;
			}
			auto* scene = context.sceneManager->GetCurrentSceneContext();
			if(!scene || !scene->GetObjectLibrary()) {
				outError = "No current scene object library.";
				return nullptr;
			}

			const Guid guid = Guid::FromString(identifier);
			if(guid.isValid()) {
				auto object = scene->GetObjectLibrary()->Find(guid);
				if(object) return object;
			}

			std::shared_ptr<SceneObject> match;
			for(const auto& object : scene->GetObjectLibrary()->GetAllObjectsShared()) {
				if(!object || object->GetName() != identifier) continue;
				if(match) {
					outError = "Multiple objects have the name '" + identifier + "'. Use a GUID instead.";
					return nullptr;
				}
				match = object;
			}
			if(!match) outError = "Object not found: " + identifier;
			return match;
		}
	}

	ConsoleCommandDispatcher::ConsoleCommandDispatcher() {
		RegisterBuiltInCommands();
	}

	ConsoleCommandResult ConsoleCommandDispatcher::Execute(
		const std::string& commandLine,
		const ConsoleCommandContext& context) const {
		const std::size_t firstCharacter = commandLine.find_first_not_of(" \t\r\n");
		if(firstCharacter == std::string::npos) return {};
		if(commandLine[firstCharacter] != '/') {
			return ErrorResult("Commands must start with '/'. Example: /help");
		}

		// UI用の接頭辞を除外し、Registryには正規化可能なコマンド名だけを渡す。
		const std::string commandBody = commandLine.substr(firstCharacter + 1);
		std::vector<std::string> tokens;
		std::string parseError;
		if(!Tokenize(commandBody, tokens, parseError)) {
			return ErrorResult(parseError);
		}
		if(tokens.empty()) return ErrorResult("Enter a command after '/'. Example: /help");

		const std::string commandName = tokens.front();
		std::vector<std::string> arguments(tokens.begin() + 1, tokens.end());
		EngineLogger::GetInstance().Add(LogLevel::Trace, LogCategory::Command, "Terminal command: " + commandLine, "ConsoleCommandDispatcher");
		return ConsoleCommandRegistry::GetInstance().Execute(commandName, arguments, context);
	}

	std::vector<ConsoleCommandDefinition> ConsoleCommandDispatcher::GetCommandSuggestions(const std::string& prefix) const {
		std::string normalizedPrefix = prefix;
		std::transform(normalizedPrefix.begin(), normalizedPrefix.end(), normalizedPrefix.begin(), [](unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});

		std::vector<ConsoleCommandDefinition> suggestions;
		for(const auto& command : ConsoleCommandRegistry::GetInstance().GetCommands()) {
			if(command.name.starts_with(normalizedPrefix)) suggestions.push_back(command);
		}
		return suggestions;
	}

	void ConsoleCommandDispatcher::RegisterBuiltInCommands() {
		auto& registry = ConsoleCommandRegistry::GetInstance();

		registry.Register({
			"clear",
			"Clear the terminal history.",
			"/clear",
			[](const ConsoleCommandContext&, const std::vector<std::string>& arguments) {
				if(!arguments.empty()) return ErrorResult("Usage: /clear");
				ConsoleCommandResult result;
				result.clearRequested = true;
				return result;
			}});

		registry.Register({
			"help",
			"List commands or show command details.",
			"/help [command]",
			[](const ConsoleCommandContext&, const std::vector<std::string>& arguments) {
				if(arguments.size() > 1) return ErrorResult("Usage: /help [command]");

				ConsoleCommandResult result;
				auto& commandRegistry = ConsoleCommandRegistry::GetInstance();
				if(arguments.empty()) {
					result.output.push_back({ConsoleOutputLevel::Info, "Available commands:"});
					for(const auto& command : commandRegistry.GetCommands()) {
						result.output.push_back({ConsoleOutputLevel::Info, "  /" + command.name + " - " + command.description});
					}
					result.output.push_back({ConsoleOutputLevel::Info, "Use '/help <command>' for usage details."});
					return result;
				}

				const auto command = commandRegistry.Find(arguments.front());
				if(!command) return ErrorResult("Command not found: " + arguments.front());
				result.output.push_back({ConsoleOutputLevel::Info, "/" + command->name + " - " + command->description});
				result.output.push_back({ConsoleOutputLevel::Info, "Usage: " + command->usage});
				return result;
			}});

		registry.Register({
			"scene.current",
			"Show the current scene name and path.",
			"/scene.current",
			[](const ConsoleCommandContext& context, const std::vector<std::string>& arguments) {
				if(!arguments.empty()) return ErrorResult("Usage: /scene.current");
				if(!context.sceneManager) return ErrorResult("SceneManager is unavailable.");
				auto* scene = context.sceneManager->GetCurrentSceneContext();
				if(!scene) return ErrorResult("No current scene context.");
				const std::string path = scene->GetScenePath().empty() ? "<unsaved>" : scene->GetScenePath();
				return InfoResult("Scene: " + scene->GetSceneName() + " | Path: " + path);
			}});

		registry.Register({
			"scene.save",
			"Save the current scene.",
			"/scene.save",
			[](const ConsoleCommandContext& context, const std::vector<std::string>& arguments) {
				if(!arguments.empty()) return ErrorResult("Usage: /scene.save");
				if(!context.sceneManager) return ErrorResult("SceneManager is unavailable.");
				auto* scene = context.sceneManager->GetCurrentSceneContext();
				if(!scene) return ErrorResult("No current scene context.");

				std::string path = scene->GetScenePath();
				if(path.empty()) {
					path = Calyx::ResolveAssetPath(std::filesystem::path("Scenes") / (scene->GetSceneName() + ".scene")).generic_string();
				}
				if(!SceneSerializer::Save(*scene, path)) return ErrorResult("Scene save failed: " + path);
				return InfoResult("Scene saved: " + path);
			}});

		registry.Register({
			"scene.open",
			"Open a scene file in the editor.",
			"/scene.open <path>",
			[](const ConsoleCommandContext& context, const std::vector<std::string>& arguments) {
				if(arguments.size() != 1) return ErrorResult("Usage: /scene.open <path>");
				if(!context.levelEditor) return ErrorResult("LevelEditor is unavailable.");
				if(!context.levelEditor->OpenSceneFromEditor(arguments.front())) {
					return ErrorResult("Scene open failed: " + arguments.front());
				}
				return InfoResult("Scene opened: " + arguments.front());
			}});

		registry.Register({
			"object.list",
			"List objects in the current scene.",
			"/object.list",
			[](const ConsoleCommandContext& context, const std::vector<std::string>& arguments) {
				if(!arguments.empty()) return ErrorResult("Usage: /object.list");
				if(!context.sceneManager) return ErrorResult("SceneManager is unavailable.");
				auto* scene = context.sceneManager->GetCurrentSceneContext();
				if(!scene || !scene->GetObjectLibrary()) return ErrorResult("No current scene object library.");

				const auto objects = scene->GetObjectLibrary()->GetAllObjectsShared();
				ConsoleCommandResult result;
				result.output.push_back({ConsoleOutputLevel::Info, "Objects: " + std::to_string(objects.size())});
				for(const auto& object : objects) {
					if(!object) continue;
					result.output.push_back({
						ConsoleOutputLevel::Info,
						"  " + object->GetName() + " [" + std::string(object->GetTypeName()) + "] " + object->GetGuid().ToString()});
				}
				return result;
			}});

		registry.Register({
			"object.select",
			"Select an object by exact name or GUID.",
			"/object.select <name|guid>",
			[](const ConsoleCommandContext& context, const std::vector<std::string>& arguments) {
				if(arguments.size() != 1) return ErrorResult("Usage: /object.select <name|guid>");
				if(!context.levelEditor) return ErrorResult("LevelEditor is unavailable.");
				std::string error;
				auto object = ResolveSceneObject(context, arguments.front(), error);
				if(!object) return ErrorResult(error);
				context.levelEditor->SetSelectedObject(object);
				return InfoResult("Object selected: " + object->GetName());
			}});

		registry.Register({
			"object.delete",
			"Delete an object by exact name or GUID. The operation supports Undo.",
			"/object.delete <name|guid>",
			[](const ConsoleCommandContext& context, const std::vector<std::string>& arguments) {
				if(arguments.size() != 1) return ErrorResult("Usage: /object.delete <name|guid>");
				if(!context.levelEditor) return ErrorResult("LevelEditor is unavailable.");
				std::string error;
				auto object = ResolveSceneObject(context, arguments.front(), error);
				if(!object) return ErrorResult(error);
				const std::string objectName = object->GetName();
				context.levelEditor->DeleteObject(object);
				return InfoResult("Object deleted: " + objectName);
			}});

		registry.Register({
			"asset.rescan",
			"Rescan the current project's asset directory.",
			"/asset.rescan",
			[](const ConsoleCommandContext&, const std::vector<std::string>& arguments) {
				if(!arguments.empty()) return ErrorResult("Usage: /asset.rescan");
				auto* database = AssetDatabase::GetInstance();
				if(!database) return ErrorResult("AssetDatabase is unavailable.");
				database->Scan();
				return InfoResult("Asset rescan completed. Records=" + std::to_string(database->GetView().size()));
			}});

		registry.Register({
			"play.start",
			"Enter play mode.",
			"/play.start",
			[](const ConsoleCommandContext& context, const std::vector<std::string>& arguments) {
				if(!arguments.empty()) return ErrorResult("Usage: /play.start");
				if(!context.playSession) return ErrorResult("PlaySession is unavailable.");
				if(context.playSession->GetMode() != EngineMode::Editor) return ErrorResult("PlaySession is already active.");
				context.playSession->Enter();
				return InfoResult("Play mode entered.");
			}});

		registry.Register({
			"play.pause",
			"Pause or resume the active play session.",
			"/play.pause",
			[](const ConsoleCommandContext& context, const std::vector<std::string>& arguments) {
				if(!arguments.empty()) return ErrorResult("Usage: /play.pause");
				if(!context.playSession) return ErrorResult("PlaySession is unavailable.");
				const EngineMode mode = context.playSession->GetMode();
				if(mode != EngineMode::Playing && mode != EngineMode::Paused) return ErrorResult("PlaySession is not playing or paused.");
				context.playSession->TogglePause();
				return InfoResult(mode == EngineMode::Playing ? "Play session paused." : "Play session resumed.");
			}});

		registry.Register({
			"play.stop",
			"Stop the active play session.",
			"/play.stop",
			[](const ConsoleCommandContext& context, const std::vector<std::string>& arguments) {
				if(!arguments.empty()) return ErrorResult("Usage: /play.stop");
				if(!context.playSession) return ErrorResult("PlaySession is unavailable.");
				if(context.playSession->GetMode() == EngineMode::Editor) return ErrorResult("PlaySession is not active.");
				context.playSession->Exit();
				return InfoResult("Play mode exit requested.");
			}});
	}

	bool ConsoleCommandDispatcher::Tokenize(
		const std::string& commandLine,
		std::vector<std::string>& outTokens,
		std::string& outError) {
		outTokens.clear();
		outError.clear();
		std::string current;
		char quote = '\0';

		for(std::size_t index = 0; index < commandLine.size(); ++index) {
			const char character = commandLine[index];
			if(quote != '\0') {
				if(character == quote) {
					quote = '\0';
				} else if(character == '\\' && index + 1 < commandLine.size() &&
						  (commandLine[index + 1] == quote || commandLine[index + 1] == '\\')) {
					current.push_back(commandLine[++index]);
				} else {
					current.push_back(character);
				}
				continue;
			}

			if(character == '"' || character == '\'') {
				quote = character;
			} else if(std::isspace(static_cast<unsigned char>(character))) {
				if(!current.empty()) {
					outTokens.push_back(std::move(current));
					current.clear();
				}
			} else {
				current.push_back(character);
			}
		}

		if(quote != '\0') {
			outError = "Command parse error: unmatched quote.";
			return false;
		}
		if(!current.empty()) outTokens.push_back(std::move(current));
		return true;
	}

} // namespace CalyxEngine
