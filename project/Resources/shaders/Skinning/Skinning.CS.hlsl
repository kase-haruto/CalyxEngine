struct Vertex {
	float4 position;
	float2 texcoord;
	float3 normal;
	float4 tangent;
};

struct VertexInfluence {
	float4 weights;
	int4 jointIndices;
};

struct Well {
	float4x4 skeletonSpaceMatrix;
	float4x4 skeletonSpaceInverseTransposeMatrix;
};

cbuffer SkinningConstants : register(b0) {
	uint vertexCount;
};

StructuredBuffer<Vertex> gInputVertices : register(t0);
StructuredBuffer<VertexInfluence> gInfluences : register(t1);
StructuredBuffer<Well> gMatrixPalette : register(t2);
RWStructuredBuffer<Vertex> gOutputVertices : register(u0);

[numthreads(128, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
	uint vertexIndex = dispatchThreadId.x;
	if(vertexIndex >= vertexCount) {
		return;
	}

	Vertex src = gInputVertices[vertexIndex];
	VertexInfluence influence = gInfluences[vertexIndex];

	float4 skinnedPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float3 skinnedNormal = float3(0.0f, 0.0f, 0.0f);
	float3 skinnedTangent = float3(0.0f, 0.0f, 0.0f);
	float totalWeight = 0.0f;

	[unroll]
	for(uint i = 0; i < 4; ++i) {
		float weight = influence.weights[i];
		int jointIndex = influence.jointIndices[i];
		if(weight <= 0.0f || jointIndex < 0) {
			continue;
		}

		Well palette = gMatrixPalette[jointIndex];
		skinnedPosition += mul(src.position, palette.skeletonSpaceMatrix) * weight;
		skinnedNormal += mul(src.normal, (float3x3)palette.skeletonSpaceInverseTransposeMatrix) * weight;
		skinnedTangent += mul(src.tangent.xyz, (float3x3)palette.skeletonSpaceMatrix) * weight;
		totalWeight += weight;
	}

	Vertex dst = src;
	if(totalWeight > 0.0f) {
		dst.position = skinnedPosition;
		dst.position.w = 1.0f;
		dst.normal = normalize(skinnedNormal);
		dst.tangent.xyz = normalize(skinnedTangent);
	}

	gOutputVertices[vertexIndex] = dst;
}
