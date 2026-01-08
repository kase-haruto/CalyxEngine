///////////////////////////////////////////////////////////////////////////////
// constant buffers
///////////////////////////////////////////////////////////////////////////////
cbuffer ShadowCB : register(b0) {
	float4x4 gLightVP;
};

cbuffer ObjectConstants : register(b1) {
	float4x4 World;
	float4x4 WorldInverseTranspose;
}

struct Well {
	float4x4 skeletonSpaceMatrix;
	float4x4 skeletonSpaceInverseTransposeMatrix;
};

///////////////////////////////////////////////////////////////////////////////
// tables
///////////////////////////////////////////////////////////////////////////////
StructuredBuffer<Well> gMatrixPalette : register(t0);

///////////////////////////////////////////////////////////////////////////////
// structs
///////////////////////////////////////////////////////////////////////////////
struct VSIn {
	float4 position : POSITION;
	float4 weight : WEIGHT;
	int4   index : INDEX;
};

struct VSOut {
	float4 svpos : SV_POSITION;
};

struct Skinned {
	float4 position;
};

Skinned Skinning(VSIn input) {

	Skinned skinned;

	// 位置の変換
	skinned.position = mul(input.position, gMatrixPalette[input.index.x].skeletonSpaceMatrix) * input.weight.x;
	skinned.position += mul(input.position, gMatrixPalette[input.index.y].skeletonSpaceMatrix) * input.weight.y;
	skinned.position += mul(input.position, gMatrixPalette[input.index.z].skeletonSpaceMatrix) * input.weight.z;
	skinned.position += mul(input.position, gMatrixPalette[input.index.w].skeletonSpaceMatrix) * input.weight.w;
	skinned.position.w = 1.0f;

	return skinned;
}

///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
VSOut main(VSIn v) {
	VSOut o;

	Skinned skinned = Skinning(v);
	float4 worldPos = mul(skinned.position, World);
	o.svpos = mul(worldPos,gLightVP);
	
	return o;
}