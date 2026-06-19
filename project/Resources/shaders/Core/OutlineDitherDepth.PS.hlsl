#include "Object3D.hlsli"

static const float4x4 kBayerMatrix = float4x4(
	0.0 / 16.0, 8.0 / 16.0, 2.0 / 16.0, 10.0 / 16.0,
	12.0 / 16.0, 4.0 / 16.0, 14.0 / 16.0, 6.0 / 16.0,
	3.0 / 16.0, 11.0 / 16.0, 1.0 / 16.0, 9.0 / 16.0,
	15.0 / 16.0, 7.0 / 16.0, 13.0 / 16.0, 5.0 / 16.0
);

void main(VertexShaderOutput input) {
	uint2 pixelPos = uint2(input.position.xy) % 4;
	float ditherThreshold = kBayerMatrix[pixelPos.y][pixelPos.x];

	float dist = length(input.worldPosition - cameraPosition);
	float fadeNear = 2.5f;
	float fadeFar = 10.0f;
	float fade = saturate((dist - fadeNear) / (fadeFar - fadeNear));

	if(fade <= ditherThreshold) {
		discard;
	}
}
