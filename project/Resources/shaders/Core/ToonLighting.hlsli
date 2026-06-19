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

    [loop]
    for(uint i = 0; i < gPointLightCount; ++i) {
        PointLight pointLight = gPointLights[i];
        if(pointLight.intensity <= 0.0f || pointLight.radius <= 0.0f) {
            continue;
        }

        float3 lightDir    = normalize(worldPos - pointLight.position);
        float  distance    = length(pointLight.position - worldPos);
        float  attenuation = pow(saturate(1.0f - distance / pointLight.radius), pointLight.decay);

        float rawNdotL = dot(normal, -lightDir);
        float contribution = saturate(rawNdotL) * pointLight.intensity * attenuation;

        float3 baseDiffuse = EvaluateToonRamp(rawNdotL, albedo);
        float3 lightDiffuse = baseDiffuse * pointLight.color.rgb * pointLight.intensity * attenuation;
        diffuse += lightDiffuse;

        float3 halfVec = normalize(-lightDir + toEye);
        float  NdotH   = saturate(dot(normal, halfVec));
        float toonSpecular = EvaluateToonSpecular(NdotH);

        float3 lightSpecular = pointLight.color.rgb * gMaterial.toonHighlightColor.rgb * toonSpecular * pointLight.intensity * attenuation;
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
