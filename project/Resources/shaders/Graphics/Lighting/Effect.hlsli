struct TransformationMatrix {
    float4x4 WVP;
    float4x4 world;
    float4x4 worldInverseTranspose;
};

struct VSOutput {
    float4 svpos : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};