#include "ShaderCompiler.h"
#include <Engine/Foundation/Utility/ConvertString/ConvertString.h>

// c++
#include <format>

void ShaderCompiler::InitializeDXC() {
	// DXC Compilerを初期化
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	if (FAILED(hr)) {
		Log("Failed to create DXC Utils");
		assert(false && "Failed to create DXC Utils");
	}

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	if (FAILED(hr)) {
		Log("Failed to create DXC Compiler");
		assert(false && "Failed to create DXC Compiler");
	}

	// Include handlerを設定
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandle);
	if (FAILED(hr)) {
		Log("Failed to create default include handler");
		assert(false && "Failed to create default include handler");
	}
}

void ShaderCompiler::LoadHLSL(const std::wstring& filePath,[[maybe_unused]] const wchar_t* profile) {
	//========================================================================
	//	これからシェーダをコンパイルする旨をログに出す
	//========================================================================
	Log(ConvertString(std::format(L"Begin CompileShader, path: {}, profile: {}\n", filePath, profile)));

	//========================================================================
	//	シェーダーファイルのフルパスを構築
	//========================================================================

	//========================================================================
	//	ファイル読み込み
	//========================================================================
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, shaderSource.GetAddressOf());
	if (FAILED(hr)) {
		Log(ConvertString(std::format(L"Failed to load HLSL file: {}\n", filePath)));
		assert(false && "Failed to load HLSL file");
	}


	//========================================================================
	//	読み込んだファイル内容を設定
	//========================================================================
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;
}

ComPtr<IDxcBlob> ShaderCompiler::CompileShader(
	const std::wstring& filePath,
	const wchar_t* profile) {
	//========================================================================
	//	HLSLファイルを読み込む
	//========================================================================
	LoadHLSL(filePath, profile);

	//========================================================================
	//	コンパイルする
	//========================================================================
	Compile(filePath, profile);

	//========================================================================
	//	警告・エラーが出てないか確認
	//========================================================================
	CheckNoError();

	//========================================================================
	//	コンパイル結果を返す
	//========================================================================
	return GetCompileResult(filePath, profile);
}

void ShaderCompiler::Compile(const std::wstring& filePath,
							 const wchar_t* profile) {
	//========================================================================
	//	コンパイルオプションの設定
	//========================================================================
	LPCWSTR arguments[] = {
		filePath.c_str(),			//コンパイル対象のhlslファイル名
		L"-E",L"main",				//エントリーポイントの指定。基本的にmain以外には市内
		L"-T",profile,				//ShaderProfileの設定
		L"-Zi",L"-Qembed_debug",	//デバッグ用の情報を埋め込む
		L"-Od",						//最適化を外しておく
		L"-Zpr",					//メモリレイアウトは行優先
	};

	//========================================================================
	//	シェーダをコンパイル
	//========================================================================
	HRESULT hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandle.Get(),
		IID_PPV_ARGS(&shaderResult)
	);

	if (FAILED(hr)) {
		Log("Failed to compile HLSL shader (DXC invocation failed)");
		assert(false && "DXC Compile failed");
	}
}

void ShaderCompiler::CheckNoError() {
	ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	HRESULT hr = shaderResult->GetOutput(
		DXC_OUT_ERRORS, IID_PPV_ARGS(shaderError.GetAddressOf()), nullptr);

	if (SUCCEEDED(hr) && shaderError != nullptr && shaderError->GetStringLength() != 0) {
		std::string msg = shaderError->GetStringPointer();

		// warning だけならログだけにする
		if (msg.find("warning") != std::string::npos) {
			Log(msg.c_str());
		} else {
			Log(msg.c_str());
			assert(false && "Shader compile error");
		}
	}
}

ComPtr<IDxcBlob> ShaderCompiler::GetCompileResult(const std::wstring& filePath,
												  const wchar_t* profile) {
	//========================================================================
	//	コンパイル結果（実行用バイナリ部分）を取得
	//========================================================================
	ComPtr<IDxcBlob> shaderBlob = nullptr;
	HRESULT hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderBlob.GetAddressOf()), nullptr);
	if (FAILED(hr)) {
		Log("Failed to get shader bytecode");
		assert(false && "Failed to get shader bytecode");
	}

	//========================================================================
	//	成功ログ
	//========================================================================
	Log(ConvertString(std::format(L"Compile Succeeded, path: {}, profile: {}\n", filePath, profile)));

	//========================================================================
	//	返却
	//========================================================================
	return shaderBlob;
}

ShaderCompiler::~ShaderCompiler() {}
