#pragma once

#include <d3d11.h>

struct ShaderProgram
{
    // binary blobs for vertex and pixel shader
    ID3D10Blob* vsBlob;
    ID3D10Blob* gsBlob;
    ID3D10Blob* psBlob;

    // vertex and pixel shader
    ID3D11VertexShader* vShader;
    ID3D11GeometryShader* gShader;
    ID3D11PixelShader* pShader;
};

struct ComputeShader
{
    // binary blob
    ID3D10Blob* csBlob;

    // compute shader
    ID3D11ComputeShader* cShader;
};

// render target consisting of texture, view for use as render target, as well as SRV and UAV
struct RenderTarget
{
    ID3D11Texture2D* renderTargetTexture;
    ID3D11RenderTargetView* renderTargetView;
    ID3D11ShaderResourceView* shaderResourceView;
    ID3D11UnorderedAccessView* unorderedAccessView;
};

struct StorageTarget
{
    ID3D11Buffer* structuredBuffer;
    ID3D11ShaderResourceView* shaderResourceView;
    ID3D11UnorderedAccessView* unorderedAccessView;
};

struct DepthStencilTarget
{
    ID3D11Texture2D* dsTexture;
    ID3D11DepthStencilView* dsView;
};

struct Mesh
{
    ID3D11Buffer* vertexBuffer;
    ID3D11InputLayout* vertexLayout;

    UINT vertexCount;
    UINT stride;
    UINT offset;
    D3D_PRIMITIVE_TOPOLOGY topology;
};

struct Transformations
{
    DirectX::XMMATRIX model;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX proj;
};

struct LightSource
{
    // light position in view space
    DirectX::XMFLOAT4 lightPosition;
    // RGB color and the light power in the w-coordinate
    DirectX::XMFLOAT4 lightColorAndPower;
};

struct Material
{
    // rgb components contain ambient and diffuse colors
    DirectX::XMFLOAT4 ambient;
    DirectX::XMFLOAT4 diffuse;
    // rgb contains color, w-coordinate contains specular exponent
    DirectX::XMFLOAT4 specularAndShininess;
};

struct ThresholdParams
{
    alignas(16) float threshold;
};

struct matrices_and_user_input
{
	/// <summary>
	/// The view matrix given by the quake cam
	/// </summary>
	DirectX::XMMATRIX mViewMatrix;
	/// <summary>
	/// The projection matrix given by the quake cam
	/// </summary>
	DirectX::XMMATRIX mProjMatrix;
	/// <summary>
	/// The position of the camera/eye in world space
	/// </summary>
	DirectX::XMVECTOR mCamPos;
	/// <summary>
	/// The looking direction of the camera/eye in world space
	/// </summary>
	DirectX::XMVECTOR mCamDir;
	/// <summary>
	/// rgb ... The background color for the background shader
	/// a   ... The strength of the gradient
	/// </summary>
	DirectX::XMVECTOR mClearColor;
	/// <summary>
	/// rgb ... The color for the 2d helper lines.
	/// a   ... ignored
	/// </summary>
	DirectX::XMVECTOR mHelperLineColor;
	/// <summary>
	/// contains resx, resy and kbuffer levels
	/// </summary>
	DirectX::XMVECTOR mkBufferInfo;

	/// <summary>
	/// The direction of the light/sun in WS
	/// </summary>
	DirectX::XMFLOAT4 mDirLightDirection;
	/// <summary>
	/// The color of the light/sun multiplied with the intensity
	/// a   ... ignored
	/// </summary>
	DirectX::XMFLOAT4 mDirLightColor;
	/// <summary>
	/// The color of the ambient light
	/// a   ... ignored
	/// </summary>
	DirectX::XMFLOAT4 mAmbLightColor;
	/// <summary>
	/// The material light properties for the tubes:
	/// r ... ambient light factor
	/// g ... diffuse light factor
	/// b ... specular light factor
	/// a ... shininess
	/// </summary>
	DirectX::XMFLOAT4 mMaterialLightReponse; // vec4(0.5, 1.0, 0.5, 32.0);  // amb, diff, spec, shininess

	/// <summary>
	/// The vertex color for minimum values (depending on the mode).
	/// Is also used for the color if in static mode
	/// a ... ignored
	/// </summary>
	DirectX::XMFLOAT4 mVertexColorMin;
	/// <summary>
	/// The vertex color for vertices with maximum values (depending on the mode)
	/// a ... ignored
	/// </summary>
	DirectX::XMFLOAT4 mVertexColorMax;
	/// <summary>
	/// The min/max levels for line transparencies in dynamic modes
	/// The min value is also used if in static mode
	/// ba ... ignored
	/// </summary>
	DirectX::XMFLOAT4 mVertexAlphaBounds;
	/// <summary>
	/// The min/max level for the radius of vertices in dynamic modes
	/// The min value is also used if in static mode
	/// ba ... ignored
	/// </summary>
	DirectX::XMFLOAT4 mVertexRadiusBounds;

	/// <summary>
	/// Flag to enable/disable the clipping of the billboard based on the raycasting
	/// and for the caps.
	/// </summary>
	BOOL mBillboardClippingEnabled;
	/// <summary>
	/// Flag to enable/disable shading of the billboard (raycasting will still be done for possible clipping)
	/// </summary>
	BOOL mBillboardShadingEnabled;

	/// <summary>
	/// The coloring mode for vertices (see main->renderUI() for possible states)
	/// </summary>
	uint32_t mVertexColorMode;
	/// <summary>
	/// The transparency mode for vertices (see main->renderUI() for possible states)
	/// </summary>
	uint32_t mVertexAlphaMode;
	/// <summary>
	/// The radius mode for vertices (see main->renderUI() for possible states)
	/// </summary>
	uint32_t mVertexRadiusMode;

	/// <summary>
	/// Reverses the factor (depending on the mode) for dynamic transparency if true
	/// </summary>
	BOOL mVertexAlphaInvert;
	/// <summary>
	/// Reverses the factor (depending on the mode) for dynamic radius if true
	/// </summary>
	BOOL mVertexRadiusInvert;
	/// <summary>
	/// The maximum line length inside the dataset. Which is necessary to calculate a
	/// factor from 0-1 in the depending on line length mode.
	/// </summary>
	float mDataMaxLineLength;
	/// <summary>
	/// The maximum line length of adjacing lines to a vertex inside the dataset.
	/// This value is unused so far but could be used for another dynamic mode
	/// </summary>
	float mDataMaxVertexAdjacentLineLength;

};


// (GAUSSIAN_RADIUS + 1) must be multiple of 4 because of the way we set up the shader
#define GAUSSIAN_RADIUS 7

struct ComputeInfo
{
    DirectX::XMVECTOR kBufferInfo;
};
