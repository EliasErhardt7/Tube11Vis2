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
    
    output.position = mul(mul(float4(input.inPosition, 1.0), uboMatricesAndUserInput.mViewMatrix), uboMatricesAndUserInput.mProjMatrix);
    output.outData = input.inCurvature;
    output.outCurvature = input.inData;
    
    return output;
}

struct GSOutput {
    float4 position : SV_POSITION;
};

[maxvertexcount(4)]
void GSMain(lineadj VertexOutput input[4], inout LineStream<GSOutput> outputStream)
{
    GSOutput output;
    output.position = input[0].position;
    outputStream.Append(output);
    output.position = input[1].position;
    outputStream.Append(output);
    output.position = input[2].position;
    outputStream.Append(output);
	output.position = input[3].position;
    outputStream.Append(output);
    outputStream.RestartStrip();
}

float4 PSMain(VertexOutput input) : SV_TARGET {
    return float4(1,0,1,1);
}