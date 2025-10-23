#pragma once

#include "BaseModel.h"

/* ========================================================================
/*		静的モデル
/* ===================================================================== */
class Model
	: public BaseModel{
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	Model() = default;
	Model(const std::string& fileName);
	~Model() override;

	void Initialize() override;
	void InitializeTextures(const std::vector<std::string>& textureFilePaths);
	void Draw(const WorldTransform& transform)override;
	void ShowImGuiInterface() override;

private:
	//===================================================================*/
	//                   private methods
	//===================================================================*/
	void CreateMaterialBuffer() override;
	void MaterialBufferMap()override;
	void Map() override;
};
