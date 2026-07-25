#pragma once
#include <Engine/Graphics/Pipeline/Shader/ShaderManager.h>
#include <Engine/Graphics/Pipeline/BlendMode/BlendMode.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <dxcapi.h>
#include <memory>

/*-----------------------------------------------------------------------------------------
 * PipelineState
 * - DirectX12のグラフィックスパイプライン状態を管理するクラス
 * - PSO、ルートシグネチャ、コンパイル済みシェーダーの保持を担当
 * - 描画命令の登録は管理しない
 *---------------------------------------------------------------------------------------*/
class PipelineState {
public:
	/** \brief パイプライン生成に必要な機能を保持する \param device 共有所有するDirectX12デバイス \param shaderManager 共有所有するシェーダー管理機能 */
	PipelineState(Microsoft::WRL::ComPtr<ID3D12Device> device, std::shared_ptr<ShaderManager> shaderManager);
	/** \brief 保持するDirectX12パイプラインリソースを解放する */
	~PipelineState();
	/** \brief シェーダーと設定記述からパイプラインを生成する \param vsPath 頂点シェーダーパス \param psPath ピクセルシェーダーパス \param rootSignatureDesc ルートシグネチャ構成 \param psoDesc パイプライン基本構成 \param blendMode ブレンド方式 \return 生成成功時はtrue */
	bool Initialize(const std::wstring& vsPath, const std::wstring& psPath, const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, const BlendMode& blendMode);
	/** \brief 描画設定用のPSOとルートシグネチャを取得する \return 非所有ポインタをまとめたパイプライン設定 */
	PipelineSet GetPipelineSet() const { return {pipelineState_.Get(), rootSignature_.Get()}; }
	/** \brief Pipeline State Objectを取得する \return PSOを所有するComPtr */
	const Microsoft::WRL::ComPtr<ID3D12PipelineState>& GetPipelineState() const { return pipelineState_; }
	/** \brief ルートシグネチャを取得する \return ルートシグネチャを所有するComPtr */
	const Microsoft::WRL::ComPtr<ID3D12RootSignature>& GetRootSignature() const { return rootSignature_; }
private:
	Microsoft::WRL::ComPtr<ID3D12Device> device_; //< パイプライン生成に使用するDirectX12デバイスを共有所有
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_; //< 描画時に設定するルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_; //< 描画時に設定するPipeline State Object
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShader_; //< コンパイル済み頂点シェーダー
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShader_; //< コンパイル済みピクセルシェーダー
	std::shared_ptr<ShaderManager> shaderManager_; //< シェーダー管理機能を共有所有
};