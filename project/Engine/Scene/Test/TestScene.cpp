#include <Engine/Scene/Test/TestScene.h>

#include <Engine/Application/System/Environment.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Utility/Converter/ConvertString.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <sstream>
#include <windows.h>

namespace {
	std::wstring ConvertUtf8ToWide(const std::string& text) {
		if(text.empty()) {
			return {};
		}

		const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
		if(length <= 0) {
			return {};
		}

		std::wstring result(static_cast<size_t>(length), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
		return result;
	}

	void WriteUtf8(const std::string& text) {
		const std::wstring wideText = ConvertUtf8ToWide(text);
		if(wideText.empty()) {
			return;
		}

		DWORD written = 0;
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wideText.c_str(), static_cast<DWORD>(wideText.size()), &written, nullptr);
	}

	size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
		const size_t totalSize = size * nmemb;
		if(output && contents) {
			output->append(static_cast<char*>(contents), totalSize);
		}
		return totalSize;
	}

	std::string JsonValueToString(const nlohmann::json& value) {
		if(value.is_string()) {
			return value.get<std::string>();
		}
		if(value.is_number_integer()) {
			return std::to_string(value.get<int64_t>());
		}
		if(value.is_number_unsigned()) {
			return std::to_string(value.get<uint64_t>());
		}
		if(value.is_number_float()) {
			return std::to_string(value.get<double>());
		}
		if(value.is_boolean()) {
			return value.get<bool>() ? "true" : "false";
		}
		return value.dump();
	}

	std::string PickJsonText(const nlohmann::json& object, std::initializer_list<const char*> keys) {
		for(const char* key : keys) {
			if(object.contains(key) && !object.at(key).is_null()) {
				return JsonValueToString(object.at(key));
			}
		}
		return {};
	}
}

TestScene::TestScene() {
	BaseScene::SetSceneName("TestScene");
}

void TestScene::LoadAssets() {}

void TestScene::Initialize() {
	sceneContext_->Initialize();
	sceneContext_->SetSceneName("TestScene");

	BaseScene::Initialize();

	std::string scenePath = "Resources/Assets/Scenes/test.scene";
	SceneSerializer::Load(*sceneContext_, scenePath);
	sceneContext_->SetScenePath(scenePath);

	LoadAssets();
}

void TestScene::Update([[maybe_unused]] float dt) {
#ifdef _DEBUG
	DrawWebApiDebugWindow();
#endif

	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void TestScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget* rt) {
	BaseScene::Draw(cmdList, psoService, rt);
}

void TestScene::CleanUp() {
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

void TestScene::FetchFacultyFromWebApi() {
	apiStatus_ = "requesting...";
	apiRawResponse_.clear();
	apiDisplayText_.clear();

	CURLcode globalResult = curl_global_init(CURL_GLOBAL_ALL);
	if(globalResult != CURLE_OK) {
		apiStatus_ = std::string("curl_global_init failed: ") + curl_easy_strerror(globalResult);
		WriteUtf8(apiStatus_ + "\n");
		return;
	}

	CURL* curl = curl_easy_init();
	if(!curl) {
		apiStatus_ = "curl_easy_init failed";
		curl_global_cleanup();
		WriteUtf8(apiStatus_ + "\n");
		return;
	}

	const std::string endpoint = BuildFacultyEndpoint();
	curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &apiRawResponse_);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	CURLcode result = curl_easy_perform(curl);
	if(result == CURLE_OK) {
		apiStatus_ = "request succeeded";
		apiDisplayText_ = BuildFacultyDisplayText(apiRawResponse_);
		WriteUtf8("response:\n" + apiRawResponse_ + "\n");
	} else {
		apiStatus_ = std::string("request failed: ") + curl_easy_strerror(result);
		WriteUtf8(apiStatus_ + "\n");
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

std::string TestScene::BuildFacultyEndpoint() const {
	return "http://localhost:3000/faculties";
}

std::string TestScene::BuildFacultyDisplayText(const std::string& response) const {
	if(response.empty()) {
		return "response is empty";
	}

	try {
		const nlohmann::json data = nlohmann::json::parse(response);
		const nlohmann::json* faculty = nullptr;

		if(data.is_array()) {
			auto it = std::find_if(data.begin(), data.end(), [this](const nlohmann::json& item) {
				return item.is_object() &&
					   item.contains("id") &&
					   item.at("id").is_number_integer() &&
					   item.at("id").get<int>() == facultyId_;
			});

			if(it == data.end() && !data.empty()) {
				it = data.begin();
			}
			if(it != data.end()) {
				faculty = &(*it);
			}
		} else if(data.is_object()) {
			faculty = &data;
		}

		if(!faculty || !faculty->is_object()) {
			return data.dump(2);
		}

		std::ostringstream stream;
		const std::string id = PickJsonText(*faculty, {"id", "facultyId"});
		const std::string name = PickJsonText(*faculty, {"name", "facultyName", "title"});
		const std::string description = PickJsonText(*faculty, {"description", "desc", "detail"});

		if(!id.empty()) {
			stream << "id: " << id << '\n';
		}
		if(!name.empty()) {
			stream << "name: " << name << '\n';
		}
		if(!description.empty()) {
			stream << "description: " << description << '\n';
		}

		const std::string text = stream.str();
		return text.empty() ? faculty->dump(2) : text;
	} catch(const std::exception& e) {
		return std::string("JSON parse failed: ") + e.what();
	}
}

void TestScene::DrawWebApiDebugWindow() {
#ifdef _DEBUG
	ImGui::Begin("Web API Debug");
	ImGui::InputInt("Faculty ID", &facultyId_);
	if(facultyId_ < 1) {
		facultyId_ = 1;
	}

	if(ImGui::Button("GET /faculties")) {
		FetchFacultyFromWebApi();
	}

	const std::string stateText = "State: " + apiStatus_;
	ImGui::TextUnformatted(stateText.c_str());
	if(!apiDisplayText_.empty()) {
		ImGui::SeparatorText("Parsed JSON");
		ImGui::TextWrapped("%s", apiDisplayText_.c_str());
	}
	if(!apiRawResponse_.empty()) {
		ImGui::SeparatorText("Raw Response");
		ImGui::TextWrapped("%s", apiRawResponse_.c_str());
	}
	ImGui::End();
#endif
}
