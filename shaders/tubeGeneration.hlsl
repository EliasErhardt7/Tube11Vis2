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


RWStructuredBuffer<uint2> kBuffer : register(u1);


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
    
    output.position = float4(input.inPosition, 1.0);//mul(mul(float4(input.inPosition, 1.0), uboMatricesAndUserInput.mViewMatrix), uboMatricesAndUserInput.mProjMatrix);
    output.outData = input.inCurvature;
    output.outCurvature = input.inData;
    
    return output;
}

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
	
    float t0 = sqrt(length(d0)*length(d0) - r0*r0);
    float s0 = r0 / t0;
    float r0s = length(d0) * s0;
	
    float t1 = sqrt(length(d1)*length(d1) - r1*r1);
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

    //float4x4 pvMatrix = mul(uboMatricesAndUserInput.mProjMatrix, uboMatricesAndUserInput.mViewMatrix);
    float2 outRadius = float2(radA, radB);
	
    GSOutput output;
	
	output.color=posA;
	
    output.position = mul(mul(float4(c0, 1.0), uboMatricesAndUserInput.mViewMatrix), uboMatricesAndUserInput.mProjMatrix);//mul(pvMatrix, float4(c0, 1.0));
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

    output.position = mul(mul(float4(c1, 1.0), uboMatricesAndUserInput.mViewMatrix), uboMatricesAndUserInput.mProjMatrix);//mul(pvMatrix, float4(c1, 1.0));
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

    output.position = mul(mul(float4(c2, 1.0), uboMatricesAndUserInput.mViewMatrix), uboMatricesAndUserInput.mProjMatrix);//mul(pvMatrix, float4(c2, 1.0));
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

    output.position = mul(mul(float4(c3, 1.0), uboMatricesAndUserInput.mViewMatrix), uboMatricesAndUserInput.mProjMatrix);//mul(pvMatrix, float4(c3, 1.0));
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
    return float3(
        float(hash & 255),
        float((hash >> 8) & 255),
        float((hash >> 16) & 255)
    ) / 255.0f;
}

[maxvertexcount(4)]
void GSMain(lineadj VertexOutput input[4], inout TriangleStream<GSOutput> outputStream)
{

	if(input[0].position.x == input[1].position.x && input[0].position.y == input[1].position.y && input[0].position.z == input[1].position.z){
		return;
	}
	float4 colA = float4(1.0, 1.0, 1.0, 1.0);
    float4 colB = float4(1.0, 1.0, 1.0, 1.0);
    float radA = 0.0;
    float radB = 0.0;
    
    // Color calculation
    if (uboMatricesAndUserInput.mVertexColorMode == 0) {
        colA.rgb = uboMatricesAndUserInput.mVertexColorMin.rgb;
        colB.rgb = uboMatricesAndUserInput.mVertexColorMin.rgb;
    }
     else if (uboMatricesAndUserInput.mVertexColorMode == 1) {  // based on data
        colA.rgb = lerp(uboMatricesAndUserInput.mVertexColorMin.rgb, uboMatricesAndUserInput.mVertexColorMax.rgb, input[1].outData);
        colB.rgb = lerp(uboMatricesAndUserInput.mVertexColorMin.rgb, uboMatricesAndUserInput.mVertexColorMax.rgb, input[2].outData);
    } else if (uboMatricesAndUserInput.mVertexColorMode == 2) { // based on length
        float factor = distance(input[1].position, input[2].position) / uboMatricesAndUserInput.mDataMaxLineLength;
        colA.rgb = lerp(uboMatricesAndUserInput.mVertexColorMin.rgb, uboMatricesAndUserInput.mVertexColorMax.rgb, factor);
        colB.rgb = colA.rgb;
    } else if (uboMatricesAndUserInput.mVertexColorMode == 3) { // based on curvature
        colA.rgb = lerp(uboMatricesAndUserInput.mVertexColorMin.rgb, uboMatricesAndUserInput.mVertexColorMax.rgb, input[1].outCurvature);
        colB.rgb = lerp(uboMatricesAndUserInput.mVertexColorMin.rgb, uboMatricesAndUserInput.mVertexColorMax.rgb, input[2].outCurvature);
    }

    // Alpha calculation
    if (uboMatricesAndUserInput.mVertexAlphaMode == 0) {
        colA.a = uboMatricesAndUserInput.mVertexAlphaBounds.x;
        colB.a = uboMatricesAndUserInput.mVertexAlphaBounds.x;
    }
    else if (uboMatricesAndUserInput.mVertexAlphaMode == 1) {  // based on data
        colA.a = lerp(uboMatricesAndUserInput.mVertexAlphaBounds.x, uboMatricesAndUserInput.mVertexAlphaBounds.y, uboMatricesAndUserInput.mVertexAlphaInvert ? 1 - input[1].outData : input[1].outData);
        colB.a = lerp(uboMatricesAndUserInput.mVertexAlphaBounds.x, uboMatricesAndUserInput.mVertexAlphaBounds.y, uboMatricesAndUserInput.mVertexAlphaInvert ? 1 - input[2].outData : input[2].outData);
    } else if (uboMatricesAndUserInput.mVertexAlphaMode == 2) { // based on length
        float factor = distance(input[1].position, input[2].position) / uboMatricesAndUserInput.mDataMaxLineLength;
        factor = uboMatricesAndUserInput.mVertexAlphaInvert ? 1 - factor : factor;
        colA.a = lerp(uboMatricesAndUserInput.mVertexAlphaBounds.x, uboMatricesAndUserInput.mVertexAlphaBounds.y, input[1].outData);
        colB.a = colA.a;
    } else if (uboMatricesAndUserInput.mVertexAlphaMode == 3) { // based on curvature
        colA.a = lerp(uboMatricesAndUserInput.mVertexAlphaBounds.x, uboMatricesAndUserInput.mVertexAlphaBounds.y, uboMatricesAndUserInput.mVertexAlphaInvert ? 1 - input[1].outCurvature : input[1].outCurvature);
        colB.a = lerp(uboMatricesAndUserInput.mVertexAlphaBounds.x, uboMatricesAndUserInput.mVertexAlphaBounds.y, uboMatricesAndUserInput.mVertexAlphaInvert ? 1 - input[2].outCurvature : input[2].outCurvature);
    }

    // Radius calculation
    if (uboMatricesAndUserInput.mVertexRadiusMode == 0) {
        radA = uboMatricesAndUserInput.mVertexRadiusBounds.x;
        radB = uboMatricesAndUserInput.mVertexRadiusBounds.x;
    }
    else if (uboMatricesAndUserInput.mVertexRadiusMode == 1) {  // based on data
        radA = lerp(uboMatricesAndUserInput.mVertexRadiusBounds.x, uboMatricesAndUserInput.mVertexRadiusBounds.y, uboMatricesAndUserInput.mVertexRadiusInvert ? 1 - input[1].outData : input[1].outData);
        radB = lerp(uboMatricesAndUserInput.mVertexRadiusBounds.x, uboMatricesAndUserInput.mVertexRadiusBounds.y, uboMatricesAndUserInput.mVertexRadiusInvert ? 1 - input[2].outData : input[2].outData);
    } else if (uboMatricesAndUserInput.mVertexRadiusMode == 2) { // based on length
        float factor = distance(input[1].position, input[2].position) / uboMatricesAndUserInput.mDataMaxLineLength;
        factor = uboMatricesAndUserInput.mVertexRadiusInvert ? 1 - factor : factor;
        radA = lerp(uboMatricesAndUserInput.mVertexRadiusBounds.x, uboMatricesAndUserInput.mVertexRadiusBounds.y, factor);
        radB = radA;
    } else if (uboMatricesAndUserInput.mVertexRadiusMode == 3) { // based on curvature
        radA = lerp(uboMatricesAndUserInput.mVertexRadiusBounds.x, uboMatricesAndUserInput.mVertexRadiusBounds.y, uboMatricesAndUserInput.mVertexRadiusInvert ? 1 - input[1].outCurvature : input[1].outCurvature);
        radB = lerp(uboMatricesAndUserInput.mVertexRadiusBounds.x, uboMatricesAndUserInput.mVertexRadiusBounds.y, uboMatricesAndUserInput.mVertexRadiusInvert ? 1 - input[2].outCurvature : input[2].outCurvature);
    }
	
	construct_billboard_for_line(
        input[1].position, input[2].position,
        radA, radB,
        uboMatricesAndUserInput.mCamPos, uboMatricesAndUserInput.mCamDir,
        input[0].position, input[3].position,
        colA, colB,
        outputStream
    );
}

//pixel shader

uint listPos(uint i, GSOutput input) {
    int2 coord = int2(input.position.xy);
    int3 imgSize = int3(uboMatricesAndUserInput.kBufferInfo.xyz);
	
	//if (coord.x >= imgSize.x || coord.y >= imgSize.y)
    //    return 0;
		
    return coord.x + coord.y * (imgSize.x) + i * (imgSize.x * imgSize.y);
}

float4 iRoundedCone(float3 ro, float3 rd, float3 pa, float3 pb, float ra, float rb) {
    float3  ba = pb - pa;
	float3  oa = ro - pa;
	float3  ob = ro - pb;
    float rr = ra - rb;
    float m0 = dot(ba,ba);
    float m1 = dot(ba,oa);
    float m2 = dot(ba,rd);
    float m3 = dot(rd,oa);
    float m5 = dot(oa,oa);
	float m6 = dot(ob,rd);
    float m7 = dot(ob,ob);
    
    float d2 = m0-rr*rr;
    
	float k2 = d2 - m2*m2;
    float k1 = d2*m3 - m1*m2 + m2*rr*ra;
    float k0 = d2*m5 - m1*m1 + m1*rr*ra*2.0 - m0*ra*ra;
    
	float h = k1*k1 - k0*k2;
	if(h < 0.0) return float4(-1.0,-1.0,-1.0,-1.0);
    float t = (-sqrt(h)-k1)/k2;
    if( t<0.0 ) return float4(-1.0,-1.0,-1.0,-1.0);

    float y = m1 - ra*rr + t*m2;
    if( y>0.0 && y<d2 ) 
    {
        return float4(t, normalize( d2*(oa + t*rd)-ba*y) );
    }

    // Caps. I feel this can be done with a single square root instead of two
    float h1 = m3*m3 - m5 + ra*ra;
    float h2 = m6*m6 - m7 + rb*rb;
    if( max(h1,h2)<0.0 ) return float4(-1.0,-1.0,-1.0,-1.0);
    
    float4 r = float4(1e20,1e20,1e20,1e20);
    if( h1>0.0 )
    {        
    	t = -m3 - sqrt( h1 );
        r = float4( t, (oa+t*rd)/ra );
    }
	if( h2>0.0 )
    {
    	t = -m6 - sqrt( h2 );
        if( t<r.x )
        r = float4( t, (ob+t*rd)/rb );
    }
    return r;
}

float get_distance_from_plane(float3 points, float4 planes) {
    return dot(planes.xyz, points.xyz) - planes.w;
}

float3 calc_blinn_phong_contribution(float3 toLight, float3 toEye, float3 normal, float3 diffFactor, float3 specFactor, float specShininess) {
    float nDotL = max(0.0, dot(normal, toLight)); // lambertian coefficient
	float3 h = normalize(toLight + toEye);
	float nDotH = max(0.0, dot(normal, h));
	float specPower = pow(nDotH, specShininess);
	float3 diffuse = diffFactor * nDotL; // component-wise product
	float3 specular = specFactor * specPower;
	return diffuse + specular;
}

float3 calculate_illumination(float3 albedo, float3 eyePos, float3 fragPos, float3 fragNorm, GSOutput input) {
    float4 mMaterialLightReponse = uboMatricesAndUserInput.mMaterialLightReponse;
    float3 dirColor = uboMatricesAndUserInput.mDirLightColor.rgb;
    float3 ambient = mMaterialLightReponse.x * input.color.rgb;
    float3 diff = mMaterialLightReponse.y * input.color.rgb;
    float3 spec = mMaterialLightReponse.zzz;
    float shini = mMaterialLightReponse.w;

    float3 ambientIllumination = ambient * uboMatricesAndUserInput.mAmbLightColor.rgb;
    
    float3 toLightDirWS = -uboMatricesAndUserInput.mDirLightDirection.xyz;
    float3 toEyeNrmWS = normalize(eyePos - fragPos);
    float3 diffAndSpecIllumination = dirColor * calc_blinn_phong_contribution(toLightDirWS, toEyeNrmWS, fragNorm, diff, spec, shini);

    return ambientIllumination + diffAndSpecIllumination;
}

uint packUnorm4x8(float4 v)
{
    uint4 scale = uint4(255.0f, 255.0f, 255.0f, 255.0f);
    uint4 rounded = uint4(round(saturate(v) * scale));
    return rounded.x | (rounded.y << 8) | (rounded.z << 16) | (rounded.w << 24);
}

uint2 pack(float depth, float4 color) {

	uint2 packedColor = uint2(asuint(depth),packUnorm4x8(color));
	return packedColor;
}

float4 unpackUnorm4x8(uint packedValue)
{
    return float4(
        (packedValue & 0xFF) / 255.0f,
        ((packedValue >> 8) & 0xFF) / 255.0f,
        ((packedValue >> 16) & 0xFF) / 255.0f,
        (packedValue >> 24) / 255.0f
    );
}

void unpack(uint2 data, out float depth, out float4 color) {

	color=unpackUnorm4x8(data.y);
	depth=asfloat(data.x);
}

float random (float2 uv)
{
	return frac(sin(dot(uv,float2(12.9898,78.233)))*43758.5453123);
}

float4 PSMain(GSOutput input) : SV_TARGET {
    uint2 coord = uint2(input.position.xy);

    float3 camWS = uboMatricesAndUserInput.mCamPos.xyz;
    float3 viewRayWS = normalize(input.viewRay.xyz);

    float4 tnor = iRoundedCone(camWS, viewRayWS, input.posA, input.posB, input.rArB.x, input.rArB.y);
    float3 posWsOnCone = camWS + viewRayWS * tnor.x;

    if (uboMatricesAndUserInput.mBillboardClippingEnabled) {
        if (tnor.x <= 0.0) discard;
        
        float4 plane1 = float4(input.n0, dot(input.posA, input.n0));
        float4 plane2 = float4(input.n1, dot(input.posB, input.n1));
        float dp1 = get_distance_from_plane(posWsOnCone, plane1);
        float dp2 = get_distance_from_plane(posWsOnCone, plane2);
        if (dp1 > 0 || dp2 > 0) discard;
    }
	
    float3 illumination = input.color.rgb;
    if (uboMatricesAndUserInput.mBillboardShadingEnabled) {
        illumination = calculate_illumination(input.color.rgb, camWS, posWsOnCone, tnor.yzw, input);
    }
    float4 color = float4(illumination * input.color.a, 1 - input.color.a);

    uint2 value = pack(input.position.z *2.0 -1.0, color);
	
    bool insert = true;
    if (value.x > kBuffer[listPos(int(uboMatricesAndUserInput.kBufferInfo.z) - 1, input)].x) {
        insert = false;
    } else if (value.x == kBuffer[listPos(int(uboMatricesAndUserInput.kBufferInfo.z) - 1, input)].x && value.y > kBuffer[listPos(int(uboMatricesAndUserInput.kBufferInfo.z) - 1, input)].y){
		insert = false;
	}
	
	
	float4 unpackedColor = float4(0,0,0,0);
	float alpha = 1;
	float4 outColor = float4(0,0,0,0);
    if (insert) {
        for (uint i = 0; i < uint(uboMatricesAndUserInput.kBufferInfo.z); ++i) {
			uint2 old = uint2(0,0);
				
			InterlockedMin(kBuffer[listPos(i, input)].x, value.x, old.x);
			//InterlockedMin(kBuffer[listPos(i, input)].y, value.y, old.y);
			
			old.y = kBuffer[listPos(i, input)].y;
			if(value.x<old.x){
				
				kBuffer[listPos(i, input)].y = value.y;
				
			}
			
            if (old.x == 0xFFFFFFFFu && old.y == 0xFFFFFFFFu) {
				
                break;
            }

            value = max(old, value);
			
            if (i == ((uboMatricesAndUserInput.kBufferInfo.z) - 1)) {
				
                if (value.x != 0xFFFFFFFFu || value.y != 0xFFFFFFFFu ) {
					
                    float depth = 0;
                    
                    unpack(value, depth, unpackedColor);
					
                    alpha = 1 - unpackedColor.a;
                    outColor = float4(unpackedColor.xyz / alpha, alpha);
                } else {
                    discard;

                }
            }
        }
    }
	return outColor;
	
}