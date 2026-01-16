#pragma once
/* ========================================================================
   include space
   ===================================================================== */
#include <Engine/objects/LightObject/DirectionalLight.h>
#include <Engine/objects/LightObject/PointLight.h>

#include <memory>

/*-----------------------------------------------------------------------------------------
 * LightLibrary
 * - ライト管理クラス
 * - シーン内のディレクショナルライト・ポイントライトの登録とGPUへのコマンド送信を管理
 *---------------------------------------------------------------------------------------*/
class LightLibrary {
public:
	LightLibrary()  = default;
	~LightLibrary() = default;

	/* GPU 同期 ---------------------------------------------------------*/
	void CyncGpu();
	void Clear();

	/* 登録／取得 --------------------------------------------------------*/
	void SetDirectionalLight(const std::shared_ptr<DirectionalLight>& light);
	void SetPointLight(const std::shared_ptr<PointLight>& light);

	DirectionalLight* GetDirectionalLight() const { return directionalLight_.get(); }
	PointLight*       GetPointLight() const { return pointLight_.get(); }

	/* 描画コマンド ------------------------------------------------------*/
	void SetCommand(ID3D12GraphicsCommandList* cmdList,PipelineType pipelineType);
	void SetCommand(ID3D12GraphicsCommandList* cmdList,
	                PipelineType               pipelineType,
	                LightType                  lightType);

private:
	std::shared_ptr<DirectionalLight> directionalLight_;
	std::shared_ptr<PointLight>       pointLight_;
};