#pragma once

/* lib */
#include<d3d12.h>
#include<DirectXMath.h>
#include<wrl.h>

using namespace DirectX;
using namespace Microsoft::WRL;

class DxCore;

//霧のパラメータを表す構造体
struct FogParameters{
	float fogStart;
	float fogEnd;
	float pad[2];
	XMFLOAT4 fogColor;
};

class FogEffect{
private:
	//霧のパラメータ
	FogParameters* parameters;
	//霧のパラメータを保有する定数バッファ
	ComPtr<ID3D12Resource> constantBuffer;
	UINT8* mappedConstantBuffer;
	UINT bufferSize;



	const DxCore* pDxCore_;

public:
	//コンストラクタ
	FogEffect(const DxCore* dxCore);
	//デストラクタ
	~FogEffect();

	void ShowImGuiInterface();


	void Update();

	//定数バッファの生成
	void CreateConstantBuffer();

	//霧の効果を適用
	void Apply();

	ComPtr<ID3D12Resource>GetConstantBuffer()const{ return constantBuffer; }
};

