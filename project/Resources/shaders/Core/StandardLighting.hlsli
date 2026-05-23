void ComputeStandardDirectionalLight(
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
    float NdotL    = saturate(rawNdotL);

    if(gMaterial.enableLighting == 0) {
        float halfLambert = pow(rawNdotL * 0.5f + 0.5f, 2.0f);
        diffuse = albedo * gDirectionalLight.color.rgb * halfLambert * gDirectionalLight.intensity;

        float3 H    = normalize(L + toEye);
        float NdotH = saturate(dot(normal, H));
        specular    = gDirectionalLight.color.rgb * pow(NdotH, gMaterial.shiniess) * gDirectionalLight.intensity;
    }
    else if(gMaterial.enableLighting == 1) {
        diffuse = albedo * gDirectionalLight.color.rgb * NdotL * gDirectionalLight.intensity;

        float3 H    = normalize(L + toEye);
        float NdotH = saturate(dot(normal, H));
        specular    = gDirectionalLight.color.rgb * pow(NdotH, gMaterial.shiniess) * gDirectionalLight.intensity;
    }
}

void ComputeStandardPointLight(
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

    float NdotL = saturate(dot(normal, -lightDir));
    diffuse     = albedo * gPointLight.color.rgb * NdotL * gPointLight.intensity * attenuation;

    float3 halfVec = normalize(-lightDir + toEye);
    float  NdotH   = saturate(dot(normal, halfVec));
    specular       = gPointLight.color.rgb * pow(NdotH, gMaterial.shiniess) * gPointLight.intensity * attenuation;
}
