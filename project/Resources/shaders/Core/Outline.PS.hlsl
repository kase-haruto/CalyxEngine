cbuffer OutlineConstants : register(b2) {
	float4 outlineColor;
	float outlineThickness;
}

float4 main() : SV_TARGET {
	return outlineColor;
}
