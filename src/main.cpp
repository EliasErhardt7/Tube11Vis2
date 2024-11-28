#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <array>
#include <iostream>
#include <sstream>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "DirectXTK.lib")
#pragma comment (lib, "d3dcompiler.lib")

#include "geometry.h"
#include "resource.h"
#include "util/timer.h"
#include "camera.h"

///////////////////////
// global declarations

constexpr int WIDTH = 1024;
constexpr int HEIGHT = 768;
constexpr size_t NUM_RENDERTARGETS = 3;
constexpr size_t NUM_STORAGETARGETS = 1;
bool isRightButtonDown = false;
// timer for retrieving delta time between frames
Timer timer;

Camera camera;

// DirectX resources: swapchain, device, and device context
IDXGISwapChain *swapchain;
ID3D11Device *device;
ID3D11DeviceContext *deviceContext;

// backbuffer obtained from the swapchain
ID3D11RenderTargetView *backbuffer;

// shaders
ShaderProgram testShader;
ComputeShader testComputeShader;
ShaderProgram modelShader;
ShaderProgram quadCompositeShader;
ShaderProgram linesShader;
ComputeShader thresholdDownsampleShader;
ComputeShader computeShader;

// render targets and depth-stencil target
RenderTarget renderTargets[NUM_RENDERTARGETS];
StorageTarget storageTargets[NUM_STORAGETARGETS];
DepthStencilTarget depthStencilTarget;

// depth-stencil states
ID3D11DepthStencilState* depthStencilStateWithDepthTest;
ID3D11DepthStencilState* depthStencilStateWithoutDepthTest;

// default texture sampler
ID3D11SamplerState* defaultSamplerState;

// default rasterizer state
ID3D11RasterizerState* defaultRasterizerState;

// meshes
Mesh objModelMesh;
Mesh screenAlignedQuadMesh;
Mesh vertexTubeMesh;
Mesh lineTubeMesh;

// model, view, and projection transform
Transformations transforms;
ID3D11Buffer* transformConstantBuffer;

// material and light source
Material material;
ID3D11Buffer* materialConstantBuffer;

LightSource lightSource;
ID3D11Buffer* lightSourceConstantBuffer;

// parameters for compute shaders
ID3D11Buffer* thresholdConstantBuffer;

ComputeInfo computeInfo;
ID3D11Buffer* computeConstantBuffer;

ID3D11Buffer* compositionConstantBuffer;

bool mBillboardClippingEnabled = true;
bool mBillboardShadingEnabled = true;

XMVECTOR mDirLightDirection = { -0.7F, -0.6F, -0.3F, 0.0F };
XMVECTOR mDirLightColor = { 1.0F, 1.0F, 1.0F, 1.0F };
float mDirLightIntensity = 1.0F;
XMVECTOR mAmbLightColor = { 0.05F, 0.05F, 0.05F, 1.0F };
XMVECTOR mMaterialReflectivity= { 0.5, 1.0, 0.5, 32.0 };

int mVertexColorMode = 0;
XMVECTOR mVertexColorStatic = { 65.0F / 255.0F, 105.0F / 255.0F, 225.0F / 255.0F, 1.f };
XMVECTOR mVertexColorMin = { 1.0F / 255.0F, 169.0F / 255.0F, 51.0F / 255.0F, 1.f };
XMVECTOR mVertexColorMax = { 1.0F, 0.0F, 0.0F, 1.f };

int mVertexAlphaMode = 0;
bool mVertexAlphaInvert = false;
float mVertexAlphaStatic = 0.5;
XMVECTOR mVertexAlphaBounds = { 0.5, 0.8 , 0.0, 0.f};

int mVertexRadiusMode = 0;
bool mVertexRadiusInvert = false;
float mVertexRadiusStatic = 0.001;
XMVECTOR mVertexRadiusBounds = { 0.001, 0.002, 0.0, 0.0 };

long mRenderCallCount = 0;
int mkBufferLayer = 8;

//
///////////////////////

///////////////////////
// global functions

// Direct3D initialization and shutdown
void InitD3D(HWND hWnd);
void ShutdownD3D(void);

// Rendering data initialization and clean-up
void InitRenderData();
void CleanUpRenderData();

// update tick for render data (e.g., to update transformation matrices)
void UpdateTick(float deltaTime);
// rendering
void RenderFrame();


// WindowProc callback function
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

//
///////////////////////

// entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // window handle and information
    HWND hWnd = nullptr;
    WNDCLASSEX wc = { };

    // fill in the struct with required information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = "DirectX Window";

    // register the window class
    RegisterClassEx(&wc);

    // set and adjust the size
    RECT wr = { 0, 0, WIDTH, HEIGHT };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    // create the window and use the result as the handle
    hWnd = CreateWindowEx(NULL,
        "DirectX Window",           // name of the window class
        "DirectX 11 Playground",    // title of the window
        WS_OVERLAPPEDWINDOW,        // window style
        200,                        // x-position of the window
        100,                        // y-position of the window
        wr.right - wr.left,         // width of the window
        wr.bottom - wr.top,         // height of the window
        NULL,                       // we have no parent window, NULL
        NULL,                       // we aren't using menus, NULL
        hInstance,                  // application handle
        NULL);                      // used with multiple windows, NULL

    ShowWindow(hWnd, nCmdShow);

    InitD3D(hWnd);

    InitRenderData();

    // main loop
    timer.Start();

    MSG msg = { };
    while (true)
    {
        // Check to see if any messages are waiting in the queue
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // translate and send the message to the WindowProc function
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            // handle quit
            if (msg.message == WM_QUIT)
            {
                break;
            }
        }
        else
        {
            // get time elapsed since last frame start
            timer.Stop();
            float elapsedMilliseconds = timer.GetElapsedTimeMilliseconds();
            timer.Start();

            // upate and render
            UpdateTick(elapsedMilliseconds / 1000.0f);

            RenderFrame();
        }
    }

    // shutdown
    CleanUpRenderData();
    ShutdownD3D();

    return 0;
}

void UpdateTick(float deltaTime)
{
    // Camera movement speed
    const float cameraSpeed = 2.0f * deltaTime;
    const float rotationSpeed = 2.0f * deltaTime;

    // Forward/Backward movement
    if (GetAsyncKeyState('W') & 0x8000) camera.MoveForward(cameraSpeed);
    if (GetAsyncKeyState('S') & 0x8000) camera.MoveForward(-cameraSpeed);

    // Left/Right movement
    if (GetAsyncKeyState('A') & 0x8000) camera.MoveRight(cameraSpeed);
    if (GetAsyncKeyState('D') & 0x8000) camera.MoveRight(-cameraSpeed);
    // Up/Down movement
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) camera.MoveUp(cameraSpeed);
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) camera.MoveUp(-cameraSpeed);

    // Rotation
    if (GetAsyncKeyState('E') & 0x8000) camera.RotateY(rotationSpeed);
    if (GetAsyncKeyState('Q') & 0x8000) camera.RotateY(-rotationSpeed);

    // Update view transform
    transforms.view = DirectX::XMMatrixTranspose(camera.GetViewMatrix());

    // Update projection transform (unchanged)
    transforms.proj = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(1.0f, static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), 0.02f, 100.f));

    // Update model transform (rotation)
    constexpr float millisecondsToAngle = 0.0001f * 6.28f;
    transforms.model = DirectX::XMMatrixMultiply(transforms.model, DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationY(deltaTime * millisecondsToAngle)));

    // Update light source (as before)
    DirectX::XMVECTOR lightWorldPos = DirectX::XMVectorSet(-1.5f, 1.5f, 1.5f, 1.f);
    DirectX::XMVECTOR lightViewPos = DirectX::XMVector4Transform(lightWorldPos, transforms.view);
    DirectX::XMStoreFloat4(&lightSource.lightPosition, lightViewPos);
    lightSource.lightColorAndPower = DirectX::XMFLOAT4(1.f, 1.f, 0.7f, 4.5f);
}

void RenderFrame()
{
    constexpr ID3D11RenderTargetView* NULL_RT = nullptr;
    constexpr ID3D11ShaderResourceView* NULL_SRV = nullptr;
    constexpr ID3D11UnorderedAccessView* NULL_UAV = nullptr;
    constexpr UINT NO_OFFSET = -1;

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };  // Black background
    deviceContext->ClearRenderTargetView(backbuffer, clearColor);
    deviceContext->ClearDepthStencilView(depthStencilTarget.dsView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    deviceContext->CSSetShader(computeShader.cShader, 0, 0);
    std::array<ID3D11UnorderedAccessView*, 1> csUAVs = { storageTargets[0].unorderedAccessView };
    ID3D11UnorderedAccessView* kBuffer = storageTargets[0].unorderedAccessView;
    {

        D3D11_MAPPED_SUBRESOURCE ms;
        size_t test = sizeof(ComputeInfo);
        deviceContext->Map(computeConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        memcpy(ms.pData, &computeInfo, sizeof(ComputeInfo));
        deviceContext->Unmap(computeConstantBuffer, 0);


        deviceContext->CSSetUnorderedAccessViews(0, 1, &csUAVs[0], &NO_OFFSET);


        deviceContext->CSSetConstantBuffers(0, 1, &computeConstantBuffer);

        deviceContext->Dispatch(WIDTH / 8, HEIGHT / 8, 1);

        // unbind UAV and SRVs

        deviceContext->CSSetUnorderedAccessViews(0, 1, &NULL_UAV, &NO_OFFSET);
    }
    
    deviceContext->OMSetRenderTargets(1, &backbuffer, NULL);
    
    deviceContext->VSSetShader(quadCompositeShader.vShader, 0, 0);
    deviceContext->GSSetShader(quadCompositeShader.gShader, 0, 0);
    deviceContext->PSSetShader(quadCompositeShader.pShader, 0, 0);

    deviceContext->IASetInputLayout(vertexTubeMesh.vertexLayout);
    deviceContext->IASetVertexBuffers(0, 1, &vertexTubeMesh.vertexBuffer, &vertexTubeMesh.stride, &vertexTubeMesh.offset);
    deviceContext->IASetPrimitiveTopology(vertexTubeMesh.topology);

    std::array<ID3D11ShaderResourceView*, 1> compositeSRVs = { storageTargets[0].shaderResourceView };
    deviceContext->PSSetShaderResources(0, 1, &compositeSRVs[0]);

    //deviceContext->PSSetSamplers(0, 1, &defaultSamplerState);

    matrices_and_user_input uni;
    uni.mViewMatrix = transforms.view;
    uni.mProjMatrix = transforms.proj;
    uni.mCamPos = camera.GetPosition();
    uni.mCamDir = camera.GetForward();
    XMVECTOR color = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    uni.mClearColor = color;
    XMVECTOR colorHelper = XMVectorSet(64.0f / 255.0f, 224.0f / 255.0f, 208.0f / 255.0f, 1.0f);
    uni.mHelperLineColor = colorHelper;
    uni.mkBufferInfo = XMVectorSet(WIDTH, HEIGHT, mkBufferLayer, 0);
    uni.mDirLightDirection = mDirLightDirection;
    uni.mDirLightColor = mDirLightIntensity * mDirLightColor;
    uni.mAmbLightColor = mAmbLightColor;
    uni.mMaterialLightReponse = mMaterialReflectivity;
    uni.mBillboardClippingEnabled = mBillboardClippingEnabled;
    uni.mBillboardShadingEnabled = mBillboardShadingEnabled;
    uni.mVertexColorMode = mVertexColorMode;
    if (mVertexColorMode == 0)
        uni.mVertexColorMin = mVertexColorStatic;
    else {
        uni.mVertexColorMin = mVertexColorMin;
        uni.mVertexColorMax = mVertexColorMax;
    }
    uni.mVertexAlphaMode = mVertexAlphaMode;
    if (mVertexAlphaMode == 0)
        uni.mVertexAlphaBounds = mVertexAlphaBounds;//uni.mVertexAlphaBounds[0] = mVertexAlphaStatic;
    else
        uni.mVertexAlphaBounds = mVertexAlphaBounds;

    uni.mVertexRadiusMode = mVertexRadiusMode;
    if (mVertexRadiusMode == 0)
        uni.mVertexRadiusBounds = mVertexRadiusBounds;//uni.mVertexRadiusBounds[0] = mVertexRadiusStatic;
    else
        uni.mVertexRadiusBounds = mVertexRadiusBounds;
    uni.mDataMaxLineLength = 1.f;
    uni.mDataMaxVertexAdjacentLineLength = 1.f;
    uni.mVertexAlphaInvert = mVertexAlphaInvert;
    uni.mVertexRadiusInvert = mVertexRadiusInvert;


    {
        D3D11_MAPPED_SUBRESOURCE ms;
        deviceContext->Map(compositionConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        memcpy(ms.pData, &uni, sizeof(matrices_and_user_input));
        deviceContext->Unmap(compositionConstantBuffer, 0);
    }
    deviceContext->VSSetConstantBuffers(0, 1, &compositionConstantBuffer);
    deviceContext->GSSetConstantBuffers(0, 1, &compositionConstantBuffer);
    deviceContext->PSSetConstantBuffers(0, 1, &compositionConstantBuffer);

    deviceContext->Draw(vertexTubeMesh.vertexCount, 0);

    //unbind SRVs
    deviceContext->PSSetShaderResources(0, 1, &NULL_SRV);

    // LINES SHADER
    /*
    deviceContext->VSSetShader(linesShader.vShader, 0, 0);
    deviceContext->PSSetShader(linesShader.pShader, 0, 0);

    deviceContext->IASetInputLayout(lineTubeMesh.vertexLayout);
    deviceContext->IASetVertexBuffers(0, 1, &lineTubeMesh.vertexBuffer, &lineTubeMesh.stride, &lineTubeMesh.offset);
    deviceContext->IASetPrimitiveTopology(lineTubeMesh.topology);

    deviceContext->VSSetConstantBuffers(0, 1, &compositionConstantBuffer);
    deviceContext->PSSetConstantBuffers(0, 1, &compositionConstantBuffer);

    deviceContext->Draw(lineTubeMesh.vertexCount, 0);*/
    // switch the back buffer and the front buffer
    swapchain->Present(0, 0);
}

void InitRenderData()
{
    HRESULT result = S_OK;


    // ----------------------- STORAGE TARGET --------------------------------------------
    {
        D3D11_BUFFER_DESC bufferDesc;
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;

        ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
        ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
        ZeroMemory(&uavDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));

        bufferDesc.ByteWidth = WIDTH * HEIGHT * sizeof(UINT64);
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bufferDesc.StructureByteStride = sizeof(UINT64);

        result = device->CreateBuffer(&bufferDesc, NULL, &storageTargets[0].structuredBuffer);
        if (FAILED(result))
        {
            std::cerr << "Failed to create render target texture\n";
            exit(-1);
        }

        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = WIDTH * HEIGHT;
        
        result = device->CreateShaderResourceView(storageTargets[0].structuredBuffer, &srvDesc, &storageTargets[0].shaderResourceView);
        if (FAILED(result))
        {
            std::cerr << "Failed to create render target texture SRV\n";
            exit(-1);
        }

        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = WIDTH * HEIGHT;

        result = device->CreateUnorderedAccessView(storageTargets[0].structuredBuffer, &uavDesc, &storageTargets[0].unorderedAccessView);
        if (FAILED(result))
        {
            std::cerr << "Failed to create render target texture UAV\n";
            exit(-1);
        }
    }

    // ----------------------- STORAGE TARGET END--------------------------------------------

    // intialize depth-stencil target
    {
        D3D11_TEXTURE2D_DESC descDepth;
        ZeroMemory(&descDepth, sizeof(D3D11_TEXTURE2D_DESC));

        descDepth.Width = WIDTH;
        descDepth.Height = HEIGHT;
        descDepth.MipLevels = 1;
        descDepth.ArraySize = 1;
        descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        descDepth.SampleDesc.Count = 1;
        descDepth.SampleDesc.Quality = 0;
        descDepth.Usage = D3D11_USAGE_DEFAULT;
        descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        descDepth.CPUAccessFlags = 0;
        descDepth.MiscFlags = 0;

        result = device->CreateTexture2D(&descDepth, NULL, &depthStencilTarget.dsTexture);
        if (FAILED(result))
        {
            std::cerr << "Failed to create depth/stencil texture\n";
            exit(-1);
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
        ZeroMemory(&descDSV, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));

        descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        descDSV.Texture2D.MipSlice = 0;

        result = device->CreateDepthStencilView(depthStencilTarget.dsTexture, &descDSV, &depthStencilTarget.dsView);
        if (FAILED(result))
        {
            std::cerr << "Failed to create depth/stencil view\n";
            exit(-1);
        }
    }

    // initialize depth-stencil state
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc;
        ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

        dsDesc.DepthEnable = true;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
        dsDesc.StencilEnable = false;

        result = device->CreateDepthStencilState(&dsDesc, &depthStencilStateWithDepthTest);
        if (FAILED(result))
        {
            std::cerr << "Failed to create depth/stencil state\n";
            exit(-1);
        }

        dsDesc.DepthEnable = false;

        result = device->CreateDepthStencilState(&dsDesc, &depthStencilStateWithoutDepthTest);
        if (FAILED(result))
        {
            std::cerr << "Failed to create depth/stencil state\n";
            exit(-1);
        }
    }

    // initialize rasterizer state
    {
        // Setup the raster description which will determine how and what polygons will be drawn.
        D3D11_RASTERIZER_DESC rasterDesc;
        ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));

        rasterDesc.AntialiasedLineEnable = false;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthBias = 0;
        rasterDesc.DepthBiasClamp = 0.0f;
        rasterDesc.DepthClipEnable = true;
        rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
        rasterDesc.FrontCounterClockwise = false;
        rasterDesc.MultisampleEnable = false;
        rasterDesc.ScissorEnable = false;
        rasterDesc.SlopeScaledDepthBias = 0.f;

        // Create the rasterizer state from the description we just filled out.
        result = device->CreateRasterizerState(&rasterDesc, &defaultRasterizerState);
        if (FAILED(result))
        {
            std::cerr << "Failed to create rasterizer state\n";
            exit(-1);
        }
        deviceContext->RSSetState(defaultRasterizerState);
    }


    // initialize screen aligned quad
    {
        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));

        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = static_cast<UINT>(sizeof(VertexData) * VertexTube.size());
        
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // create the buffer
        result = device->CreateBuffer(&bd, NULL, &vertexTubeMesh.vertexBuffer);
        if (FAILED(result))
        {
            std::cerr << "Failed to create screen aligned quad vertex buffer\n";
            exit(-1);
        }

        // copy vertex data to buffer
        D3D11_MAPPED_SUBRESOURCE ms;
        deviceContext->Map(vertexTubeMesh.vertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
        memcpy(ms.pData, VertexTube.data(), sizeof(VertexData) * VertexTube.size());
        deviceContext->Unmap(vertexTubeMesh.vertexBuffer, NULL);

        // create input vertex layout
        D3D11_INPUT_ELEMENT_DESC ied[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

        result = device->CreateInputLayout(ied, 3, quadCompositeShader.vsBlob->GetBufferPointer(), quadCompositeShader.vsBlob->GetBufferSize(), &vertexTubeMesh.vertexLayout);
        if (FAILED(result))
        {
            std::cerr << "Failed to create screen aligned quad input layout\n";
            exit(-1);
        }

        vertexTubeMesh.vertexCount = static_cast<UINT>(VertexTube.size());
        vertexTubeMesh.stride = static_cast<UINT>(sizeof(VertexData));
        vertexTubeMesh.offset = 0;
        vertexTubeMesh.topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
    }
    /* {
        D3D11_RASTERIZER_DESC rasterDesc;
        ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));
        rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
        rasterDesc.CullMode = D3D11_CULL_BACK;
        rasterDesc.FrontCounterClockwise = FALSE;
        rasterDesc.DepthClipEnable = TRUE;

        ID3D11RasterizerState* pRasterState;
        result = device->CreateRasterizerState(&rasterDesc, &pRasterState);
        if (FAILED(result))
        {
            std::cerr << "Failed to create rasterizer state\n";
            exit(-1);
        }
        deviceContext->RSSetState(pRasterState);
    }*/
    // initialize line helper
    {
        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));

        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = static_cast<UINT>(sizeof(XMFLOAT3) * LineTube.size());

        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // create the buffer
        result = device->CreateBuffer(&bd, NULL, &lineTubeMesh.vertexBuffer);
        if (FAILED(result))
        {
            std::cerr << "Failed to create screen aligned quad vertex buffer\n";
            exit(-1);
        }

        // copy vertex data to buffer
        D3D11_MAPPED_SUBRESOURCE ms;
        deviceContext->Map(lineTubeMesh.vertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
        memcpy(ms.pData, LineTube.data(), sizeof(XMFLOAT3) * LineTube.size());
        deviceContext->Unmap(lineTubeMesh.vertexBuffer, NULL);

        // create input vertex layout
        D3D11_INPUT_ELEMENT_DESC ied[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

        result = device->CreateInputLayout(ied, 1, linesShader.vsBlob->GetBufferPointer(), linesShader.vsBlob->GetBufferSize(), &lineTubeMesh.vertexLayout);
        if (FAILED(result))
        {
            std::cerr << "Failed to create screen aligned quad input layout\n";
            exit(-1);
        }

        lineTubeMesh.vertexCount = static_cast<UINT>(LineTube.size());
        lineTubeMesh.stride = static_cast<UINT>(sizeof(XMFLOAT3));
        lineTubeMesh.offset = 0;
        lineTubeMesh.topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
    }

    // initialize transforms
    //{
        //transforms.model = DirectX::XMMatrixIdentity();
        //transforms.view = DirectX::XMMatrixIdentity();
        //transforms.proj = DirectX::XMMatrixIdentity();
    //}

    // create transformation constant buffer
    {
        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(CD3D11_BUFFER_DESC));

        bd.ByteWidth = sizeof(Transformations);
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // create the buffer
        result = device->CreateBuffer(&bd, NULL, &transformConstantBuffer);
        if (FAILED(result))
        {
            std::cerr << "Failed to create transform constant buffer\n";
            exit(-1);
        }
    }

    // compositing constant buffer
    {
        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(CD3D11_BUFFER_DESC));

        bd.ByteWidth = sizeof(matrices_and_user_input);
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // create the buffer
        result = device->CreateBuffer(&bd, NULL, &compositionConstantBuffer);
        if (FAILED(result))
        {
            std::cerr << "Failed to create composition constant buffer\n";
            exit(-1);
        }
    }

    // compute kBufferInfo parameter
    {
        computeInfo.kBufferInfo = DirectX::XMFLOAT4(WIDTH,HEIGHT,mkBufferLayer,0);
    }
    {
        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(CD3D11_BUFFER_DESC));

        bd.ByteWidth = sizeof(ComputeInfo);
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // create the buffer
        result = device->CreateBuffer(&bd, NULL, &computeConstantBuffer);
        if (FAILED(result))
        {
            std::cerr << "Failed to create blur constant buffer\n";
            exit(-1);
        }
    }

    // set the viewport (note: since this does not change, it is sufficient to do this once)
    /*
    D3D11_VIEWPORT viewport = { };
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(WIDTH);
    viewport.Height = static_cast<float>(HEIGHT);
    viewport.MinDepth = 0.f;
    viewport.MaxDepth = 1.f;

    deviceContext->RSSetViewports(1, &viewport);*/

    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(WIDTH), static_cast<float>(HEIGHT), 0.0f, 1.0f };
    deviceContext->RSSetViewports(1, &viewport);

    D3D11_RECT scissorRect = { 0, 0, WIDTH, HEIGHT };
    deviceContext->RSSetScissorRects(1, &scissorRect);
}

void CleanUpRenderData()
{
    // states
    defaultRasterizerState->Release();
    defaultSamplerState->Release();

    // constant buffers
    transformConstantBuffer->Release();
    lightSourceConstantBuffer->Release();
    materialConstantBuffer->Release();

    compositionConstantBuffer->Release();
    computeConstantBuffer->Release();


    // meshes

    vertexTubeMesh.vertexBuffer->Release();
    vertexTubeMesh.vertexLayout->Release();

    // depth-stencil states
    depthStencilStateWithDepthTest->Release();
    depthStencilStateWithoutDepthTest->Release();

    // depth-stencil target
    depthStencilTarget.dsView->Release();
    depthStencilTarget.dsTexture->Release();

    // render targets
    for (UINT32 i = 0; i < NUM_RENDERTARGETS; ++i)
    {
        RenderTarget& renderTarget = renderTargets[i];

        //renderTarget.unorderedAccessView->Release();
        //renderTarget.shaderResourceView->Release();
        //renderTarget.renderTargetView->Release();
        //renderTarget.renderTargetTexture->Release();
    }
}

void InitD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    scd.BufferCount = 1;                                        // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;    // use SRGB for gamma-corrected output
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;          // how swap chain is to be used
    scd.OutputWindow = hWnd;                                    // the window to be used
    scd.SampleDesc.Count = 1;                                   // how many multisamples
    scd.Windowed = TRUE;                                        // windowed/full-screen mode

    //D3D_FEATURE_LEVEL* featureLevel;
    UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;

    // create a device, device context and swap chain
    D3D11CreateDeviceAndSwapChain(NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        createDeviceFlags,
        NULL,
        NULL,
        D3D11_SDK_VERSION,
        &scd,
        &swapchain,
        &device,
        NULL,
        &deviceContext);

    // get the address of the back buffer
    ID3D11Texture2D *pBackBuffer = nullptr;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    if (pBackBuffer != nullptr)
    {
        // use the back buffer address to create the render target
        device->CreateRenderTargetView(pBackBuffer, nullptr, &backbuffer);
        pBackBuffer->Release();
    }
    else
    {
        std::cerr << "Could not obtain backbuffer from swapchain";
        exit(-1);
    }
    
    ID3DBlob* errorBlob = nullptr;
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    
    // textured screen-aligned quad shader
    {
        auto hr = D3DCompileFromFile(L"shaders/quadcomposite.hlsl", 0, 0, "VSMain", "vs_4_0", compileFlags, 0, &quadCompositeShader.vsBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }

            exit(-1);
        }

        hr = D3DCompileFromFile(L"shaders/quadcomposite.hlsl", 0, 0, "GSMain", "gs_4_0", compileFlags, 0, &quadCompositeShader.gsBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }

            exit(-1);
        }

        hr = D3DCompileFromFile(L"shaders/quadcomposite.hlsl", 0, 0, "PSMain", "ps_4_0", compileFlags, 0, &quadCompositeShader.psBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }

            exit(-1);
        }

        // encapsulate both shaders into shader objects
        device->CreateVertexShader(quadCompositeShader.vsBlob->GetBufferPointer(), quadCompositeShader.vsBlob->GetBufferSize(), NULL, &quadCompositeShader.vShader);
        device->CreateGeometryShader(quadCompositeShader.gsBlob->GetBufferPointer(), quadCompositeShader.gsBlob->GetBufferSize(), NULL, &quadCompositeShader.gShader);
        device->CreatePixelShader(quadCompositeShader.psBlob->GetBufferPointer(), quadCompositeShader.psBlob->GetBufferSize(), NULL, &quadCompositeShader.pShader);
    }

    {
        auto hr = D3DCompileFromFile(L"shaders/computeShader.hlsl", 0, 0, "main", "cs_5_0", compileFlags, 0, &computeShader.csBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }

            exit(-1);
        }
        device->CreateComputeShader(computeShader.csBlob->GetBufferPointer(), computeShader.csBlob->GetBufferSize(), NULL, &computeShader.cShader);
    }
    
    // helper lines
    {
        auto hr = D3DCompileFromFile(L"shaders/2dlines.hlsl", 0, 0, "VSMain", "vs_4_0", compileFlags, 0, &linesShader.vsBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }

            exit(-1);
        }

        hr = D3DCompileFromFile(L"shaders/2dlines.hlsl", 0, 0, "PSMain", "ps_4_0", compileFlags, 0, &linesShader.psBlob, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }

            exit(-1);
        }

        // encapsulate both shaders into shader objects
        device->CreateVertexShader(linesShader.vsBlob->GetBufferPointer(), linesShader.vsBlob->GetBufferSize(), NULL, &linesShader.vShader);
        device->CreatePixelShader(linesShader.psBlob->GetBufferPointer(), linesShader.psBlob->GetBufferSize(), NULL, &linesShader.pShader);
    }

}

void ShutdownD3D()
{


    computeShader.csBlob->Release();
    computeShader.cShader->Release();

    quadCompositeShader.vShader->Release();
    quadCompositeShader.gShader->Release();
    quadCompositeShader.pShader->Release();
    quadCompositeShader.vsBlob->Release();
    quadCompositeShader.gsBlob->Release();
    quadCompositeShader.psBlob->Release();

    swapchain->Release();
    backbuffer->Release();
    device->Release();
    deviceContext->Release();
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // check for window closing
    switch (message)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    break;
    }

    // handle messages that the switch statement did not handle
    return DefWindowProc(hWnd, message, wParam, lParam);
}
