float ToonBand(float value, float threshold, float softness) {
    float width = max(softness, 0.0001f);
    return smoothstep(threshold - width, threshold + width, value);
}

float3 EvaluateToonRamp(float ndotl, float3 albedo) {
    float shadeStep = min(gMaterial.toonShadeStep, gMaterial.toonBaseStep);
    float baseStep = max(gMaterial.toonShadeStep, gMaterial.toonBaseStep);

    float3 shadow = albedo * gMaterial.toonShadowColor.rgb;
    float3 midShadow = albedo * gMaterial.toonMidShadowColor.rgb;
    float3 base = albedo * gMaterial.toonBaseColor.rgb;

    float3 shadeRamp = lerp(shadow, midShadow, ToonBand(ndotl, shadeStep, gMaterial.toonShadeFeather));
    return lerp(shadeRamp, base, ToonBand(ndotl, baseStep, gMaterial.toonBaseFeather));
}

float EvaluateToonSpecular(float ndoth) {
    return ToonBand(ndoth, saturate(gMaterial.toonSpecularThreshold), gMaterial.toonSpecularSoftness) *
           max(gMaterial.toonSpecularIntensity, 0.0f);
}

void ComputeToonDirectionalLight(
    float3 normal,
    float3 toEye,
    float3 albedo,
    out float3 diffuse,
    out float3 specular
) {
    diffuse  = 0.0f;
    specular = 0.0f;

    float3 L = -gDirectionalLight.direction;
    float rawNdotL = dot(normal, L);
    float3 rampDiffuse = EvaluateToonRamp(rawNdotL, albedo);
    diffuse = rampDiffuse * gDirectionalLight.color.rgb * gDirectionalLight.intensity;

    float3 H    = normalize(L + toEye);
    float NdotH = saturate(dot(normal, H));
    
    float toonSpecular = EvaluateToonSpecular(NdotH);
    specular = gDirectionalLight.color.rgb * gMaterial.toonHighlightColor.rgb * toonSpecular * gDirectionalLight.intensity;
}

void ComputeToonPointLight(
    float3 normal,
    float3 toEye,
    float3 worldPos,
    float3 albedo,
    out float3 diffuse,
    out float3 specular
) {
    diffuse  = 0.0f;
    specular = 0.0f;

    float3 lightDir    = normalize(worldPos - gPointLight.position);
    float  distance    = length(gPointLight.position - worldPos);
    float  attenuation = pow(saturate(1.0f - distance / gPointLight.radius), gPointLight.decay);

    float rawNdotL = dot(normal, -lightDir);

    float3 baseDiffuse = EvaluateToonRamp(rawNdotL, albedo);
    
    diffuse = baseDiffuse * gPointLight.color.rgb * gPointLight.intensity * attenuation;

    float3 halfVec = normalize(-lightDir + toEye);
    float  NdotH   = saturate(dot(normal, halfVec));
    float toonSpecular = EvaluateToonSpecular(NdotH);

    specular = gPointLight.color.rgb * gMaterial.toonHighlightColor.rgb * toonSpecular * gPointLight.intensity * attenuation;
}
