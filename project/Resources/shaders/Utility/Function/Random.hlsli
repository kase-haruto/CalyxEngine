float rand3dTo1d(float3 value, float3 dotDir = float3(12.9898, 78.233, 37.719))
{
    //make value smaller to avoid artefacts
    float3 smallValue = sin(value);
    //get scalar value from 3d vector
    float random = dot(smallValue, dotDir);
    //make value more random by making it bigger and then taking the factional part
    random = frac(sin(random) * 143758.5453);
    return random;
}

float rand2dTo1d(float2 value, float2 dotDir = float2(12.9898, 78.233))
{
    float2 smallValue = sin(value);
    float random = dot(smallValue, dotDir);
    random = frac(sin(random) * 143758.5453);
    return random;
}

float rand1dTo1d(float3 value, float mutator = 0.546)
{
    float random = (frac(sin(value.x + mutator) * 143758.5453));
    return random;
}

//to 2d functions

float2 rand3dTo2d(float3 value)
{
    return float2(
        rand3dTo1d(value, float3(12.989, 78.233, 37.719)),
        rand3dTo1d(value, float3(39.346, 11.135, 83.155))
    );
}

float2 rand2dTo2d(float2 value)
{
    return float2(
        rand2dTo1d(value, float2(12.989, 78.233)),
        rand2dTo1d(value, float2(39.346, 11.135))
    );
}

float2 rand1dTo2d(float value)
{
    return float2(
        rand2dTo1d(value, 3.9812),
        rand2dTo1d(value, 7.1536)
    );
}

//to 3d functions

float3 rand3dTo3d(float3 value)
{
    return float3(
        rand3dTo1d(value, float3(12.989, 78.233, 37.719)),
        rand3dTo1d(value, float3(39.346, 11.135, 83.155)),
        rand3dTo1d(value, float3(73.156, 52.235, 09.151))
    );
}

float3 rand2dTo3d(float2 value)
{
    return float3(
        rand2dTo1d(value, float2(12.989, 78.233)),
        rand2dTo1d(value, float2(39.346, 11.135)),
        rand2dTo1d(value, float2(73.156, 52.235))
    );
}

float3 rand1dTo3d(float value)
{
    return float3(
        rand1dTo1d(value, 3.9812),
        rand1dTo1d(value, 7.1536),
        rand1dTo1d(value, 5.7241)
    );
}



class RandomGenerator
{
    uint seed;

    void InitSeed(uint3 baseSeed,float perTime)
    {
        seed = baseSeed.x * 73856093 ^ baseSeed.y * 19349663 ^ baseSeed.z * 83492791;
        seed ^= asuint(perTime * 1000.0f);
    }

    uint XorShift()
    {
        seed ^= (seed << 13);
        seed ^= (seed >> 17);
        seed ^= (seed << 5);
        return seed;
    }

    float Generate1d()
    {
        return float(XorShift() & 0x00FFFFFF) / 16777216.0f; // [0,1)
    }

    float3 Generate3d()
    {
        return float3(Generate1d(), Generate1d(), Generate1d()) * 2.0f - 1.0f; // [-1,1]
    }
    
    float3 GenerateUnitSphereDirection()
    {
        float u = Generate1d(); // [0,1)
        float v = Generate1d(); // [0,1)

        float theta = 2.0f * 3.14159265f * u; // [0, 2π)
        float phi = acos(2.0f * v - 1.0f); // [0, π]

        float x = sin(phi) * cos(theta);
        float y = sin(phi) * sin(theta);
        float z = cos(phi);

        return float3(x, y, z);
    }
    
    float GenerateRange1d(float minValue, float maxValue)
    {
        float r = Generate1d(); // [0,1)
        return lerp(minValue, maxValue, r);
    }
};