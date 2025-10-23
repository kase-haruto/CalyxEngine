#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Pipeline/PipelineType.h>

// lib
#include <cassert>
#include <d3d12.h>
#include <dxcapi.h>
#include <string>
#include <unordered_map>
#include <wrl.h>

class ShaderManager {
public:
	//===================================================================*/
	//          public functions
	//===================================================================*/
	ShaderManager() {}
	~ShaderManager();

	void	  InitializeDXC();
	IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile);

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
};
