#include "CalyxCore.h"
/////////////////////////////////////////////////////////////////////////////////////////
//  include space
/////////////////////////////////////////////////////////////////////////////////////////

// engine
#include <Engine/Application/System/Environment.h>
#include <Engine/Foundation/Audio/Audio.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Utility/Func/DxFunc.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
#include <Engine/Graphics/Pipeline/BlendMode/BlendMode.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Graphics/RenderTarget/SwapChainRT/SwapChainRenderTarget.h>
#include <Engine/System/Command/Manager/CommandManager.h>

// manager
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/Model/ModelManager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/PostProcess/Manager/PostEffectManager.h>

// editor

// lib
#include "Engine/Scene/System/SceneManager.h"
#include <Engine/Editor/PickingPass.h>

#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

namespace CalyxEngine {

	/////////////////////////////////////////////////////////////////////////////////////////
	//  静的変数初期化
	/////////////////////////////////////////////////////////////////////////////////////////
	HINSTANCE CalyxCore::hInstance_ = nullptr;
	HWND	  CalyxCore::hwnd_		= nullptr;

	/////////////////////////////////////////////////////////////////////////////////////////
	//  コンストラクタ
	/////////////////////////////////////////////////////////////////////////////////////////
	CalyxCore::CalyxCore() {}

	/////////////////////////////////////////////////////////////////////////////////////////
	//  初期化処理
	/////////////////////////////////////////////////////////////////////////////////////////
	void CalyxCore::Initialize(HINSTANCE hInstance, int32_t clientWidth, int32_t clientHeight, const std::string _windowTitle) {
		winApp_	   = std::make_unique<WinApp>(clientWidth, clientHeight, _windowTitle);
		hInstance_ = hInstance;
		hwnd_	   = winApp_->GetHWND();

		dxCore_ = std::make_unique<CalyxGraphics::DxCore>();
		dxCore_->Initialize(winApp_.get(), clientWidth, clientHeight);

		ComPtr<ID3D12Device> device = dxCore_->GetDevice();

		// インプットの初期化
		CalyxFoundation::Input::Initialize();

		// audioの初期化
		Audio::Initialize();

		DescriptorAllocator::Initialize(dxCore_->GetDevice().Get());
		DescriptorAllocator::CreateHeap(DescriptorUsage::CbvSrvUav, {});
		DescriptorAllocator::CreateHeap(DescriptorUsage::Rtv, {.maxDescriptors = 256, .shaderVisible = false});
		DescriptorAllocator::CreateHeap(DescriptorUsage::Dsv, {.maxDescriptors = 64, .shaderVisible = false});

		// 管理クラスの初期化
		shaderManager_		  = std::make_shared<ShaderManager>();
		pipelineStateManager_ = std::make_unique<PipelineStateManager>(device, shaderManager_);

		// パイプラインを設定
		CreatePipelines();

		GraphicsGroup::GetInstance()->Initialize(dxCore_.get(), pipelineStateManager_.get());

		imguiManager_ = std::make_unique<ImGuiManager>();
		imguiManager_->Initialize(winApp_.get(), dxCore_.get());

		// リサイズコールバックの設定
		winApp_->SetResizeCallback([this](int width, int height) {
			dxCore_->Resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		});

		// srvの先頭をimguiが使用するためそのあとに初期化
		dxCore_->RendererInitialize(clientWidth, clientHeight);
		// カメラの生成
		PrimitiveDrawer::GetInstance()->Initialize();

		// モデル管理クラスの初期化(インスタンス生成)
		ModelManager::Initialize();
		ModelManager::StartUpLoad();

		// textureManagerの初期化
		TextureManager::GetInstance()->Initialize(imguiManager_.get());
		// スタート時に読み込み
		TextureManager::GetInstance()->StartUpLoad();

		auto* db = AssetDatabase::GetInstance();
		// 実行ファイルの場所からプロジェクトルートを解決して Assets へ
		const auto exe		  = std::filesystem::current_path();
		const auto assetsRoot = exe / "Resources" / "Assets";
		db->Initialize(assetsRoot);
	}

	void CalyxCore::InitializePostProcess(PipelineService* service) {
		/////////////////////////////////////////////////////////////////////////////////////////
		/*                     postProcessの初期化                                             */
		/////////////////////////////////////////////////////////////////////////////////////////
		//=========================================================
		// PostProcessManager の初期化
		//=========================================================
		PostEffectManager::Get()->Initialize(service, /*enableAll=*/false);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//  フレーム開始処理
	/////////////////////////////////////////////////////////////////////////////////////////
	void CalyxCore::BeginFrame() {
		// インプットの更新
		CalyxFoundation::Input::Update();

		auto* clock = ClockManager::GetInstance();
		clock->Update();

		PostEffectManager::Get()->Update(clock->GetDeltaTime());

		// ImGui受付開始
		imguiManager_->Begin();

		ModelManager::GetInstance()->ProcessLoadingTasks();
		// オフスクリーンレンダーターゲットの開始
		dxCore_->PreDrawOffscreen();

		EditorUpdate();
	}
	/////////////////////////////////////////////////////////////////////////////////////////
	//  フレーム終了処理
	/////////////////////////////////////////////////////////////////////////////////////////
	void CalyxCore::EndFrame() {
		imguiManager_->End();
		imguiManager_->Draw();

		dxCore_->PostDraw();
	}

	void CalyxCore::ExecutePostEffect(const PipelineService* service) {
		auto* cmd = dxCore_->GetCommandList().Get();

		auto* backBuffer   = dxCore_->GetRenderTargetCollection().Get("BackBuffer");
		auto* offscreenRes = dxCore_->GetRenderTargetCollection().Get("Offscreen")->GetResource();
		auto* postOutput   = dxCore_->GetRenderTargetCollection().Get("PostEffectOutput");
		auto* debugRT	   = dxCore_->GetRenderTargetCollection().Get("DebugView");

		if(auto* scTarget = dynamic_cast<SwapChainRenderTarget*>(backBuffer)) {
			scTarget->SetBufferIndex(dxCore_->GetSwapChain().GetCurrentBackBufferIndex());
		}

		PostEffectManager::Get()->Execute(cmd, offscreenRes, postOutput, dxCore_.get());

		postOutput->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pEngineUICore_->SetMainViewportTexture(postOutput->GetSRV().ptr);

		debugRT->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pEngineUICore_->SetDebugViewportTexture(debugRT->GetSRV().ptr);
		if(pEngineUICore_ && pEngineUICore_->GetLevelEditor()) {
			if(auto* sceneMgr = pEngineUICore_->GetLevelEditor()->GetSceneManager()) {
				if(auto* pickingPass = sceneMgr->GetPickingPass()) {
					auto* pickingColor = pickingPass->GetColor();
					if(pickingColor) {
						D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
							pickingColor,
							D3D12_RESOURCE_STATE_RENDER_TARGET,
							D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
						cmd->ResourceBarrier(1, &barrier);

						pEngineUICore_->SetPickingViewportTexture(pickingPass->GetSrv().ptr);
					}
				}
			}
		}

		PipelineSet pipelineSet = service->GetPipelineSet(PipelineTag::PostProcess::CopyImage);
		DrawTextureToRenderTarget(cmd, postOutput->GetSRV(), backBuffer,
								  pipelineSet.pipelineState, pipelineSet.rootSignature);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//  Editorの更新
	/////////////////////////////////////////////////////////////////////////////////////////
	void CalyxCore::EditorUpdate() {
		CalyxFoundation::Input* input	   = CalyxFoundation::Input::GetInstance();
		CommandManager*			cmdManager = CommandManager::GetInstance();
		// コマンド
		if(input->PushKey(DIK_LCONTROL) && input->TriggerKey(DIK_Z)) {
			if(input->PushKey(DIK_LSHIFT))
				cmdManager->Redo();
			else
				cmdManager->Undo();
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//  Editorの描画
	/////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////
	//  終了処理
	/////////////////////////////////////////////////////////////////////////////////////////
	void CalyxCore::Finalize() {

		// imgui終了処理
		imguiManager_->Finalize();
		// textureの終了処理
		TextureManager::GetInstance()->Finalize();
		// モデルマネージャーの開放
		ModelManager::GetInstance()->Finalize();
		PrimitiveDrawer::GetInstance()->Finalize();
		// カメラの開放
		// pipelineの終了処理
		pipelineStateManager_->Finalize();
		DescriptorAllocator::Finalize();
		CalyxFoundation::Input::Finalize();
		Audio::Finalize();
		// ウィンドウの破棄
		winApp_->TerminateGameWindow();
	}

	void CalyxCore::InitializeEditor() {
		/////////////////////////////////////////////////////////////////////////////////////////
		/*                     editorの初期化と追加                                              */
		/////////////////////////////////////////////////////////////////////////////////////////
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	// プロセスメッセージ
	/////////////////////////////////////////////////////////////////////////////////////////
	int CalyxCore::ProcessMessage() { return winApp_->ProcessMessage() ? 1 : 0; }

	//=============================================================================================================
	//              Pipelineの作成
	//=============================================================================================================
	void CalyxCore::CreatePipelines() {
		shaderManager_->InitializeDXC();
		LinePipeline();
		EffectPipeline();
		SkyBoxPipeline();
	}

	void CalyxCore::LinePipeline() {
		// InputLayoutの設定
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
		inputElementDescs[0].SemanticName			  = "POSITION";
		inputElementDescs[0].SemanticIndex			  = 0;
		inputElementDescs[0].Format					  = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs[0].AlignedByteOffset		  = D3D12_APPEND_ALIGNED_ELEMENT;

		inputElementDescs[1].SemanticName	   = "COLOR";
		inputElementDescs[1].SemanticIndex	   = 0;
		inputElementDescs[1].Format			   = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs;
		inputLayoutDesc.NumElements		   = _countof(inputElementDescs);

		// BlendStateの設定
		BlendMode blendMode = BlendMode::NORMAL;

		// RasterizerStateの設定
		D3D12_RASTERIZER_DESC rasterizeDesc{};
		rasterizeDesc.CullMode				= D3D12_CULL_MODE_NONE;		 // カリングを無効化
		rasterizeDesc.FillMode				= D3D12_FILL_MODE_WIREFRAME; // 線描画の場合はこちら
		rasterizeDesc.FrontCounterClockwise = FALSE;
		rasterizeDesc.DepthClipEnable		= TRUE;

		// DepthStencilStateの設定
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable	= true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc		= D3D12_COMPARISON_FUNC_LESS_EQUAL;

		D3D12_DEPTH_STENCIL_DESC depthDesc = {};
		depthDesc.DepthEnable			   = TRUE;
		depthDesc.DepthWriteMask		   = D3D12_DEPTH_WRITE_MASK_ZERO; // 書き込みを無効にする
		depthDesc.DepthFunc				   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthDesc.StencilEnable			   = FALSE;

		// シェーダの読み込み
		if(!shaderManager_->LoadShader(Line, L"Fragment.VS.hlsl", L"Fragment.PS.hlsl")) {
			return;
		}

		// RootSignatureの設定
		D3D12_ROOT_PARAMETER rootParameters[2] = {};

		// wvp/world
		rootParameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		// カメラ
		rootParameters[1].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].Descriptor.ShaderRegister = 1;

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Flags						= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.pParameters				= rootParameters;
		rootSignatureDesc.NumParameters				= _countof(rootParameters);

		// RootSignatureの作成
		ComPtr<ID3DBlob> signatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		HRESULT			 hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if(FAILED(hr)) {
			if(errorBlob) {
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			}
			return;
		}

		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12Device>		device = dxCore_->GetDevice();
		hr								   = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		if(FAILED(hr)) {
			return;
		}

		// PSOの設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature					   = rootSignature.Get();
		psoDesc.InputLayout						   = inputLayoutDesc;
		psoDesc.VS								   = {shaderManager_->GetVertexShader(Line)->GetBufferPointer(), shaderManager_->GetVertexShader(Line)->GetBufferSize()};
		psoDesc.PS								   = {shaderManager_->GetPixelShader(Line)->GetBufferPointer(), shaderManager_->GetPixelShader(Line)->GetBufferSize()};
		psoDesc.RasterizerState					   = rasterizeDesc;
		psoDesc.DepthStencilState				   = depthStencilDesc;
		psoDesc.NumRenderTargets				   = 1;
		psoDesc.RTVFormats[0]					   = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.PrimitiveTopologyType			   = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; // 線描画用に設定
		psoDesc.SampleDesc.Count				   = 1;
		psoDesc.SampleMask						   = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.DSVFormat						   = DXGI_FORMAT_D32_FLOAT;

		// パイプラインステートオブジェクトの作成
		if(!pipelineStateManager_->CreatePipelineState(Line, L"Fragment.VS.hlsl", L"Fragment.PS.hlsl", rootSignatureDesc, psoDesc, blendMode)) {
			return;
		}
	}

	void CalyxCore::EffectPipeline() {
		// InputLayoutの設定
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
		inputElementDescs[0].SemanticName			  = "POSITION";
		inputElementDescs[0].SemanticIndex			  = 0;
		inputElementDescs[0].Format					  = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset		  = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[1].SemanticName			  = "TEXCOORD";
		inputElementDescs[1].SemanticIndex			  = 0;
		inputElementDescs[1].Format					  = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs[1].AlignedByteOffset		  = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[2].SemanticName			  = "COLOR";
		inputElementDescs[2].SemanticIndex			  = 0;
		inputElementDescs[2].Format					  = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[2].AlignedByteOffset		  = D3D12_APPEND_ALIGNED_ELEMENT;

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
		inputLayoutDesc.pInputElementDescs		= inputElementDescs;
		inputLayoutDesc.NumElements				= _countof(inputElementDescs);

		// rasterizerStateの設定
		D3D12_RASTERIZER_DESC rasterizeDesc = {};
		rasterizeDesc.CullMode				= D3D12_CULL_MODE_NONE;
		rasterizeDesc.FillMode				= D3D12_FILL_MODE_SOLID;

		// DepthStencilStateの設定
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable			  = true;
		depthStencilDesc.DepthWriteMask			  = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc				  = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		// シェーダの読み込み
		if(!shaderManager_->LoadShader(Effect, L"Effect.VS.hlsl", L"Effect.PS.hlsl")) {
			return;
		}

		// RootSignatureの設定
		D3D12_ROOT_PARAMETER   rootParameters[3]			 = {};
		D3D12_DESCRIPTOR_RANGE descriptorRange[1]			 = {};
		descriptorRange[0].BaseShaderRegister				 = 0;
		descriptorRange[0].NumDescriptors					 = 1;
		descriptorRange[0].RangeType						 = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// マテリアル
		rootParameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		// wvp/world
		rootParameters[1].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].Descriptor.ShaderRegister = 1;

		// テクスチャ
		rootParameters[2].ParameterType						  = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility					  = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].DescriptorTable.pDescriptorRanges	  = descriptorRange;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Flags						= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.pParameters				= rootParameters;
		rootSignatureDesc.NumParameters				= _countof(rootParameters);

		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
		staticSamplers[0].Filter					= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSamplers[0].AddressU					= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplers[0].AddressV					= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplers[0].AddressW					= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplers[0].ComparisonFunc			= D3D12_COMPARISON_FUNC_NEVER;
		staticSamplers[0].MaxLOD					= D3D12_FLOAT32_MAX;
		staticSamplers[0].ShaderRegister			= 0;
		staticSamplers[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_PIXEL;

		rootSignatureDesc.pStaticSamplers	= staticSamplers;
		rootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

		// rootsignatureの作成
		ComPtr<ID3DBlob> signatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		HRESULT			 hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if(FAILED(hr)) {
			if(errorBlob) {
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			}
			return;
		}

		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12Device>		device = dxCore_->GetDevice();
		hr								   = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		if(FAILED(hr)) {
			return;
		}

		// PSOの設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature					   = rootSignature.Get();
		psoDesc.InputLayout						   = inputLayoutDesc;
		psoDesc.VS								   = {shaderManager_->GetVertexShader(Effect)->GetBufferPointer(), shaderManager_->GetVertexShader(Effect)->GetBufferSize()};
		psoDesc.PS								   = {shaderManager_->GetPixelShader(Effect)->GetBufferPointer(), shaderManager_->GetPixelShader(Effect)->GetBufferSize()};
		psoDesc.RasterizerState					   = rasterizeDesc;
		psoDesc.DepthStencilState				   = depthStencilDesc;
		psoDesc.NumRenderTargets				   = 1;
		psoDesc.RTVFormats[0]					   = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.PrimitiveTopologyType			   = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.SampleDesc.Count				   = 1;
		psoDesc.SampleMask						   = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.DSVFormat						   = DXGI_FORMAT_D32_FLOAT;

		// shaderPath
		std::wstring vsPath = L"Effect.VS.hlsl";
		std::wstring psPath = L"Effect.PS.hlsl";

		for(int i = 0; i < static_cast<int>(BlendMode::kBlendModeCount); i++) {
			BlendMode mode = static_cast<BlendMode>(i);

			// mode に対応する処理を行う
			pipelineStateManager_->CreatePipelineState(
				PipelineType::Effect,
				vsPath,
				psPath,
				rootSignatureDesc,
				psoDesc,
				mode);
		}
	}

	void CalyxCore::SkyBoxPipeline() {
		// InputLayoutの設定
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
		inputElementDescs[0].SemanticName			  = "POSITION";
		inputElementDescs[0].SemanticIndex			  = 0;
		inputElementDescs[0].Format					  = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset		  = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[1].SemanticName			  = "TEXCOORD";
		inputElementDescs[1].SemanticIndex			  = 0;
		inputElementDescs[1].Format					  = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs[1].AlignedByteOffset		  = D3D12_APPEND_ALIGNED_ELEMENT;

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs;
		inputLayoutDesc.NumElements		   = _countof(inputElementDescs);

		// RasterizerStateの設定
		D3D12_RASTERIZER_DESC rasterizeDesc{};
		rasterizeDesc.CullMode = D3D12_CULL_MODE_NONE;
		rasterizeDesc.FillMode = D3D12_FILL_MODE_SOLID;

		// DepthStencilStateの設定
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable	= true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc		= D3D12_COMPARISON_FUNC_LESS_EQUAL;

		// シェーダの読み込み
		if(!shaderManager_->LoadShader(Skybox, L"Skybox.VS.hlsl", L"Skybox.PS.hlsl")) {
			// シェーダの読み込みに失敗した場合のエラーハンドリング
			return;
		}

		// RootSignatureの設定
		D3D12_ROOT_PARAMETER   rootParameters[3]			 = {};
		D3D12_DESCRIPTOR_RANGE descriptorRange[1]			 = {};
		descriptorRange[0].BaseShaderRegister				 = 0;
		descriptorRange[0].NumDescriptors					 = 1;
		descriptorRange[0].RangeType						 = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// transform
		rootParameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		// カメラ
		rootParameters[1].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[1].Descriptor.ShaderRegister = 1;

		// texture
		rootParameters[2].ParameterType						  = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility					  = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].DescriptorTable.pDescriptorRanges	  = descriptorRange;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Flags						= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.pParameters				= rootParameters;
		rootSignatureDesc.NumParameters				= _countof(rootParameters);

		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
		staticSamplers[0].Filter					= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSamplers[0].AddressU					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressV					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressW					= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].ComparisonFunc			= D3D12_COMPARISON_FUNC_NEVER;
		staticSamplers[0].MaxLOD					= D3D12_FLOAT32_MAX;
		staticSamplers[0].ShaderRegister			= 0;
		staticSamplers[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_PIXEL;

		rootSignatureDesc.pStaticSamplers	= staticSamplers;
		rootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

		// RootSignatureの作成
		ComPtr<ID3DBlob> signatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		HRESULT			 hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if(FAILED(hr)) {
			// RootSignatureのシリアライズに失敗した場合のエラーハンドリング
			if(errorBlob) {
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			}
			return;
		}

		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12Device>		device = dxCore_->GetDevice();
		hr								   = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		if(FAILED(hr)) {
			// RootSignatureの作成に失敗した場合のエラーハンドリング
			return;
		}

		// PSOの設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature					   = rootSignature.Get();
		psoDesc.InputLayout						   = inputLayoutDesc;
		psoDesc.VS								   = {shaderManager_->GetVertexShader(Skybox)->GetBufferPointer(), shaderManager_->GetVertexShader(Skybox)->GetBufferSize()};
		psoDesc.PS								   = {shaderManager_->GetPixelShader(Skybox)->GetBufferPointer(), shaderManager_->GetPixelShader(Skybox)->GetBufferSize()};
		psoDesc.RasterizerState					   = rasterizeDesc;
		psoDesc.DepthStencilState				   = depthStencilDesc;
		psoDesc.NumRenderTargets				   = 1;
		psoDesc.RTVFormats[0]					   = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.PrimitiveTopologyType			   = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.SampleDesc.Count				   = 1;
		psoDesc.SampleMask						   = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.DSVFormat						   = DXGI_FORMAT_D32_FLOAT;

		// shaderPath
		std::wstring vsPath = L"Skybox.VS.hlsl";
		std::wstring psPath = L"Skybox.PS.hlsl";

		pipelineStateManager_->CreatePipelineState(
			PipelineType::Skybox,
			vsPath,
			psPath,
			rootSignatureDesc,
			psoDesc,
			BlendMode::NORMAL);
	}

} // namespace CalyxEngine