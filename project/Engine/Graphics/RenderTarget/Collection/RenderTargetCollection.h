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

class RenderTargetCollection{
public:
	RenderTargetCollection() = default;
	~RenderTargetCollection() = default;

	void Add(const std::string& name, std::unique_ptr<IRenderTarget> target);
	IRenderTarget* Get(const std::string& name) const;
	void ClearAll(ID3D12GraphicsCommandList* cmdList);

private:
	std::unordered_map<std::string, std::unique_ptr<IRenderTarget>> targets_;
};