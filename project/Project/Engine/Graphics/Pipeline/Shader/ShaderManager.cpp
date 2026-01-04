#include "ShaderManager.h"

// lib
#include <Engine/Foundation/Utility/ConvertString/ConvertString.h>

/* c++ */
#include<format>

ShaderManager::~ShaderManager() {
	dxcUtils.Reset();
	dxcCompiler.Reset();
	includeHandler.Reset();
}

void ShaderManager::InitializeDXC() {
	// DXC Compilerを初期化
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	// Include handlerを設定
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));
}

IDxcBlob* ShaderManager:://CompilerするShaderファイルへのパス
CompileShader(const std::wstring& filePath, const wchar_t* profile) {

	HRESULT hr;
	//==============================
	//HLSLファイルの読み込み
	//==============================
	//これからシェーダをコンパイルする旨をログに出す
	Log(ConvertString(std::format(L"Begin CompileShader,path:{},profile:{}\n", filePath, profile)));
	//hlslファイルを読む
	ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	//読めなければ止める
	assert(SUCCEEDED(hr));
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

	ComPtr<IDxcResult> shaderResult = nullptr;
	//実際にshaderをコンパイルする
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,//読み込んだオプション
		arguments,//コンパイルオプション
		_countof(arguments),//コンパイルオプションの数
		includeHandler.Get(),//includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)//コンパイル結果
	);
	//コンパイルエラーではなくdxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));

	//==============================
	//警告、エラーが出ていないか確認
	//==============================
	// 警告、エラーが出ていないか確認
	ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);

	if (SUCCEEDED(hr) && shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		assert(false); // エラーがあった場合は止める
	}


	//==============================
	//Compile結果を受け取る
	//==============================

	//コンパイル結果から実行用のバイナリ部分を取得
	ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	//成功したログを出す
	Log(ConvertString(std::format(L"Compile Succeded,path:{},profile:{}\n", filePath, profile)));

	//実行用のバイナリ返却
	return shaderBlob.Detach();
}

bool ShaderManager::LoadShader(const PipelineType& type, const std::wstring& vsPath, const std::wstring& psPath) {
	//ファイルパスをワイド文字列として結合
	//ファイルパスをワイド文字列として結合
	ComPtr<IDxcBlob> vertexShader = CompileShader(L"Resources/shaders/" + vsPath, L"vs_6_0");
	ComPtr<IDxcBlob> pixelShader = CompileShader(L"Resources/shaders/" + psPath, L"ps_6_0");

	vertexShaders[type] = vertexShader;
	pixelShaders[type] = pixelShader;
	return true;
}

const ComPtr<IDxcBlob>& ShaderManager::GetVertexShader(const PipelineType& type) const {
	auto it = vertexShaders.find(type);
	if (it != vertexShaders.end()) {
		return it->second;
	}
	assert("Vertex shader not found: ");
	static Microsoft::WRL::ComPtr<IDxcBlob> nullShader;
	return nullShader;
}

const ComPtr<IDxcBlob>& ShaderManager::GetPixelShader(const PipelineType& type) const {
	auto it = pixelShaders.find(type);
	if (it != pixelShaders.end()) {
		return it->second;
	}
	assert("Pixel shader not found: ");
	static Microsoft::WRL::ComPtr<IDxcBlob> nullShader;
	return nullShader;
}