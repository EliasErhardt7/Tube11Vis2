struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

cbuffer ConstantBuffer : register(b0)
{
    matrix worldViewProj;
}

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position,1.0), worldViewProj);
    output.color = input.color;
    return output;
}