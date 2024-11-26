struct VertexPosTexCoordIn
{
    float4 position : POSITION;
    float2 tex : TEXCOORD;
};

struct VertexPosTexCoordOut
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
};

Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);
StructuredBuffer<uint2> kBuffer : register(t2);

cbuffer CompositeParams: register(b0)
{
    float coefficient;
}

SamplerState texSampler : register(s0);

// vertex shader
VertexPosTexCoordOut VSMain(VertexPosTexCoordIn v)
{
    VertexPosTexCoordOut output;
    output.position = v.position;
    output.tex = v.tex;

    return output;
}

// geometry shader
// Input and output structures for the geometry shader

struct GSOutput
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
};

// Geometry Shader
[maxvertexcount(3)]
void GSMain(triangle VertexPosTexCoordOut input[3], inout TriangleStream<GSOutput> outputStream)
{
    GSOutput output;
    
    // Pass through each vertex of the triangle
    for (int i = 0; i < 3; i++)
    {
        output.position = input[i].position/2;
        output.tex = input[i].tex;
        
        // You can add additional processing here if needed
        // For example, you could offset the vertices or modify the texture coordinates
        
        outputStream.Append(output);
    }
    
    outputStream.RestartStrip();
}

// pixel shader
float4 PSMain(GSOutput p) : SV_TARGET
{
    // output: tex0 + coefficient * tex1
	float x = float((kBuffer[p.position.x].x*kBuffer[p.position.x].y)/1024.f);
	float y = float((kBuffer[p.position.y].x*kBuffer[p.position.y].y)/768.f);
    return float4(x,y,0,1); //mad(coefficient, , tex0.Sample(texSampler, p.tex));
}