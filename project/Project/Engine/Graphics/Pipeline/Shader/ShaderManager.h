#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Pipeline/PipelineType.h>

//lib
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <unordered_map>
#include <dxcapi.h>
#include <cassert>

using Microsoft::WRL::ComPtr;

class ShaderManager{
public:
    //===================================================================*/
	//          public functions
    //===================================================================*/
    ShaderManager(){}
    ~ShaderManager();

    void InitializeDXC();
    IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile);

    bool LoadShader(const PipelineType& type, const std::wstring& vsPath, const std::wstring& psPath);
    const ComPtr<IDxcBlob>& GetVertexShader(const PipelineType& type) const;
    const ComPtr<IDxcBlob>& GetPixelShader(const PipelineType& type) const;

private:
    //===================================================================*/
	//          private variables
    //===================================================================*/

    std::unordered_map<PipelineType, ComPtr<IDxcBlob>> vertexShaders;
    std::unordered_map<PipelineType, ComPtr<IDxcBlob>> pixelShaders;

    // DXC関連のメンバ変数
    ComPtr<IDxcUtils> dxcUtils;
    ComPtr<IDxcCompiler3> dxcCompiler;
    ComPtr<IDxcIncludeHandler> includeHandler;
};

