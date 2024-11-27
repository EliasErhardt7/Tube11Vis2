struct matrices_and_user_input {
	float4x4 mViewMatrix;
	float4x4 mProjMatrix;
	float4 mCamPos;
	float4 mCamDir;
	float4 mClearColor;
	float4 mHelperLineColor;
	float4 kBufferInfo; 

	float4 mDirLightDirection;
    float4 mDirLightColor;
    float4 mAmbLightColor;
    float4 mMaterialLightReponse;

	float4 mVertexColorMin;
	float4 mVertexColorMax;
	float4 mVertexAlphaBounds;
	float4 mVertexRadiusBounds;
	
	bool mBillboardClippingEnabled;
	bool mBillboardShadingEnabled;
	uint mVertexColorMode;
	uint mVertexAlphaMode;
	uint mVertexRadiusMode;

	bool mVertexAlphaInvert;
	bool mVertexRadiusInvert;
	float mDataMaxLineLength;
	float mDataMaxVertexAdjacentLineLength;
};


StructuredBuffer<uint2> kBuffer : register(t0);

cbuffer UniformBlock : register(b0) {
    matrices_and_user_input uboMatricesAndUserInput;
};

// Input structure
struct VertexInput
{
    float3 inPosition : POSITION;
    float inCurvature : TEXCOORD0;
    float inData : TEXCOORD1;
};

// Output structure
struct VertexOutput
{
    float4 position : SV_POSITION;
    float outCurvature : TEXCOORD0;
    float outData : TEXCOORD1;
};

// Vertex shader main function
VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    
    output.position = float4(input.inPosition, 1.0);
    output.outData = input.inCurvature;
    output.outCurvature = input.inData;
    
    return output;
}

// geometry shader
// Input and output structures for the geometry shader

struct GSOutput
{
    float4 position : SV_POSITION;
    float4 viewRay : TEXCOORD0;
    float4 color : COLOR;
    float3 posA : TEXCOORD1;
    float3 posB : TEXCOORD2;
    float2 rArB : TEXCOORD3;
    float3 n0 : TEXCOORD4;
    float3 n1 : TEXCOORD5;
    float3 posWS : TEXCOORD6;
};
/*
struct GSOutput
{
	float4 position : SV_POSITION;
};*/

void construct_billboard_for_line(float4 posA, float4 posB, float radA, float radB, float4 eyePos, float4 camDir, float4 posAPre, float4 posBSuc, float4 colA, float4 colB, inout TriangleStream<GSOutput> outputStream)
{
    float3 x0 = posA.xyz;
    float3 x1 = posB.xyz;
    float r0 = radA;
    float r1 = radB;
    if (r0 > r1) {
        x0 = posB.xyz;
        x1 = posA.xyz;
        r0 = radB;
        r1 = radA;
    }
    float3 e = eyePos.xyz;
    float3 d = x1 - x0;
    float3 d0 = e - x0;
    float3 d1 = e - x1;
    float3 u = normalize(cross(d, d0));
    float3 v0 = normalize(cross(u, d0));
    float3 v1 = normalize(cross(u, d1));
    float t0 = sqrt(dot(d0, d0) - r0 * r0);
    float s0 = r0 / t0;
    float r0s = length(d0) * s0;
    float t1 = sqrt(dot(d1, d1) - r1 * r1);
    float s1 = r1 / t1;
    float r1s = length(d1) * s1;
    float3 p0 = x0 + r0s * v0;
    float3 p1 = x0 - r0s * v0;
    float3 p2 = x1 + r1s * v1;
    float3 p3 = x1 - r1s * v1;
    float sm = max(s0, s1);
    float r0ss = length(d0) * sm;
    float r1ss = length(d1) * sm;
    float3 v = camDir.xyz;
    float3 w = cross(u, v);
    float a0 = dot(normalize(p0 - e), normalize(w));
    float a2 = dot(normalize(p2 - e), normalize(w));
    float3 ps = (a0 <= a2) ? p0 : p2;
    float rs = (a0 <= a2) ? r0ss : r1ss;
    float a1 = dot(normalize(p1 - e), normalize(w));
    float a3 = dot(normalize(p3 - e), normalize(w));
    float3 pe = (a1 <= a3) ? p3 : p1;
    float re = (a1 <= a3) ? r1ss : r0ss;
    float3 c0 = ps - rs * u;
    float3 c1 = ps + rs * u;
    float3 c2 = pe - re * u;
    float3 c3 = pe + re * u;

    // Clipping
    float3 cx0 = posAPre.xyz;
    float3 cx1 = x0;
    float3 cx2 = x1;
    float3 cx3 = posBSuc.xyz;

    // Find start and end cap flag
    float start = 1.0;
    if (cx0.x < 0.0) {
        start = 0.0;
        cx0 = abs(cx0);
    }
    float end = 1.0;
    if (cx3.x < 0.0) {
        end = 0.0;
        cx3 = abs(cx3);
    }
    float3 n0 = -normalize(cx2 - cx0) * start;
    float3 n1 = normalize(cx3 - cx1) * end;

    float4x4 pvMatrix = mul(uboMatricesAndUserInput.mProjMatrix, uboMatricesAndUserInput.mViewMatrix);
    float2 outRadius = float2(radA, radB);

    GSOutput output;

    output.position = mul(pvMatrix, float4(c0, 1.0));
    output.posWS = c0;
    d = c0 - e;
    output.viewRay = float4(d.xyz, 0);
    output.color = colA;
    output.posA = posA.xyz;
    output.posB = posB.xyz;
    output.rArB = outRadius;
    output.n0 = n0;
    output.n1 = n1;
    outputStream.Append(output);

    output.position = mul(pvMatrix, float4(c1, 1.0));
    output.posWS = c1;
    d = c1 - e;
    output.viewRay = float4(d.xyz, 0);
    output.color = colA;
    output.posA = posA.xyz;
    output.posB = posB.xyz;
    output.rArB = outRadius;
    output.n0 = n0;
    output.n1 = n1;
    outputStream.Append(output);

    output.position = mul(pvMatrix, float4(c2, 1.0));
    output.posWS = c2;
    d = c2 - e;
    output.viewRay = float4(d.xyz, 0);
    output.color = colB;
    output.posA = posA.xyz;
    output.posB = posB.xyz;
    output.rArB = outRadius;
    output.n0 = n0;
    output.n1 = n1;
    outputStream.Append(output);

    output.position = mul(pvMatrix, float4(c3, 1.0));
    output.posWS = c3;
    d = c3 - e;
    output.viewRay = float4(d.xyz, 0);
    output.color = colB;
    output.posA = posA.xyz;
    output.posB = posB.xyz;
    output.rArB = outRadius;
    output.n0 = n0;
    output.n1 = n1;
    outputStream.Append(output);

    outputStream.RestartStrip();
}

uint compute_hash(uint a)
{
    a = (a + 0x7ed55d16) + (a << 12);
    a = (a ^ 0xc761c23c) ^ (a >> 19);
    a = (a + 0x165667b1) + (a << 5);
    a = (a + 0xd3a2646c) ^ (a << 9);
    a = (a + 0xfd7046c5) + (a << 3);
    a = (a ^ 0xb55a4f09) ^ (a >> 16);
    return a;
}

float3 color_from_id_hash(uint a)
{
    uint hash = compute_hash(a);
    return float3(float(hash & 255), float((hash >> 8) & 255), float((hash >> 16) & 255)) / 255.0;
}

// Geometry Shader
[maxvertexcount(4)]
void GSMain(lineadj VertexOutput input[4], inout TriangleStream<GSOutput> outputStream)
{
    float4 colA = float4(1.0, 1.0, 1.0, 1.0);
    float4 colB = float4(1.0, 1.0, 1.0, 1.0);
    float radA = 0.0;
    float radB = 0.0;
    
     // Color calculation
    if (uboMatricesAndUserInput.mVertexColorMode == 0) {
        colA.rgb = uboMatricesAndUserInput.mVertexColorMin.rgb;
        colB.rgb = uboMatricesAndUserInput.mVertexColorMin.rgb;
    }
    // ... (other color mode calculations)

    // Alpha calculation
    if (uboMatricesAndUserInput.mVertexAlphaMode == 0) {
        colA.a = uboMatricesAndUserInput.mVertexAlphaBounds.x;
        colB.a = uboMatricesAndUserInput.mVertexAlphaBounds.x;
    }
    // ... (other alpha mode calculations)

    // Radius calculation
    if (uboMatricesAndUserInput.mVertexRadiusMode == 0) {
        radA = uboMatricesAndUserInput.mVertexRadiusBounds.x;
        radB = uboMatricesAndUserInput.mVertexRadiusBounds.x;
    }
    // ... (other radius mode calculations)

    construct_billboard_for_line(
        input[1].position, input[2].position,
        radA, radB,
        uboMatricesAndUserInput.mCamPos, uboMatricesAndUserInput.mCamDir,
        input[0].position, input[3].position,
        colA, colB,
        outputStream
    );
	/*
	GSOutput output;

    for (int i = 0; i < 4; i++)
    {
        output.position = input[i].position/2;
        //output.tex = input[i].tex;
        
        // You can add additional processing here if needed
        // For example, you could offset the vertices or modify the texture coordinates
        
        outputStream.Append(output);
    }

    outputStream.RestartStrip();*/
	
}

// pixel shader
float4 PSMain(GSOutput p) : SV_TARGET
{
    // output: tex0 + coefficient * tex1
	//float x = float((kBuffer[p.position.x].x*kBuffer[p.position.x].y)/1024.f);
	//float y = float((kBuffer[p.position.y].x*kBuffer[p.position.y].y)/768.f);
    return float4(1,0,1,1); //mad(coefficient, , tex0.Sample(texSampler, p.tex));
}