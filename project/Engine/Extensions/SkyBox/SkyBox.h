#pragma once

/* ========================================================================
/* include space
/* ===================================================================== */

#include <Engine/Renderer/Mesh/VertexData.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Graphics/Buffer/DxIndexBuffer.h>
#include <Engine/Graphics/Buffer/DxVertexBuffer.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>

#include <string>
#include <array> 
#include <functional>
#include <optional>

struct CxMath::Vector3;

class SkyBox :
	public SceneObject{
public:
	SkyBox(const std::string& fileName,
		   std::optional<std::string> objectName);
	~SkyBox()override = default;
	void Initialize();
	void ShowGui();
	void Update(float dt)override;
	void AlwaysUpdate(float dt)override;
	void Draw(ID3D12GraphicsCommandList* cmd);
	const WorldTransform& GetWorldTransform()const;

	//* config ================================================================*/
	std::string_view GetTypeName() const override{return "SkyBox";}
private:
	std::array<VertexData, 24> vertices_;
	std::array<uint16_t, 36> indices_;
	std::string textureName_;

	DxVertexBuffer<VertexData> vertexBuffer_;
	DxIndexBuffer<uint16_t> indexBuffer_;
	WorldTransform worldTransform_;
};

