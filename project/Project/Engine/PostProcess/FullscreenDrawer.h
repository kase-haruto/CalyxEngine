#pragma once
#include <d3d12.h>

class FullscreenDrawer{
public:
	// 一度だけ呼んで初期化
	static void Initialize(ID3D12Device* device);
	// 三角形描画の実行
	static void Draw(ID3D12GraphicsCommandList* cmd);

private:
	static void CreateVertexBuffer(ID3D12Device* device);
	static bool initialized_;
};