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
    float roughness = saturate(gMaterial.roughness);
    float specularWeight = pow(1.0f - roughness, 2.0f);

    if(gMaterial.enableLighting == 0) {
        float halfLambert = pow(rawNdotL * 0.5f + 0.5f, 2.0f);
        diffuse = albedo * gDirectionalLight.color.rgb * halfLambert * gDirectionalLight.intensity;

        float3 H    = normalize(L + toEye);
        float NdotH = saturate(dot(normal, H));
        specular    = gDirectionalLight.color.rgb * pow(NdotH, gMaterial.shiniess) * gDirectionalLight.intensity * specularWeight;
    }
    else if(gMaterial.enableLighting == 1) {
        diffuse = albedo * gDirectionalLight.color.rgb * NdotL * gDirectionalLight.intensity;

        float3 H    = normalize(L + toEye);
        float NdotH = saturate(dot(normal, H));
        specular    = gDirectionalLight.color.rgb * pow(NdotH, gMaterial.shiniess) * gDirectionalLight.intensity * specularWeight;
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
    float topContribution0 = 0.0f;
    float topContribution1 = 0.0f;
    float3 topDiffuse0 = 0.0f;
    float3 topDiffuse1 = 0.0f;
    float3 topSpecular0 = 0.0f;
    float3 topSpecular1 = 0.0f;
    float3 topPosition0 = 0.0f;
    float3 topPosition1 = 0.0f;
    float topDistance0 = 0.0f;
    float topDistance1 = 0.0f;
    float roughness = saturate(gMaterial.roughness);
    float specularWeight = pow(1.0f - roughness, 2.0f);

    [loop]
    for(uint i = 0; i < gPointLightCount; ++i) {
        PointLight pointLight = gPointLights[i];
        if(pointLight.intensity <= 0.0f || pointLight.radius <= 0.0f) {
            continue;
        }

        float3 lightDir    = normalize(worldPos - pointLight.position);
        float  distance    = length(pointLight.position - worldPos);
        float  attenuation = pow(saturate(1.0f - distance / pointLight.radius), pointLight.decay);

        float NdotL = saturate(dot(normal, -lightDir));
        float contribution = NdotL * pointLight.intensity * attenuation;
        float3 lightDiffuse = albedo * pointLight.color.rgb * contribution;
        diffuse += lightDiffuse;

        float3 halfVec = normalize(-lightDir + toEye);
        float  NdotH   = saturate(dot(normal, halfVec));
        float3 lightSpecular = pointLight.color.rgb * pow(NdotH, gMaterial.shiniess) * pointLight.intensity * attenuation * specularWeight;
        specular += lightSpecular;

        if(gPointLightShadowsEnabled != 0 && contribution > gPointShadowContributionThreshold) {
            if(contribution > topContribution0) {
                topContribution1 = topContribution0;
                topDiffuse1 = topDiffuse0;
                topSpecular1 = topSpecular0;
                topPosition1 = topPosition0;
                topDistance1 = topDistance0;

                topContribution0 = contribution;
                topDiffuse0 = lightDiffuse;
                topSpecular0 = lightSpecular;
                topPosition0 = pointLight.position;
                topDistance0 = distance;
            } else if(contribution > topContribution1) {
                topContribution1 = contribution;
                topDiffuse1 = lightDiffuse;
                topSpecular1 = lightSpecular;
                topPosition1 = pointLight.position;
                topDistance1 = distance;
            }
        }
    }

    if(gPointLightShadowsEnabled != 0 && gMaxPointShadowLights > 0 && topContribution0 > 0.0f) {
        float shadow0 = ComputePointHardShadow_RT(worldPos, normal, topPosition0, topDistance0);
        diffuse += topDiffuse0 * (shadow0 - 1.0f);
        specular += topSpecular0 * (shadow0 - 1.0f);
    }
    if(gPointLightShadowsEnabled != 0 && gMaxPointShadowLights > 1 && topContribution1 > 0.0f) {
        float shadow1 = ComputePointHardShadow_RT(worldPos, normal, topPosition1, topDistance1);
        diffuse += topDiffuse1 * (shadow1 - 1.0f);
        specular += topSpecular1 * (shadow1 - 1.0f);
    }
}
