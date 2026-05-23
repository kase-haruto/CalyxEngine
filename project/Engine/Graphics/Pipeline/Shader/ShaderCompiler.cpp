#include "ShaderCompiler.h"
#include <Engine/Foundation/Debug/CxAssert.h>
#include <Engine/Foundation/Utility/Converter/ConvertString.h>
#include <Engine/Foundation/Utility/FileSystem/FileSystemHelper.h>

// c++
#include <algorithm>
#include <format>

namespace {
	CalyxEngine::ShaderResourceKind ToShaderResourceKind(D3D_SHADER_INPUT_TYPE type) {
		switch(type) {
		case D3D_SIT_CBUFFER:
			return CalyxEngine::ShaderResourceKind::CBuffer;
		case D3D_SIT_TEXTURE:
			return CalyxEngine::ShaderResourceKind::Texture;
		case D3D_SIT_SAMPLER:
			return CalyxEngine::ShaderResourceKind::Sampler;
		case D3D_SIT_UAV_RWTYPED:
			return CalyxEngine::ShaderResourceKind::UAV;
		case D3D_SIT_STRUCTURED:
		case D3D_SIT_BYTEADDRESS:
			return CalyxEngine::ShaderResourceKind::StructuredBuffer;
		case D3D_SIT_RTACCELERATIONSTRUCTURE:
			return CalyxEngine::ShaderResourceKind::RaytracingAccelerationStructure;
		default:
			return CalyxEngine::ShaderResourceKind::Unknown;
		}
	}
}

void ShaderCompiler::InitializeDXC() {
	// DXC Compilerを初期化
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	if(FAILED(hr)) {
		Log("Failed to create DXC Utils");
		CX_CHECK(false && "Failed to create DXC Utils", "Assertion failed");
	}

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	if(FAILED(hr)) {
		Log("Failed to create DXC Compiler");
		CX_CHECK(false && "Failed to create DXC Compiler", "Assertion failed");
	}

	// Include handlerを設定
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandle);
	if(FAILED(hr)) {
		Log("Failed to create default include handler");
		CX_CHECK(false && "Failed to create default include handler", "Assertion failed");
	}
}

void ShaderCompiler::InitializeShaderCache(const std::wstring& shaderRootDir) {
	shaderRootPath = shaderRootDir;
	shaderCache = FileSystemHelper::BuildFileCacheW(shaderRootDir);
	Log(ConvertString(std::format(L"========== InitializeShaderCache called ==========\n")));
	Log(ConvertString(std::format(L"Shader Root: {}\n", shaderRootDir)));
	Log(ConvertString(std::format(L"Shader cache initialized: {} files found\n", shaderCache.size())));

	//========================================================================
	//	デバッグ：全キャッシュ内容をログ出力
	//========================================================================
	int count = 0;
	for (const auto& pair : shaderCache) {
		Log(ConvertString(std::format(L"  [{}] {}\n", count + 1, pair.first)));
		count++;
	}
	Log(ConvertString(std::format(L"========== End InitializeShaderCache ==========\n")));
}

void ShaderCompiler::LoadHLSL(const std::wstring& filePath,[[maybe_unused]] const wchar_t* profile) {
	//========================================================================
	//	これからシェーダをコンパイルする旨をログに出す
	//========================================================================
	Log(ConvertString(std::format(L"Begin CompileShader, path: {}, profile: {}\n",filePath,profile)));

	//========================================================================
	//	シェーダーファイルのフルパスを構築
	//========================================================================

	//========================================================================
	//	ファイル読み込み
	//========================================================================
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(),nullptr,shaderSource.GetAddressOf());
	if(FAILED(hr)) {
		Log(ConvertString(std::format(L"Failed to load HLSL file: {}\n",filePath)));
		CX_CHECK(false && "Failed to load HLSL file", "Assertion failed");
	}

	//========================================================================
	//	読み込んだファイル内容を設定
	//========================================================================
	shaderSourceBuffer.Ptr      = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size     = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::CompileShader(
	const std::wstring& filePath,
	const wchar_t*      profile) {
	//========================================================================
	//	HLSLファイルを読み込む
	//========================================================================
	LoadHLSL(filePath,profile);

	//========================================================================
	//	コンパイルする
	//========================================================================
	Compile(filePath,profile);

	//========================================================================
	//	警告・エラーが出てないか確認
	//========================================================================
	CheckNoError();

	//========================================================================
	//	コンパイル結果を返す
	//========================================================================
	return GetCompileResult(filePath,profile);
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::CompileSource(
	const std::wstring& sourceName,
	const std::string& source,
	const wchar_t* profile) {
	ShaderCompileResult result = TryCompileSource(sourceName, source, profile);
	if(!result.succeeded) {
		if(!result.errors.empty()) Log(result.errors.c_str());
		CX_CHECK(false && "DXC CompileSource failed", "Assertion failed");
	}
	return result.bytecode;
}

ShaderCompileResult ShaderCompiler::TryCompileSource(
	const std::wstring& sourceName,
	const std::string& source,
	const wchar_t* profile) {
	Log(ConvertString(std::format(L"Begin CompileSource, name: {}, profile: {}\n", sourceName, profile)));

	ShaderCompileResult result;

	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = source.data();
	sourceBuffer.Size = source.size();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	LPCWSTR arguments[] = {
		sourceName.c_str(),
		L"-E", L"main",
		L"-T", profile,
		L"-Zi",
#ifdef _DEBUG
		L"-Qembed_debug",
		L"-Od",
#endif
		L"-Zpr",
	};

	HRESULT hr = dxcCompiler->Compile(
		&sourceBuffer,
		arguments,
		_countof(arguments),
		includeHandle.Get(),
		IID_PPV_ARGS(&shaderResult));

	if(FAILED(hr)) {
		result.errors = "Failed to compile generated HLSL source (DXC invocation failed).";
		Log(result.errors.c_str());
		return result;
	}

	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(shaderError.GetAddressOf()), nullptr);
	if(SUCCEEDED(hr) && shaderError != nullptr && shaderError->GetStringLength() != 0) {
		result.errors = shaderError->GetStringPointer();
		result.hasWarnings = result.errors.find("warning") != std::string::npos;
		Log(result.errors.c_str());
	}

	HRESULT status = S_OK;
	hr = shaderResult->GetStatus(&status);
	if(FAILED(hr) || FAILED(status)) {
		if(result.errors.empty()) result.errors = "Generated HLSL compile failed without an error message.";
		return result;
	}

	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(result.bytecode.GetAddressOf()), nullptr);
	if(FAILED(hr) || !result.bytecode) {
		result.errors = "Failed to get generated shader bytecode.";
		Log(result.errors.c_str());
		return result;
	}

	result.succeeded = true;
	Log(ConvertString(std::format(L"Compile Succeeded, path: {}, profile: {}\n", sourceName, profile)));
	return result;
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::CompileShaderByName(
	const std::wstring& shaderName,
	const wchar_t* profile) {

	// 1. デフォルト設定
	if (shaderRootPath.empty()) {
		shaderRootPath = L"Resources/shaders";
	}

	// 2. パス正規化
	std::wstring normalizedName = shaderName;
	for (auto& c : normalizedName) { if (c == L'/') c = L'\\'; }

	// 3. キャッシュ確認
	auto it = shaderCache.find(normalizedName);
	if (it != shaderCache.end()) {
		return CompileShader(it->second, profile);
	}

	// 4. 検索
	std::filesystem::path root(shaderRootPath);
	std::wstring fullPath;

	// 直接のパス確認
	std::filesystem::path directPath = root / normalizedName;
	if (std::filesystem::exists(directPath)) {
		fullPath = directPath.wstring();
	} else {
		// 再帰検索 (core/ などのサブフォルダを掘る)
		if (std::filesystem::exists(root)) {
			for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
				if (entry.is_regular_file()) {
					if (_wcsicmp(entry.path().filename().c_str(), normalizedName.c_str()) == 0) {
						fullPath = entry.path().wstring();
						break;
					}
				}
			}
		}
	}

	if (fullPath.empty()) {
		// 見つからない場合はエラーログを出して止める
		Log(ConvertString(std::format(L"[Error] Shader not found in {}: {}\n", shaderRootPath, shaderName)));
		CX_CHECK(false && "Shader file not found", "Assertion failed");
		return nullptr;
	}

	shaderCache[normalizedName] = fullPath;
	return CompileShader(fullPath, profile);
}

CalyxEngine::ShaderReflectionInfo ShaderCompiler::ReflectShader(
	const std::wstring& filePath,
	const wchar_t* profile) {
	LoadHLSL(filePath, profile);
	Compile(filePath, profile);
	CheckNoError();

	CalyxEngine::ShaderReflectionInfo info;
	info.entryPoint = "main";
	info.profile = ConvertString(std::wstring(profile));

	Microsoft::WRL::ComPtr<IDxcBlob> reflectionBlob = nullptr;
	HRESULT hr = shaderResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionBlob.GetAddressOf()), nullptr);
	if(FAILED(hr) || !reflectionBlob) {
		Log("Failed to get shader reflection data");
		return info;
	}

	DxcBuffer reflectionBuffer{};
	reflectionBuffer.Ptr = reflectionBlob->GetBufferPointer();
	reflectionBuffer.Size = reflectionBlob->GetBufferSize();
	reflectionBuffer.Encoding = DXC_CP_UTF8;

	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection = nullptr;
	hr = dxcUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(reflection.GetAddressOf()));
	if(FAILED(hr) || !reflection) {
		Log("Failed to create D3D12 shader reflection");
		return info;
	}

	D3D12_SHADER_DESC shaderDesc{};
	hr = reflection->GetDesc(&shaderDesc);
	if(FAILED(hr)) {
		Log("Failed to read shader reflection desc");
		return info;
	}

	for(UINT i = 0; i < shaderDesc.BoundResources; ++i) {
		D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
		if(FAILED(reflection->GetResourceBindingDesc(i, &bindDesc))) continue;

		CalyxEngine::ShaderResourceBinding binding;
		binding.name = bindDesc.Name ? bindDesc.Name : "";
		binding.kind = ToShaderResourceKind(bindDesc.Type);
		binding.bindPoint = bindDesc.BindPoint;
		binding.bindCount = bindDesc.BindCount;
		binding.space = bindDesc.Space;
		info.resources.push_back(std::move(binding));
	}

	for(UINT i = 0; i < shaderDesc.ConstantBuffers; ++i) {
		ID3D12ShaderReflectionConstantBuffer* cbuffer = reflection->GetConstantBufferByIndex(i);
		if(!cbuffer) continue;

		D3D12_SHADER_BUFFER_DESC bufferDesc{};
		if(FAILED(cbuffer->GetDesc(&bufferDesc))) continue;

		CalyxEngine::ShaderCBufferLayout layout;
		layout.name = bufferDesc.Name ? bufferDesc.Name : "";
		layout.size = bufferDesc.Size;

		auto bindingIt = std::find_if(info.resources.begin(), info.resources.end(), [&layout](const CalyxEngine::ShaderResourceBinding& binding) {
			return binding.kind == CalyxEngine::ShaderResourceKind::CBuffer && binding.name == layout.name;
		});
		if(bindingIt != info.resources.end()) {
			layout.bindPoint = bindingIt->bindPoint;
			layout.space = bindingIt->space;
		}

		for(UINT v = 0; v < bufferDesc.Variables; ++v) {
			ID3D12ShaderReflectionVariable* variable = cbuffer->GetVariableByIndex(v);
			if(!variable) continue;

			D3D12_SHADER_VARIABLE_DESC variableDesc{};
			if(FAILED(variable->GetDesc(&variableDesc))) continue;

			layout.variables.push_back({
				variableDesc.Name ? variableDesc.Name : "",
				variableDesc.StartOffset,
				variableDesc.Size});
		}

		info.cbuffers.push_back(std::move(layout));
	}

	return info;
}

void ShaderCompiler::Compile(const std::wstring& filePath,
							 const wchar_t*      profile) {
	//========================================================================
	//	コンパイルオプションの設定
	//========================================================================
	LPCWSTR arguments[] = {
			filePath.c_str(), //コンパイル対象のhlslファイル名
			L"-E",L"main",    //エントリーポイントの指定。基本的にmain以外には市内
			L"-T",profile,    //ShaderProfileの設定
			L"-Zi",
#ifdef _DEBUG
		L"-Qembed_debug",	//デバッグ用の情報を埋め込む
		L"-Od",						//最適化を外しておく
#endif
			L"-Zpr", //メモリレイアウトは行優先
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

	if(FAILED(hr)) {
		Log("Failed to compile HLSL shader (DXC invocation failed)");
		CX_CHECK(false && "DXC Compile failed", "Assertion failed");
	}
}

void ShaderCompiler::CheckNoError() {
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	HRESULT                              hr          = shaderResult->GetOutput(
		DXC_OUT_ERRORS, IID_PPV_ARGS(shaderError.GetAddressOf()),nullptr);

	if(SUCCEEDED(hr) && shaderError != nullptr && shaderError->GetStringLength() != 0) {
		std::string msg = shaderError->GetStringPointer();

		// warning だけならログだけにする
		if(msg.find("warning") != std::string::npos) { Log(msg.c_str()); } else {
			Log(msg.c_str());
			CX_CHECK(false && "Shader compile error", "Assertion failed");
		}
	}
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::GetCompileResult(const std::wstring& filePath,
																  const wchar_t*      profile) {
	//========================================================================
	//	コンパイル結果（実行用バイナリ部分）を取得
	//========================================================================
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	HRESULT                          hr         = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderBlob.GetAddressOf()),nullptr);
	if(FAILED(hr)) {
		Log("Failed to get shader bytecode");
		CX_CHECK(false && "Failed to get shader bytecode", "Assertion failed");
	}

	//========================================================================
	//	成功ログ
	//========================================================================
	Log(ConvertString(std::format(L"Compile Succeeded, path: {}, profile: {}\n",filePath,profile)));

	//========================================================================
	//	返却
	//========================================================================
	return shaderBlob;
}

ShaderCompiler::~ShaderCompiler() {}
