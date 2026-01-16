#pragma once

#include "BaseModel.h"

/*-----------------------------------------------------------------------------------------
 * Model
 * - 静的モデルクラス
 * - BaseModelを継承し、静的メッシュの描画・テクスチャ管理を担当
 *---------------------------------------------------------------------------------------*/
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
