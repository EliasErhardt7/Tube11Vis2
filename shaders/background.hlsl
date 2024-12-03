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


static const float2 positions[6] = {
    float2(0.0, 0.0),
    float2(0.0, 1.0),
    float2(1.0, 1.0),
    float2(1.0, 1.0),
    float2(1.0, 0.0),
    float2(0.0, 0.0)
};

struct VertexOutput
{
    float4 position : SV_POSITION;
};

VertexOutput VSMain(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    
    output.position = float4(positions[vertexID] * 2.0 - 1.0, 0.0, 1.0);
    
    return output;
}


cbuffer UniformBlock : register(b0) {
    matrices_and_user_input uboMatricesAndUserInput;
};




float4 PSMain(VertexOutput input) : SV_TARGET
{
    float gradientIntensity = 1.0 - uboMatricesAndUserInput.mClearColor.a;
    float3 colorA = lerp(uboMatricesAndUserInput.mClearColor.rgb, float3(0.0, 0.0, 0.0), gradientIntensity);
    float3 colorB = lerp(uboMatricesAndUserInput.mClearColor.rgb, float3(1.0, 1.0, 1.0), gradientIntensity);

    float2 pdc = input.position.xy / uboMatricesAndUserInput.kBufferInfo.xy;

    return float4(lerp(colorA, colorB, pdc.y), 0.0);
}
