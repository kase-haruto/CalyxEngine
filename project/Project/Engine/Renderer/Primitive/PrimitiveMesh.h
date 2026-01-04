#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
/* engine */
#include <Engine/Graphics/Pipeline/BlendMode/BlendMode.h>

#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
/* ========================================================================
/* primitiveObject
/* ===================================================================== */
class IPrimitiveMesh{
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	IPrimitiveMesh() = default;

	//--------- accessor -----------------------------------------------------
	//material
	virtual void SetColor(const Vector4& color) = 0;

	virtual void SetBlendMode(BlendMode mode) = 0;
	virtual BlendMode GetBlendMode() const = 0;
	virtual const Matrix4x4& GetWorldMatrix() const =0;
private:
	//===================================================================*/
	//			private functions
	//===================================================================*/
};

