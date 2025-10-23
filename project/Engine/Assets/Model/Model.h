#pragma once

#include "BaseModel.h"

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
	void Map() override;
	void Draw(const WorldTransform& transform)override;
	void ShowImGuiInterface() override;

private:
	//===================================================================*/
	//                   private methods
	//===================================================================*/
	void CreateMaterialBuffer() override;
	void MaterialBufferMap()override;
};
