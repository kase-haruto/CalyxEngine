#include "ShaderManager.h"
#include <Engine/Foundation/Debug/CxAssert.h>

// lib
#include <Engine/Foundation/Utility/Converter/ConvertString.h>
#include <Engine/Foundation/Utility/FileSystem/FileSystemHelper.h>

/* c++ */
#include<format>
#include<filesystem>

ShaderManager::~ShaderManager() {
	dxcUtils.Reset();
	dxcCompiler.Reset();
	includeHandler.Reset();
}

void ShaderManager::InitializeDXC() {
	// DXC Compilerを初期化
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// Include handlerを設定
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");
}

void ShaderManager::InitializeShaderCache(const std::wstring& shaderRootDir) {
	shaderRootPath = shaderRootDir;
	shaderCache = FileSystemHelper::BuildFileCacheW(shaderRootDir);
	Log(ConvertString(std::format(L"Shader cache initialized: {} files found\n", shaderCache.size())));
}

IDxcBlob* ShaderManager::CompileShader(const std::wstring& filePath, const wchar_t* profile) {

	HRESULT hr;
	//==============================
	//HLSLファイルの読み込み
	//==============================
	//これからシェーダをコンパイルする旨をログに出す
	Log(ConvertString(std::format(L"Begin CompileShader,path:{},profile:{}\n", filePath, profile)));
	//hlslファイルを読む
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	//読めなければ止める
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");
	//読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	//UTF8の文字コードであることを通知
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;


	//==============================
	//Compileする
	//==============================
	LPCWSTR arguments[] = {
		filePath.c_str(),			//コンパイル対象のhlslファイル名
		L"-E",L"main",				//エントリーポイントの指定。基本的にmain以外には市内
		L"-T",profile,				//ShaderProfileの設定
		L"-Zi",L"-Qembed_debug",	//デバッグ用の情報を埋め込む
		L"-Od",						//最適化を外しておく
		L"-Zpr",					//メモリレイアウトは行優先
	};

	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
	//実際にshaderをコンパイルする
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,//読み込んだオプション
		arguments,//コンパイルオプション
		_countof(arguments),//コンパイルオプションの数
		includeHandler.Get(),//includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)//コンパイル結果
	);
	//コンパイルエラーではなくdxcが起動できないなど致命的な状況
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	//==============================
	//警告、エラーが出ていないか確認
	//==============================
	// 警告、エラーが出ていないか確認
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);

	if (SUCCEEDED(hr) && shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		CX_CHECK(false, "Assertion failed"); // エラーがあった場合は止める
	}


	//==============================
	//Compile結果を受け取る
	//==============================

	//コンパイル結果から実行用のバイナリ部分を取得
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");
	//成功したログを出す
	Log(ConvertString(std::format(L"Compile Succeded,path:{},profile:{}\n", filePath, profile)));

	//実行用のバイナリ返却
	return shaderBlob.Detach();
}

IDxcBlob* ShaderManager::CompileShaderByName(const std::wstring& shaderName, const wchar_t* profile) {
	//========================================================================
	//	相対パスの場合はルートディレクトリと結合
	//========================================================================
	std::wstring fullPath;

	// パスセパレータを正規化
	std::wstring normalizedName = shaderName;
	for (auto& c : normalizedName) {
		if (c == L'/') {
			c = L'\\';
		}
	}

	// シェーダールートからの相対パス（例: postEffects\exampleEffect.hlsl）を処理
	if (normalizedName.find(L'\\') != std::wstring::npos) {
		// サブディレクトリを含む相対パスの場合
		std::wstring relativePath = shaderRootPath + L"\\" + normalizedName;

		// パスを正規化
		std::filesystem::path p(relativePath);
		fullPath = p.wstring();

		// ファイルが存在するか確認
		if (!std::filesystem::exists(fullPath)) {
			Log(ConvertString(std::format(L"Failed to find shader file: {} (resolved to: {})\n", shaderName, fullPath)));
			CX_CHECK(false && "Shader file not found", "Assertion failed");
			return nullptr;
		}
	} else {
		// ファイル名のみの場合はキャッシュから検索
		auto it = shaderCache.find(normalizedName);
		if (it != shaderCache.end()) {
			fullPath = it->second;
		} else {
			// キャッシュにない場合は再帰検索を試みる
			auto result = FileSystemHelper::FindFileRecursiveW(shaderRootPath, normalizedName);
			if (result.has_value()) {
				fullPath = result.value();
				// キャッシュに追加
				shaderCache[normalizedName] = fullPath;
			} else {
				Log(ConvertString(std::format(L"Failed to find shader file: {}\n", shaderName)));
				CX_CHECK(false && "Shader file not found", "Assertion failed");
				return nullptr;
			}
		}
	}

	//========================================================================
	//	フルパスからコンパイル
	//========================================================================
	return CompileShader(fullPath, profile);
}

void ShaderManager::RegisterPipelineShaders(const PipelineType& type, const std::wstring& vsPath, const std::wstring& psPath) {
	//========================================================================
	//	PipelineType とシェーダーパスの対応を登録
	//========================================================================
	pipelineShaderMap[static_cast<int>(type)] = {vsPath, psPath};
	Log(ConvertString(std::format(L"Registered pipeline shaders: type={}, VS={}, PS={}\n",
		static_cast<int>(type), vsPath, psPath)));
}

bool ShaderManager::LoadShaderAuto(const PipelineType& type) {
	//========================================================================
	//	登録済みのシェーダーパスから自動ロード
	//========================================================================
	auto it = pipelineShaderMap.find(static_cast<int>(type));
	if (it == pipelineShaderMap.end()) {
		Log(ConvertString(std::format(L"Pipeline type {} is not registered\n", static_cast<int>(type))));
		return false;
	}

	const auto& vsPath = it->second.first;
	const auto& psPath = it->second.second;

	return LoadShader(type, vsPath, psPath);
}

bool ShaderManager::LoadShader(const PipelineType& type, const std::wstring& vsPath, const std::wstring& psPath) {
	//========================================================================
	//	ファイル名またはパスからシェーダーをコンパイル
	//========================================================================
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShader = CompileShaderByName(vsPath, L"vs_6_5");
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShader = CompileShaderByName(psPath, L"ps_6_5");

	if (!vertexShader || !pixelShader) {
		return false;
	}

	vertexShaders[type] = vertexShader;
	pixelShaders[type] = pixelShader;
	return true;
}

const Microsoft::WRL::ComPtr<IDxcBlob>& ShaderManager::GetVertexShader(const PipelineType& type) const {
	auto it = vertexShaders.find(type);
	if (it != vertexShaders.end()) {
		return it->second;
	}
	static Microsoft::WRL::ComPtr<IDxcBlob> nullShader;
	return nullShader;
}

const Microsoft::WRL::ComPtr<IDxcBlob>& ShaderManager::GetPixelShader(const PipelineType& type) const {
	auto it = pixelShaders.find(type);
	if (it != pixelShaders.end()) {
		return it->second;
	}
	static Microsoft::WRL::ComPtr<IDxcBlob> nullShader;
	return nullShader;
}
