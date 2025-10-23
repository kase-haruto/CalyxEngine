#pragma once

// lib
#include <d3d12.h>
#include <d3dx12.h>
#include <cassert>

enum class BlendMode{
	NONE,				//< ブレンドなし
	ALPHA,				//< アルファブレンド
	ADD,				//< 加算ブレンド
	SUB,				//< 減算ブレンド
	MUL,				//< 乗算ブレンド
	NORMAL,				//< 通常ブレンド
	SCREEN,				//< スクリーンブレンド
	kBlendModeCount		//< ブレンドモードの数
};

/* ========================================================================
/*		blendModeの作成
/* ===================================================================== */
inline D3D12_BLEND_DESC CreateBlendDesc(BlendMode mode){
	// D3D12_DEFAULTをベースに初期化
	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	// 今回はRT[0]のみ設定する想定
	auto& rt0 = blendDesc.RenderTarget[0];
	rt0.BlendEnable = FALSE;
	// 書き込みマスク(RGBAすべてに書き込み)
	rt0.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	switch (mode){
		//-------------------------------------------------------------------------
		case BlendMode::NONE:
			break;

			//-------------------------------------------------------------------------
		case BlendMode::ALPHA:
			rt0.BlendEnable = true;
			rt0.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rt0.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			rt0.BlendOp = D3D12_BLEND_OP_ADD;
			rt0.SrcBlendAlpha = D3D12_BLEND_ZERO;
			rt0.DestBlendAlpha = D3D12_BLEND_ONE;
			rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;

			//-------------------------------------------------------------------------
		case BlendMode::ADD:
			rt0.BlendEnable = true;
			rt0.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rt0.DestBlend = D3D12_BLEND_ONE;
			rt0.BlendOp = D3D12_BLEND_OP_ADD;
			rt0.SrcBlendAlpha = D3D12_BLEND_ZERO;
			rt0.DestBlendAlpha = D3D12_BLEND_ONE;
			rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;

			//-------------------------------------------------------------------------
		case BlendMode::SUB:
			rt0.BlendEnable = true;
			rt0.SrcBlend = D3D12_BLEND_ONE;
			rt0.DestBlend = D3D12_BLEND_ONE;
			rt0.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
			rt0.SrcBlendAlpha = D3D12_BLEND_ZERO;
			rt0.DestBlendAlpha = D3D12_BLEND_ONE;
			rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;

			//-------------------------------------------------------------------------
		case BlendMode::MUL:
			rt0.BlendEnable = true;
			rt0.SrcBlend = D3D12_BLEND_DEST_COLOR;
			rt0.DestBlend = D3D12_BLEND_ZERO;
			rt0.BlendOp = D3D12_BLEND_OP_ADD;
			rt0.SrcBlendAlpha = D3D12_BLEND_ZERO;
			rt0.DestBlendAlpha = D3D12_BLEND_ONE;
			rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;

			//-------------------------------------------------------------------------
		case BlendMode::NORMAL:
			rt0.BlendEnable = true;
			rt0.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rt0.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			rt0.BlendOp = D3D12_BLEND_OP_ADD;
			rt0.SrcBlendAlpha = D3D12_BLEND_ZERO;
			rt0.DestBlendAlpha = D3D12_BLEND_ONE;
			rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;

			//-------------------------------------------------------------------------
		case BlendMode::SCREEN:
			rt0.BlendEnable = true;
			rt0.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			rt0.DestBlend = D3D12_BLEND_ONE;
			rt0.BlendOp = D3D12_BLEND_OP_ADD;
			rt0.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
			rt0.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
			rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;

			//-------------------------------------------------------------------------
		default:
			assert(false && "Unsupported BlendMode!");
			break;
	}

	return blendDesc;
}
