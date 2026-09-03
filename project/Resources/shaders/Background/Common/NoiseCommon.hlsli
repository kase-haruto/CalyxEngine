float Hash11(float p) {
    p = frac(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return frac(p);
}

float Hash31(float3 p) {
    p = frac(p * 0.1031f);
    p += dot(p, p.yzx + 33.33f);
    return frac((p.x + p.y) * p.z);
}

float3 Hash33(float3 p) {
    p = frac(p * float3(0.1031f, 0.1030f, 0.0973f));
    p += dot(p, p.yxz + 33.33f);
    return frac((p.xxy + p.yxx) * p.zyx);
}

float ValueNoise(float3 p) {
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0f - 2.0f * f);
    float n000 = Hash31(i + float3(0,0,0));
    float n100 = Hash31(i + float3(1,0,0));
    float n010 = Hash31(i + float3(0,1,0));
    float n110 = Hash31(i + float3(1,1,0));
    float n001 = Hash31(i + float3(0,0,1));
    float n101 = Hash31(i + float3(1,0,1));
    float n011 = Hash31(i + float3(0,1,1));
    float n111 = Hash31(i + float3(1,1,1));
    return lerp(lerp(lerp(n000,n100,f.x),lerp(n010,n110,f.x),f.y),
                lerp(lerp(n001,n101,f.x),lerp(n011,n111,f.x),f.y),f.z);
}

float Fbm4(float3 p) {
    float sum = 0.0f;
    float amplitude = 0.55f;
    [unroll] for(int octave = 0; octave < 4; ++octave) {
        sum += ValueNoise(p) * amplitude;
        p = p * 2.03f + float3(7.1f, 3.7f, 5.3f);
        amplitude *= 0.47f;
    }
    return sum;
}
