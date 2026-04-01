#include "Object3D.hlsli"

struct GSOutput {
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 normal   : NORMAL0;
	float3 worldPos : POSITION0;
};

struct WireframeConstants {
	float thickness;
};
ConstantBuffer<WireframeConstants> gWireframe : register(b6);


void EmitEdge(VertexShaderOutput v0, VertexShaderOutput v1, inout TriangleStream<GSOutput> stream) {
	float2 p0 = v0.position.xy / v0.position.w;
	float2 p1 = v1.position.xy / v1.position.w;

	float2 dir = normalize(p1 - p0);
	float2 normal = float2(-dir.y, dir.x);

	// Screen-space expansion
	// viewportSize はピクセル単位。NDCは -1〜1 なので、2.0 / viewportSize をかける。
	float2 expansion = normal * (gWireframe.thickness / viewportSize);

	GSOutput o;
	o.normal   = v0.normal;
	o.worldPos = v0.worldPosition;
	o.texcoord = v0.texcoord;

	// Quad vertices
	o.position = v0.position + float4(expansion * v0.position.w, 0, 0);
	stream.Append(o);
	o.position = v0.position - float4(expansion * v0.position.w, 0, 0);
	stream.Append(o);
	o.position = v1.position + float4(expansion * v1.position.w, 0, 0);
	stream.Append(o);
	o.position = v1.position - float4(expansion * v1.position.w, 0, 0);
	stream.Append(o);

	stream.RestartStrip();
}

[maxvertexcount(12)]
void main(triangle VertexShaderOutput input[3], inout TriangleStream<GSOutput> outStream) {
	EmitEdge(input[0], input[1], outStream);
	EmitEdge(input[1], input[2], outStream);
	EmitEdge(input[2], input[0], outStream);
}
