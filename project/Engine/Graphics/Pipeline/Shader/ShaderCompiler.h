#pragma once
#include <Engine/Foundation/Debug/CxAssert.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3d12shader.h>
#include <dxcapi.h>
#include <Engine/Graphics/MaterialGraph/ShaderReflectionInfo.h>
#include <string>
#include <wrl.h>
#include <unordered_map>

#pragma comment(lib,"dxcompiler.lib")

/**
 * @brief ShaderCompileResultに関するデータを保持する構造体です。
 */
struct ShaderCompileResult {
	Microsoft::WRL::ComPtr<IDxcBlob> bytecode;
	std::string errors;
	bool succeeded = false;
	bool hasWarnings = false;
};

/**
 * @brief ShaderCompilerの機能を提供するクラスです。
 */
class ShaderCompiler{
public://メンバ関数
	ShaderCompiler() = default;
	~ShaderCompiler();

	void InitializeDXC();

	/// <summary>
	/// シェーダーディレクトリ（Resources/shaders など）をスキャンし、ファイルキャッシュを構築
	/// </summary>
	void InitializeShaderCache(const std::wstring& shaderRootDir);

	void LoadHLSL(const std::wstring& filePath, const wchar_t* profile);

	 Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		const std::wstring& filePath,
		const wchar_t* profile);

	 Microsoft::WRL::ComPtr<IDxcBlob> CompileSource(
		const std::wstring& sourceName,
		const std::string& source,
		const wchar_t* profile);

	 ShaderCompileResult TryCompileSource(
		const std::wstring& sourceName,
		const std::string& source,
		const wchar_t* profile);

	 /// <summary>
	 /// ファイル名またはサブディレクトリを含む相対パスからシェーダーをコンパイル
	 /// 例: "Fragment.VS.hlsl" や "Skybox/Skybox.VS.hlsl"
	 /// </summary>
	 Microsoft::WRL::ComPtr<IDxcBlob> CompileShaderByName(
		const std::wstring& shaderName,
		const wchar_t* profile);

	 CalyxEngine::ShaderReflectionInfo ReflectShader(
		const std::wstring& filePath,
		const wchar_t* profile);

	void Compile(const std::wstring& filePath,
				 const wchar_t* profile);
	void CheckNoError();

	 Microsoft::WRL::ComPtr<IDxcBlob> GetCompileResult(const std::wstring& filePath,
									  const wchar_t* profile);

public:
	// ComPtr 型のゲッター
	Microsoft::WRL::ComPtr<IDxcUtils> GetDxcUtils() const { return dxcUtils; }
	Microsoft::WRL::ComPtr<IDxcCompiler3> GetDxcCompiler() const { return dxcCompiler; }
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> GetIncludeHandler() const { return includeHandle; }

private://メンバ変数
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils		= nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler	= nullptr;
	Microsoft::WRL::ComPtr<IDxcResult>	  shaderResult	= nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandle = nullptr;
	DxcBuffer shaderSourceBuffer {};
	Microsoft::WRL::ComPtr<IDxcBlobEncoding>   shaderSource = nullptr;

	// シェーダーファイルキャッシュ: ファイル名 -> フルパス
	std::unordered_map<std::wstring, std::wstring> shaderCache;
	std::wstring shaderRootPath;
};
