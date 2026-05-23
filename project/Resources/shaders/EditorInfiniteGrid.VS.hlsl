struct VertexOut {
	float4 position : SV_POSITION;
	float2 ndc : TEXCOORD0;
};

static const float2 kPositions[3] = {
	float2(-1.0f,  1.0f),
	float2( 3.0f,  1.0f),
	float2(-1.0f, -3.0f),
};

VertexOut main(uint vertexId : SV_VertexID) {
	VertexOut output;
	output.ndc = kPositions[vertexId];
	output.position = float4(output.ndc, 0.0f, 1.0f);
	return output;
}
