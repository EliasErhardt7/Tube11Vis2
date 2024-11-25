struct PSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 lightDir = normalize(float3(1, 1, -1));
    float diffuse = max(dot(input.normal, lightDir), 0.0);
    float3 finalColor = input.color * (diffuse + 0.2);
    return float4(finalColor, 1.0);
}