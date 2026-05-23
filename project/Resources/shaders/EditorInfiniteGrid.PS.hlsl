struct PixelIn {
	float4 position : SV_POSITION;
	float2 ndc : TEXCOORD0;
};

cbuffer CameraConstants : register(b0) {
	float4x4 View;
	float4x4 Projection;
	float4x4 ViewProjection;
	float3 cameraPosition;
	float  _cameraPad0;
	float3 camRight;
	float  _cameraPad1;
	float3 camUp;
	float  _cameraPad2;
	float3 camForward;
	float  _cameraPad3;
	float2 viewportSize;
	float2 _cameraPad4;
};

cbuffer GridConstants : register(b1) {
	float minorSpacing;
	float majorSpacing;
	float axisWidth;
	float minorAlpha;
	float majorAlpha;
	float fadeStart;
	float fadeEnd;
	float horizonFade;
	float4 minorColor;
	float4 majorColor;
	float4 xAxisColor;
	float4 yAxisColor;
	float4 zAxisColor;
};

float GridLine(float coord, float spacing, float width) {
	float scaled = coord / spacing;
	float d = abs(frac(scaled + 0.5f) - 0.5f);
	float aa = max(fwidth(scaled), 1e-5f);
	return 1.0f - smoothstep(width, width + aa * 1.5f, d);
}

float AxisLine(float coord, float widthWorld) {
	float d = abs(coord);
	float aa = max(fwidth(coord), 1e-5f);
	return 1.0f - smoothstep(widthWorld, widthWorld + aa * 2.0f, d);
}

float3 ReconstructWorldRay(float2 ndc) {
	float tanY = 1.0f / max(Projection._22, 1e-5f);
	float tanX = 1.0f / max(Projection._11, 1e-5f);

	float3 viewRay = normalize(float3(ndc.x * tanX, ndc.y * tanY, 1.0f));
	float3 worldRay =
		camRight * viewRay.x +
		camUp * viewRay.y +
		camForward * viewRay.z;
	return normalize(worldRay);
}

float4 main(PixelIn input) : SV_TARGET {
	float3 ray = ReconstructWorldRay(input.ndc);

	// Y-Up world: draw XZ plane at Y=0.
	if(abs(ray.y) < 1e-5f) discard;

	float t = -cameraPosition.y / ray.y;
	if(t <= 0.0f) discard;

	float3 worldPos = cameraPosition + ray * t;
	float distanceToCamera = length(worldPos - cameraPosition);

	float distanceFade = 1.0f - smoothstep(fadeStart, fadeEnd, distanceToCamera);
	float grazing = saturate(abs(ray.y));
	float horizon = smoothstep(0.0f, max(horizonFade, 1e-4f), grazing);
	float fade = distanceFade * horizon;
	if(fade <= 0.001f) discard;

	// Camera adaptive LOD. Close range adds finer lines; far range leaves major lines.
	float lod = saturate((distanceToCamera - fadeStart * 0.25f) / max(fadeEnd - fadeStart * 0.25f, 1e-4f));
	float minorLodAlpha = (1.0f - lod) * minorAlpha;

	float minorX = GridLine(worldPos.x, minorSpacing, 0.018f);
	float minorZ = GridLine(worldPos.z, minorSpacing, 0.018f);
	float majorX = GridLine(worldPos.x, majorSpacing, 0.014f);
	float majorZ = GridLine(worldPos.z, majorSpacing, 0.014f);

	float minorLine = max(minorX, minorZ) * minorLodAlpha;
	float majorLine = max(majorX, majorZ) * majorAlpha;

	float xAxis = AxisLine(worldPos.z, axisWidth);
	float zAxis = AxisLine(worldPos.x, axisWidth);
	float yAxis = AxisLine(length(worldPos.xz), axisWidth * 0.7f);

	float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
	color = lerp(color, minorColor, minorLine * minorColor.a);
	color = lerp(color, majorColor, majorLine * majorColor.a);
	color = lerp(color, xAxisColor, xAxis * xAxisColor.a);
	color = lerp(color, zAxisColor, zAxis * zAxisColor.a);
	color = lerp(color, yAxisColor, yAxis * yAxisColor.a);

	color.a = max(max(minorLine, majorLine), max(max(xAxis, zAxis), yAxis)) * fade;
	color.rgb *= fade;

	if(color.a <= 0.001f) discard;
	return color;
}
