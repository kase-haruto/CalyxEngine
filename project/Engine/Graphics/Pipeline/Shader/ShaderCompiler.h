#pragma once
#include <cassert>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <string>
#include <wrl.h>

#pragma comment(lib,"dxcompiler.lib")

class ShaderCompiler{
public://メンバ関数
	ShaderCompiler() = default;
	~ShaderCompiler();

	void InitializeDXC();

	void LoadHLSL(const std::wstring& filePath, const wchar_t* profile);

	 Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
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
};
