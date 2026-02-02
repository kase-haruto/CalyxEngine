#include "Object3D.hlsli"

struct VertexShaderInput {
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

// PickingPass.cpp で inst.transform->SetCommand(cmd, 1) によりセットされるバッファ
cbuffer WorldTransformCB : register(b0) {
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput o;
    
    float4 worldPos = mul(input.position, World);
    o.position = mul(worldPos, ViewProjection);
    o.worldPosition = worldPos.xyz;
    o.texcoord = input.texcoord;
    o.normal = normalize(mul(input.normal, (float3x3)WorldInverseTranspose));
    
    return o;
}
