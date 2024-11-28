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


cbuffer UniformBlock : register(b0) {
    matrices_and_user_input uboMatricesAndUserInput;
};

struct VertexInput
{
    float3 inPosition : POSITION;
};


struct VertexOutput
{
    float4 position : SV_POSITION;
};

// Vertex shader main function
VertexOutput VSMain(VertexInput input)
{
	VertexOutput output;
	output.position =  mul(mul(float4(input.inPosition, 1.0),(uboMatricesAndUserInput.mViewMatrix)),uboMatricesAndUserInput.mProjMatrix);
	return output;
}



//pixel shader

float4 PSMain(VertexOutput input) : SV_TARGET {
    return uboMatricesAndUserInput.mHelperLineColor;
}