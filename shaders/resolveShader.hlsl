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
    float2 texCoords : TEXCOORD0;
};

VertexOutput VSMain(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    
    output.texCoords = positions[vertexID];
    output.position = float4(positions[vertexID] * 2.0 - 1.0, 0.0, 1.0);
    
    return output;
}


StructuredBuffer<uint2> kBuffer : register(t0);

cbuffer UniformBlock : register(b0) {
    matrices_and_user_input uboMatricesAndUserInput;
};


struct Samples
{
    float4 color;
    float depth;
};

static const uint K_MAX = 16;

uint listPos(uint i, VertexOutput input)
{
    int2 coord = int2(input.position.xy);
    int3 imgSize = int3(uboMatricesAndUserInput.kBufferInfo.xyz);
    return coord.x + coord.y * imgSize.x + i * (imgSize.x * imgSize.y);
}


// Helper function to unpack unorm4x8
float4 unpack_unorm4_8(uint packedValue)
{
    return float4(
        (packedValue & 0xFF) / 255.0f,
        ((packedValue >> 8) & 0xFF) / 255.0f,
        ((packedValue >> 16) & 0xFF) / 255.0f,
        (packedValue >> 24) / 255.0f
    );
}


float4 PSMain(VertexOutput input) : SV_TARGET
{
    int2 coord = int2(input.position.xy);

    // load k samples from the data buffers
    Samples samples[K_MAX];
	for(uint i = 0; i<K_MAX; ++i){
		samples[i].color = float4(0,0,0,0);
		samples[i].depth = 0;
	}
    int sample_count = 0;
    for (int i = 0; i < uboMatricesAndUserInput.kBufferInfo.z; ++i)
    {
        uint2 value = kBuffer[listPos(i,input)];
        if (value.x == 0xFFFFFFFFu && value.y == 0xFFFFFFFFu)
        {
            break;
        }
        samples[i].color = unpack_unorm4_8(value.y);
        samples[i].depth = asfloat(value.x);
        sample_count++;
    }
	
    if (sample_count == 0)
        discard; //return float4(1,0,0,1);
	//return float4(samples[2].color.rgb,1);
    float3 color = float3(0, 0, 0);
    float alpha = 1;
    for (int i = 0; i < sample_count; ++i)
    {
        color = color * (samples[sample_count - i - 1].color.a) + samples[sample_count - i - 1].color.rgb;
        alpha *= samples[sample_count - i - 1].color.a;
    }

    alpha = 1 - alpha;
    return float4(color / alpha, alpha);
    //return float4(1,0,1,alpha);
}
