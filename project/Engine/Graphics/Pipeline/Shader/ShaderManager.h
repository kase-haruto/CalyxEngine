#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Pipeline/PipelineType.h>

// lib
#include <Engine/Foundation/Debug/CxAssert.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <string>
#include <unordered_map>
#include <wrl.h>

/**
 * @brief ShaderManagerの機能を提供するクラスです。
 */
class ShaderManager {
public:
	//===================================================================*/
	//          public functions
	//===================================================================*/
	ShaderManager() {}
	~ShaderManager();

	void	  InitializeDXC();

	/// <summary>
	/// シェーダーディレクトリ（Resources/shaders など）をスキャンし、ファイルキャッシュを構築
	/// </summary>
	void InitializeShaderCache(const std::wstring& shaderRootDir);

	IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile);

	/// <summary>
	/// ファイル名または相対パスからシェーダーをコンパイル
	/// </summary>
	IDxcBlob* CompileShaderByName(const std::wstring& shaderName, const wchar_t* profile);

	/// <summary>
	/// PipelineType に対応するシェーダーパスを登録
	/// </summary>
	void RegisterPipelineShaders(const PipelineType& type, const std::wstring& vsPath, const std::wstring& psPath);

	/// <summary>
	/// 登録済みの PipelineType からシェーダーを自動ロード
	/// </summary>
	bool LoadShaderAuto(const PipelineType& type);

	bool									LoadShader(const PipelineType& type, const std::wstring& vsPath, const std::wstring& psPath);
	const Microsoft::WRL::ComPtr<IDxcBlob>& GetVertexShader(const PipelineType& type) const;
	const Microsoft::WRL::ComPtr<IDxcBlob>& GetPixelShader(const PipelineType& type) const;

private:
	//===================================================================*/
	//          private variables
	//===================================================================*/

	std::unordered_map<PipelineType, Microsoft::WRL::ComPtr<IDxcBlob>> vertexShaders;
	std::unordered_map<PipelineType, Microsoft::WRL::ComPtr<IDxcBlob>> pixelShaders;

	// DXC関連のメンバ変数
	Microsoft::WRL::ComPtr<IDxcUtils>		   dxcUtils;
	Microsoft::WRL::ComPtr<IDxcCompiler3>	   dxcCompiler;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;

	// シェーダーキャッシュ
	std::unordered_map<std::wstring, std::wstring> shaderCache;
	std::wstring shaderRootPath;

	// PipelineType ごとのシェーダーパス登録
	std::unordered_map<int, std::pair<std::wstring, std::wstring>> pipelineShaderMap;
};
