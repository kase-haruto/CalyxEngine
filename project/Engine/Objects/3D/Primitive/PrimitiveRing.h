#pragma once

// engine
#include "IPrimitiveMesh.h"
#include "../Mesh/MeshData.h"

/*-----------------------------------------------------------------------------------------
 * PrimitiveRing
 * - リングプリミティブクラス
 * - 3D空間におけるリング形状の基本情報と操作を提供
 *---------------------------------------------------------------------------------------*/
class PrimitiveRing final :
	public IPrimitiveMesh {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	PrimitiveRing() = default;
	~PrimitiveRing() override = default;

	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;

private:
	//===================================================================*/
	//			private members
	//===================================================================*/
	MeshResource mesh_;
};