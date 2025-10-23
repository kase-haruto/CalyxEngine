#pragma once

/* ========================================================================
/*	include	space
/* ===================================================================== */
// enigne
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>

// c++
#include <string>
#include <unordered_map>
#include <memory>

/* ========================================================================
/*	レンダーtargetCollection
/* ===================================================================== */
class RenderTargetCollection{
public:
	RenderTargetCollection() = default;
	~RenderTargetCollection() = default;

	/// <summary>
	/// レンダーtarget追加
	/// </summary>
	/// <param name="name"></param>
	/// <param name="target"></param>
	void Add(const std::string& name, std::unique_ptr<IRenderTarget> target);

	/// <summary>
	/// 取得
	/// </summary>
	/// <param name="name"></param>
	/// <returns></returns>
	IRenderTarget* Get(const std::string& name) const;

	/// <summary>
	/// すべてクリア
	/// </summary>
	/// <param name="cmdList"></param>
	void ClearAll(ID3D12GraphicsCommandList* cmdList);

private:
	std::unordered_map<std::string, std::unique_ptr<IRenderTarget>> targets_;
};