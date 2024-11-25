struct GSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

struct GSOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL;
};

[maxvertexcount(3)]
void GSMain(triangle GSInput input[3], inout TriangleStream<GSOutput> outputStream)
{
    float3 faceNormal = normalize(cross(input[1].position.xyz - input[0].position.xyz,
                                        input[2].position.xyz - input[0].position.xyz));
    
    for (int i = 0; i < 3; i++)
    {
        GSOutput output;
        output.position = input[i].position + float4(faceNormal * 0.1, 0.0);
        output.color = input[i].color;
        output.normal = faceNormal;
        outputStream.Append(output);
    }
}