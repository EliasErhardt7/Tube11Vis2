#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <array>
#include <iostream>
#include <sstream>
#include <d3dcompiler.h>
#include <format>
//#include <fmt/core.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

#include "Dataset.h"
#include "geometry.h"
#include "resource.h"
#include "util/timer.h"
#include "camera.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imfilebrowser.h"

///////////////////////
// global declarations
#define IMGUI_COLLAPSEINDENTWIDTH 20.0F // The width of the coll
int WIDTH = 1024;
int HEIGHT = 768;
constexpr size_t NUM_RENDERTARGETS = 3;
constexpr size_t NUM_STORAGETARGETS = 1;
bool isRightButtonDown = false;
bool finishInit = false;
// timer for retrieving delta time between frames
Timer timer;

Camera camera;

// DirectX resources: swapchain, device, and device context
IDXGISwapChain* swapchain;
ID3D11Device* device;
ID3D11DeviceContext* deviceContext;

// backbuffer obtained from the swapchain
ID3D11RenderTargetView* backbuffer;

// shaders
ShaderProgram testShader;
ComputeShader testComputeShader;
ShaderProgram modelShader;
ShaderProgram quadCompositeShader;
ShaderProgram linesShader;
ShaderProgram resolveShader;
ShaderProgram backgroundShader;
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
ID3D11Buffer* mNewVertexBuffer;
ID3D11Buffer* mVertexBuffer;
// material and light source
Material material;
ID3D11Buffer* materialConstantBuffer;

LightSource lightSource;
ID3D11Buffer* lightSourceConstantBuffer;

// parameters for compute shaders
ID3D11Buffer* thresholdConstantBuffer;

ComputeInfo computeInfo;
ID3D11Buffer* computeConstantBuffer;

ID3D11BlendState* blendState;

ID3D11Buffer* compositionConstantBuffer;

std::unique_ptr<Dataset> mDataset;
std::vector<draw_call_t> mDrawCalls;
ID3D11Buffer* mIndexBuffer;

bool mBillboardClippingEnabled = true;
bool mBillboardShadingEnabled = true;
bool mReplaceOldBufferWithNextFrame = false;

float mDirLightDirection[3] = { -0.7F, -0.6F, -0.3F };
float mDirLightColor[4] = { 1.0F, 1.0F, 1.0F, 1.0F };
float mDirLightIntensity = 1.0F;
float mAmbLightColor[4] = { 0.05F, 0.05F, 0.05F, 1.0F };
float mMaterialReflectivity[4] = { 0.5, 1.0, 0.5, 32.0 };

int mVertexColorMode = 0;
float mVertexColorStatic[3] = { 65.0F / 255.0F, 105.0F / 255.0F, 225.0F / 255.0F };
float mVertexColorMin[3] = { 1.0F / 255.0F, 169.0F / 255.0F, 51.0F / 255.0F };
float mVertexColorMax[3] = { 1.0F, 0.0F, 0.0F };

int mVertexAlphaMode = 0;
bool mVertexAlphaInvert = false;
float mVertexAlphaStatic = 0.5;
float mVertexAlphaBounds[2] = { 0.5, 0.8 };

int mVertexRadiusMode = 0;
bool mVertexRadiusInvert = false;
float mVertexRadiusStatic = 0.001;
float mVertexRadiusBounds[2] = { 0.001, 0.002 };
float mClearColor[4] = { 1.0F, 1.0F, 1.0F, 0.2F };
bool mShowStatisticsWindow = true;
bool mFullscreenModeEnabled = false;

bool mDraw2DHelperLines = false;

float mHelperLineColor[4] = { 64.0f / 255.0f, 224.0f / 255.0f, 208.0f / 255.0f, 1.0f };

bool mMainRenderPassEnabled = true;

bool mResolveKBuffer = true;
bool vertexBufferSet = false;

long mRenderCallCount = 0;
int mkBufferLayer = 8;
int mkBufferLayerCount = 16;

bool mShowUI = true;
ImGui::FileBrowser mOpenFileDialog;
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
void loadDatasetFromFile(std::string& filename);
void renderUI();
// update tick for render data (e.g., to update transformation matrices)
void UpdateTick(float deltaTime);
void OnSwapChainResized(HWND hWnd);
// rendering
void RenderFrame();
//void activateImGuiStyle(bool darkMode, float alpha);
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
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

	camera.Init(hWnd);

	mDataset = std::make_unique<Dataset>();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	//activateImGuiStyle(false, 0.8);
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	InitD3D(hWnd);

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device, deviceContext);

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
			//renderUI();
			RenderFrame();
		}
	}

	// shutdown
	CleanUpRenderData();
	ShutdownD3D();

	return 0;
}
void loadDatasetFromFile(std::string& filename) {

	mDataset->importFromFile(filename);

	std::vector<VertexData> gpuVertexData;

	std::vector<uint32_t> dataIndexBuffer;
	//mDataset->fillGPUReadyBuffer(gpuVertexData, mDrawCalls);
	mDataset->fillGPUReadyBuffer(gpuVertexData, dataIndexBuffer);

	// Create new GPU Buffer
	D3D11_BUFFER_DESC id;
	ZeroMemory(&id, sizeof(D3D11_BUFFER_DESC));

	id.Usage = D3D11_USAGE_DYNAMIC;
	id.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * dataIndexBuffer.size());
	id.BindFlags = D3D11_BIND_INDEX_BUFFER;
	id.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	// create the buffer
	HRESULT result = device->CreateBuffer(&id, NULL, &mIndexBuffer);
	if (FAILED(result))
	{
		std::cerr << "Failed to create screen aligned quad vertex buffer\n";
		exit(-1);
	}
	// copy vertex data to buffer
	D3D11_MAPPED_SUBRESOURCE ms2;
	deviceContext->Map(mIndexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms2);
	memcpy(ms2.pData, dataIndexBuffer.data(), sizeof(uint32_t) * dataIndexBuffer.size());
	deviceContext->Unmap(mIndexBuffer, NULL);


	// Create new GPU Buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = static_cast<UINT>(sizeof(VertexData) * gpuVertexData.size());

	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	// create the buffer
	result = device->CreateBuffer(&bd, NULL, &mNewVertexBuffer);
	if (FAILED(result))
	{
		std::cerr << "Failed to create screen aligned quad vertex buffer\n";
		exit(-1);
	}

	// copy vertex data to buffer
	D3D11_MAPPED_SUBRESOURCE ms;
	deviceContext->Map(mNewVertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, gpuVertexData.data(), sizeof(VertexData) * gpuVertexData.size());
	deviceContext->Unmap(mNewVertexBuffer, NULL);

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

	vertexTubeMesh.vertexCount = gpuVertexData.size();
	vertexTubeMesh.indexCount = dataIndexBuffer.size();
	vertexTubeMesh.stride = static_cast<UINT>(sizeof(VertexData));
	vertexTubeMesh.offset = 0;
	vertexTubeMesh.topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;

	mReplaceOldBufferWithNextFrame = true;
	vertexBufferSet = true;
}

void toggleInputMode() {

}

/// <summary>
/// Switches between unobstracted fullscreen mode and windowed mode
/// </summary>
void toggleFullscreenMode() {
	if (mFullscreenModeEnabled)
		//gvk::context().main_window()->switch_to_windowed_mode();
	//else
		//gvk::context().main_window()->switch_to_fullscreen_mode();
		mFullscreenModeEnabled = !mFullscreenModeEnabled;
}

void UpdateTick(float deltaTime)
{
	// Camera movement speed
	const float cameraSpeed = 1.0f * deltaTime;
	const float rotationSpeed = 1.0f * deltaTime;

	
	camera.UpdateMouse(rotationSpeed);

	// Forward/Backward movement
	if (GetAsyncKeyState('W') & 0x8000) camera.MoveForward(cameraSpeed);
	if (GetAsyncKeyState('S') & 0x8000) camera.MoveForward(-cameraSpeed);

	// Left/Right movement
	if (GetAsyncKeyState('A') & 0x8000) camera.MoveRight(-cameraSpeed);
	if (GetAsyncKeyState('D') & 0x8000) camera.MoveRight(cameraSpeed);
	// Up/Down movement
	//if (GetAsyncKeyState(VK_SPACE) & 0x8000) camera.MoveUp(cameraSpeed);
	//if (GetAsyncKeyState(VK_SHIFT) & 0x8000) camera.MoveUp(-cameraSpeed);

	// Rotation
	if (GetAsyncKeyState('E') & 0x8000) camera.MoveUp(rotationSpeed);
	if (GetAsyncKeyState('Q') & 0x8000) camera.MoveUp(-rotationSpeed);

	// Update view transform
	transforms.view = DirectX::XMMatrixTranspose(camera.GetViewMatrix());


	// Update projection transform (unchanged)
	transforms.proj = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(60.0f), static_cast<float>(-WIDTH) / static_cast<float>(HEIGHT), 0.001f, 100.f));

	// Update model transform (rotation)
	constexpr float millisecondsToAngle = 0.0001f * 6.28f;
	transforms.model = DirectX::XMMatrixMultiply(transforms.model, DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationY(deltaTime * millisecondsToAngle)));

	// Update light source (as before)
	DirectX::XMVECTOR lightWorldPos = DirectX::XMVectorSet(-1.5f, 1.5f, 1.5f, 1.f);
	DirectX::XMVECTOR lightViewPos = DirectX::XMVector4Transform(lightWorldPos, transforms.view);
	DirectX::XMStoreFloat4(&lightSource.lightPosition, lightViewPos);
	lightSource.lightColorAndPower = DirectX::XMFLOAT4(1.f, 1.f, 0.7f, 4.5f);
}

void renderUI() {
	if (!mShowUI) return;

	//auto resolution = gvk::context().main_window()->resolution();
	ImGui::Begin("Line Renderer - Tool Box", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	ImGui::SetWindowSize(ImVec2(0.0F, HEIGHT + 2.0), ImGuiCond_Always);
	ImGui::SetWindowPos(ImVec2(-1.0F, -1.0F), ImGuiCond_Once);

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Load Data-File")) {
				mOpenFileDialog.Open();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	if (ImGui::CollapsingHeader("Background", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("BG");
		ImGui::Indent(IMGUI_COLLAPSEINDENTWIDTH);
		ImGui::ColorPicker3("Color", mClearColor, ImGuiColorEditFlags_PickerHueWheel);
		ImGui::SliderFloat("Gradient", &mClearColor[3], 0.0F, 1.0F);
		ImGui::Indent(-IMGUI_COLLAPSEINDENTWIDTH);
		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader("Debug Properties")) {
		ImGui::Indent(IMGUI_COLLAPSEINDENTWIDTH);
		ImGui::Checkbox("Main Render Pass Enabled", &mMainRenderPassEnabled);
		if (mMainRenderPassEnabled) {
			ImGui::Checkbox("Billboard-Clipping", &mBillboardClippingEnabled);
			ImGui::Checkbox("Billboard-Shading", &mBillboardShadingEnabled);
		}
		ImGui::Separator();
		ImGui::Checkbox("Resolve K-Buffer", &mResolveKBuffer);
		ImGui::SliderInt("Layer", &mkBufferLayer, 1, mkBufferLayerCount);
		ImGui::Indent(-IMGUI_COLLAPSEINDENTWIDTH);
	}
	if (ImGui::CollapsingHeader("Vertex Color", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("VC");
		ImGui::Indent(IMGUI_COLLAPSEINDENTWIDTH);
		const char* vertexColorModes[] = { "Static", "Data dependent", "Length dependent", "Curvature dependent" };
		ImGui::Combo("Mode", &mVertexColorMode, vertexColorModes, IM_ARRAYSIZE(vertexColorModes));
		if (mVertexColorMode == 0) {
			ImGui::ColorEdit3("Color", mVertexColorStatic);
		}
		else if (mVertexColorMode < 4) {
			ImGui::ColorEdit3("Min-Color", mVertexColorMin);
			ImGui::ColorEdit3("Max-Color", mVertexColorMax);
			if (ImGui::Button("Invert colors")) {
				float colorBuffer[3];
				std::copy(mVertexColorMin, mVertexColorMin + 3, colorBuffer);
				std::copy(mVertexColorMax, mVertexColorMax + 3, mVertexColorMin);
				std::copy(colorBuffer, colorBuffer + 3, mVertexColorMax);
			}
		}
		ImGui::Indent(-IMGUI_COLLAPSEINDENTWIDTH);
		ImGui::PopID();
	}
	if (ImGui::CollapsingHeader("Vertex Transparency", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("VT");
		ImGui::Indent(IMGUI_COLLAPSEINDENTWIDTH);
		const char* vertexAlphaModes[] = { "Static", "Data dependent", "Length dependent", "Curvature dependent" };
		ImGui::Combo("Mode", &mVertexAlphaMode, vertexAlphaModes, IM_ARRAYSIZE(vertexAlphaModes));
		if (mVertexAlphaMode == 0) ImGui::SliderFloat("Alpha", &mVertexAlphaStatic, 0.0F, 1.0F);
		else {
			ImGui::SliderFloat2("Bounds", mVertexAlphaBounds, 0.0F, 1.0F);
			ImGui::Checkbox("Invert", &mVertexAlphaInvert);
		}
		ImGui::Indent(-IMGUI_COLLAPSEINDENTWIDTH);
		ImGui::PopID();
	}
	if (ImGui::CollapsingHeader("Vertex Radius", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("VR");
		ImGui::Indent(IMGUI_COLLAPSEINDENTWIDTH);
		const char* vertexRadiusModes[] = { "Static", "Data dependent", "Length dependent", "Curvature dependent" };
		ImGui::Combo("Mode", &mVertexRadiusMode, vertexRadiusModes, IM_ARRAYSIZE(vertexRadiusModes));
		if (mVertexRadiusMode == 0) ImGui::SliderFloat("Alpha", &mVertexRadiusStatic, 0.0F, 1.0F);
		else {
			ImGui::SliderFloat2("Bounds", mVertexRadiusBounds, 0.0F, 0.02F, "%.4f");
			ImGui::Checkbox("Invert", &mVertexRadiusInvert);
		}
		ImGui::Indent(-IMGUI_COLLAPSEINDENTWIDTH);
		ImGui::PopID();
	}
	if (ImGui::CollapsingHeader("Lighting & Material")) {
		ImGui::Indent(IMGUI_COLLAPSEINDENTWIDTH);
		if (ImGui::CollapsingHeader("Ambient Light", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(IMGUI_COLLAPSEINDENTWIDTH);
			ImGui::ColorEdit4("Color", mAmbLightColor);
			ImGui::Indent(-IMGUI_COLLAPSEINDENTWIDTH);
		}
		if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(IMGUI_COLLAPSEINDENTWIDTH);
			ImGui::SliderFloat("Intensity", &mDirLightIntensity, 0.0F, 10.0F);
			ImGui::ColorEdit4("Color", mDirLightColor);
			ImGui::SliderFloat3("Direction", mDirLightDirection, -1.0F, 1.0F);
			ImGui::Indent(-IMGUI_COLLAPSEINDENTWIDTH);
		}
		if (ImGui::CollapsingHeader("Material Constants", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(IMGUI_COLLAPSEINDENTWIDTH);
			ImGui::SliderFloat3("Amb/Dif/Spec", mMaterialReflectivity, 0.0F, 3.0F);
			ImGui::SliderFloat("Shininess", &mMaterialReflectivity[3], 0.0F, 128.0F);
			ImGui::Indent(-IMGUI_COLLAPSEINDENTWIDTH);
		}
		ImGui::Indent(-IMGUI_COLLAPSEINDENTWIDTH);
	}
	ImGui::End();

	if (mShowStatisticsWindow) {
		ImGui::Begin("Statistics", &mShowStatisticsWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		ImGui::SetWindowPos(ImVec2(WIDTH - ImGui::GetWindowWidth() + 1.0F, -1.0F), ImGuiCond_Always);
		std::string fps = std::to_string(ImGui::GetIO().Framerate);
		static std::vector<float> values;
		values.push_back(ImGui::GetIO().Framerate);
		if (values.size() > 90) values.erase(values.begin());
		ImGui::PlotLines(fps.c_str(), values.data(), static_cast<int>(values.size()), 0, nullptr, 0.0f, FLT_MAX, ImVec2(0.0f, 60.0f));

		if (mDataset->isFileOpen()) {
			if (ImGui::CollapsingHeader("Dataset Information", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Indent(IMGUI_COLLAPSEINDENTWIDTH);
				ImGui::Text("File:             %s", mDataset->getName().c_str());
				ImGui::Text("Line-Count:       %d", mDataset->getLineCount());
				ImGui::Text("Polyline-Count:   %d", mDataset->getPolylineCount());
				ImGui::Text("Vertex-Count:     %d", mDataset->getVertexCount());
				ImGui::Separator();
				auto dim = mDataset->getDimensions();
				auto vel = mDataset->getDataBounds();
				ImGui::Text("Dimensions:       %.1f x %.1f x %.1f", dim.x, dim.y, dim.z);
				ImGui::Text("Data-Bounds:  %.5f - %.5f", vel.x, vel.y);
				ImGui::Separator();
				ImGui::Text("Loading-Time:     %.3f s", mDataset->getLastLoadingTime());
				ImGui::Text("Preprocess-Time:  %.3f s", mDataset->getLastPreprocessTime());
				ImGui::Indent(-IMGUI_COLLAPSEINDENTWIDTH);
			}
		}
		ImGui::End();
	}

	mOpenFileDialog.Display();
	if (mOpenFileDialog.HasSelected()) {
		std::string filename = mOpenFileDialog.GetSelected().string();
		mOpenFileDialog.ClearSelected();
		loadDatasetFromFile(filename);
	}
}

void RenderFrame()
{
	constexpr ID3D11RenderTargetView* NULL_RT = nullptr;
	constexpr ID3D11ShaderResourceView* NULL_SRV = nullptr;
	constexpr ID3D11UnorderedAccessView* NULL_UAV = nullptr;
	constexpr UINT NO_OFFSET = -1;

	//float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };  // Black background
	deviceContext->ClearRenderTargetView(backbuffer, mClearColor);
	deviceContext->ClearDepthStencilView(depthStencilTarget.dsView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	computeInfo.kBufferInfo = XMVectorSet(WIDTH, HEIGHT, mkBufferLayer, 0);
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

	matrices_and_user_input uni;
	uni.mViewMatrix = transforms.view;
	uni.mProjMatrix = transforms.proj;
	uni.mCamPos = camera.GetPosition();
	uni.mCamDir = camera.GetForward();
	uni.mClearColor = XMFLOAT4(mClearColor[0], mClearColor[1], mClearColor[2], mClearColor[3]);
	uni.mHelperLineColor = XMFLOAT4(mHelperLineColor[0], mHelperLineColor[1], mHelperLineColor[2], mHelperLineColor[3]);;
	uni.mkBufferInfo = XMVectorSet(WIDTH, HEIGHT, mkBufferLayer, 0);
	uni.mDirLightDirection = XMFLOAT4(mDirLightDirection[0], mDirLightDirection[1], mDirLightDirection[2], 0);
	uni.mDirLightColor = XMFLOAT4(mDirLightColor[0] * mDirLightIntensity, mDirLightColor[1] * mDirLightIntensity, mDirLightColor[2] * mDirLightIntensity, mDirLightColor[3] * mDirLightIntensity);
	uni.mAmbLightColor = XMFLOAT4(mAmbLightColor[0], mAmbLightColor[1], mAmbLightColor[2], mAmbLightColor[3]);
	uni.mMaterialLightReponse = XMFLOAT4(mMaterialReflectivity[0], mMaterialReflectivity[1], mMaterialReflectivity[2], mMaterialReflectivity[3]);;
	uni.mBillboardClippingEnabled = mBillboardClippingEnabled;
	uni.mBillboardShadingEnabled = mBillboardShadingEnabled;
	uni.mVertexColorMode = mVertexColorMode;
	if (mVertexColorMode == 0)
		uni.mVertexColorMin = XMFLOAT4(mVertexColorStatic[0], mVertexColorStatic[1], mVertexColorStatic[2], 0.0);
	else {
		uni.mVertexColorMin = XMFLOAT4(mVertexColorMin[0], mVertexColorMin[1], mVertexColorMin[2], 0.0);
		uni.mVertexColorMax = XMFLOAT4(mVertexColorMax[0], mVertexColorMax[1], mVertexColorMax[2], 0.0);
	}
	uni.mVertexAlphaMode = mVertexAlphaMode;
	if (mVertexAlphaMode == 0)
		uni.mVertexAlphaBounds = XMFLOAT4(mVertexAlphaStatic, mVertexAlphaBounds[1], 0, 0);//uni.mVertexAlphaBounds[0] = mVertexAlphaStatic;
	else
		uni.mVertexAlphaBounds = XMFLOAT4(mVertexAlphaBounds[0], mVertexAlphaBounds[1], 0, 0);;

	uni.mVertexRadiusMode = mVertexRadiusMode;
	if (mVertexRadiusMode == 0)
		uni.mVertexRadiusBounds = XMFLOAT4(mVertexRadiusStatic, mVertexRadiusBounds[1], 0, 0);//uni.mVertexRadiusBounds[0] = mVertexRadiusStatic;
	else
		uni.mVertexRadiusBounds = XMFLOAT4(mVertexRadiusBounds[0], mVertexRadiusBounds[1], 0, 0);
	uni.mDataMaxLineLength = mDataset->getMaxLineLength();
	uni.mDataMaxVertexAdjacentLineLength = mDataset->getMaxVertexAdjacentLineLength();
	uni.mVertexAlphaInvert = mVertexAlphaInvert;
	uni.mVertexRadiusInvert = mVertexRadiusInvert;
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		deviceContext->Map(compositionConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		memcpy(ms.pData, &uni, sizeof(matrices_and_user_input));
		deviceContext->Unmap(compositionConstantBuffer, 0);
	}

	if (mReplaceOldBufferWithNextFrame) {
		vertexTubeMesh.vertexBuffer = std::move(mNewVertexBuffer);
		mReplaceOldBufferWithNextFrame = false;
	}

	std::array<ID3D11ShaderResourceView*, 1> compositeSRVs = { storageTargets[0].shaderResourceView };
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	// BACKGROUND
	{	
		deviceContext->OMSetRenderTargets(1, &backbuffer, NULL);
		deviceContext->VSSetShader(backgroundShader.vShader, 0, 0);

		deviceContext->PSSetShader(backgroundShader.pShader, 0, 0);
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		deviceContext->PSSetConstantBuffers(0, 1, &compositionConstantBuffer);

		deviceContext->Draw(6, 0);

	}
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	deviceContext->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);
	if (vertexBufferSet) {
		if (mMainRenderPassEnabled) {

			//deviceContext->OMSetRenderTargets(1, &backbuffer, NULL);
			
			UINT initialCounts[] = { 0 }; // Optional: Reset counters for append/consume buffers
			deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(1, &backbuffer, NULL, 1, 1, &csUAVs[0], initialCounts);
			deviceContext->OMSetDepthStencilState(depthStencilStateWithoutDepthTest, 0);
			deviceContext->VSSetShader(quadCompositeShader.vShader, 0, 0);
			deviceContext->GSSetShader(quadCompositeShader.gShader, 0, 0);
			deviceContext->PSSetShader(quadCompositeShader.pShader, 0, 0);

			deviceContext->IASetInputLayout(vertexTubeMesh.vertexLayout);
			deviceContext->IASetVertexBuffers(0, 1, &vertexTubeMesh.vertexBuffer, &vertexTubeMesh.stride, &vertexTubeMesh.offset);
			deviceContext->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			deviceContext->IASetPrimitiveTopology(vertexTubeMesh.topology);


			//deviceContext->PSSetShaderResources(0, 1, &compositeSRVs[0]);

			//deviceContext->PSSetSamplers(0, 1, &defaultSamplerState);

			deviceContext->VSSetConstantBuffers(0, 1, &compositionConstantBuffer);
			deviceContext->GSSetConstantBuffers(0, 1, &compositionConstantBuffer);
			deviceContext->PSSetConstantBuffers(0, 1, &compositionConstantBuffer);

			//for (unsigned int i = 0; i < mDrawCalls.size(); i++) {
			//	deviceContext->Draw(mDrawCalls[i].numberOfPrimitives, mDrawCalls[i].firstIndex);
			//}
			deviceContext->DrawIndexed(vertexTubeMesh.indexCount, 0, 0);
			//deviceContext->Draw(vertexTubeMesh.vertexCount, 0);
			//unbind SRVs
			//deviceContext->PSSetShaderResources(0, 1, &NULL_SRV);
			deviceContext->GSSetShader(NULL, 0, 0);
		}

		// RESOLVE KBUFFER
		if (mResolveKBuffer) {
			deviceContext->OMSetRenderTargets(1, &backbuffer, NULL);
			deviceContext->OMSetDepthStencilState(depthStencilStateWithDepthTest, 0);
			//deviceContext->IASetVertexBuffers(0, 1, nullptr, 0, 0);
			deviceContext->VSSetShader(resolveShader.vShader, 0, 0);

			deviceContext->PSSetShader(resolveShader.pShader, 0, 0);

			deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			deviceContext->PSSetShaderResources(0, 1, &compositeSRVs[0]);

			deviceContext->PSSetConstantBuffers(0, 1, &compositionConstantBuffer);

			deviceContext->Draw(6, 0);

			//unbind SRVs
			deviceContext->PSSetShaderResources(0, 1, &NULL_SRV);
		}
	}
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
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Render your UI
	renderUI();

	// Render the ImGui frame
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
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

		bufferDesc.ByteWidth = WIDTH * HEIGHT * sizeof(UINT64) * mkBufferLayerCount;
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
		srvDesc.Buffer.NumElements = WIDTH * HEIGHT * mkBufferLayerCount;


		result = device->CreateShaderResourceView(storageTargets[0].structuredBuffer, &srvDesc, &storageTargets[0].shaderResourceView);
		if (FAILED(result))
		{
			std::cerr << "Failed to create render target texture SRV\n";
			exit(-1);
		}

		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = WIDTH * HEIGHT * mkBufferLayerCount;

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
		//dsDesc.StencilEnable = false;
		//dsDesc.FrontFace.StencilFailOp = dsDesc.FrontFace.StencilDepthFailOp = dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		//dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

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

	// blend state
	{

		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = TRUE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		
		device->CreateBlendState(&blendDesc, &blendState);
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
		rasterDesc.DepthClipEnable = false;
		rasterDesc.FillMode = D3D11_FILL_SOLID;
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

	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(WIDTH), static_cast<float>(HEIGHT), 0.0f, 1.0f };
	//viewport.MinDepth = 0;
	//viewport.MaxDepth = 1;
	deviceContext->RSSetViewports(1, &viewport);

	D3D11_RECT scissorRect = { 0, 0, WIDTH, HEIGHT };
	deviceContext->RSSetScissorRects(1, &scissorRect);

	mOpenFileDialog.SetTitle("Open Line-Data File");
	mOpenFileDialog.SetTypeFilters({ ".obj" });

	finishInit = true;
}

void CleanUpRenderData()
{
	// states
	defaultRasterizerState->Release();
	//defaultSamplerState->Release();

	// constant buffers
	transformConstantBuffer->Release();
	//lightSourceConstantBuffer->Release();
	//materialConstantBuffer->Release();

	compositionConstantBuffer->Release();
	computeConstantBuffer->Release();


	// meshes
	if (vertexTubeMesh.vertexBuffer != nullptr) {
		//vertexTubeMesh.vertexBuffer->Release();
	}
	if (vertexTubeMesh.vertexLayout != nullptr) {
		//vertexTubeMesh.vertexLayout->Release();
	}


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
	ID3D11Texture2D* pBackBuffer = nullptr;
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
		auto hr = D3DCompileFromFile(L"shaders/quadcomposite.hlsl", 0, 0, "VSMain", "vs_5_0", compileFlags, 0, &quadCompositeShader.vsBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			exit(-1);
		}

		hr = D3DCompileFromFile(L"shaders/quadcomposite.hlsl", 0, 0, "GSMain", "gs_5_0", compileFlags, 0, &quadCompositeShader.gsBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			exit(-1);
		}

		hr = D3DCompileFromFile(L"shaders/quadcomposite.hlsl", 0, 0, "PSMain", "ps_5_0", compileFlags, 0, &quadCompositeShader.psBlob, &errorBlob);
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

	// background Shader

	{
		auto hr = D3DCompileFromFile(L"shaders/background.hlsl", 0, 0, "VSMain", "vs_5_0", compileFlags, 0, &backgroundShader.vsBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			exit(-1);
		}

		hr = D3DCompileFromFile(L"shaders/background.hlsl", 0, 0, "PSMain", "ps_5_0", compileFlags, 0, &backgroundShader.psBlob, &errorBlob);
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
		device->CreateVertexShader(backgroundShader.vsBlob->GetBufferPointer(), backgroundShader.vsBlob->GetBufferSize(), NULL, &backgroundShader.vShader);
		device->CreatePixelShader(backgroundShader.psBlob->GetBufferPointer(), backgroundShader.psBlob->GetBufferSize(), NULL, &backgroundShader.pShader);
	}

	// resolve Shader
	{
		auto hr = D3DCompileFromFile(L"shaders/resolveShader.hlsl", 0, 0, "VSMain", "vs_5_0", compileFlags, 0, &resolveShader.vsBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			exit(-1);
		}

		hr = D3DCompileFromFile(L"shaders/resolveShader.hlsl", 0, 0, "PSMain", "ps_5_0", compileFlags, 0, &resolveShader.psBlob, &errorBlob);
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
		device->CreateVertexShader(resolveShader.vsBlob->GetBufferPointer(), resolveShader.vsBlob->GetBufferSize(), NULL, &resolveShader.vShader);
		device->CreatePixelShader(resolveShader.psBlob->GetBufferPointer(), resolveShader.psBlob->GetBufferSize(), NULL, &resolveShader.pShader);
	}

	// helper lines
	{
		auto hr = D3DCompileFromFile(L"shaders/2dlines.hlsl", 0, 0, "VSMain", "vs_5_0", compileFlags, 0, &linesShader.vsBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			exit(-1);
		}

		hr = D3DCompileFromFile(L"shaders/2dlines.hlsl", 0, 0, "PSMain", "ps_5_0", compileFlags, 0, &linesShader.psBlob, &errorBlob);
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
	resolveShader.vShader->Release();
	resolveShader.pShader->Release();
	resolveShader.vsBlob->Release();
	resolveShader.psBlob->Release();
	backgroundShader.vShader->Release();
	backgroundShader.pShader->Release();
	backgroundShader.vsBlob->Release();
	backgroundShader.psBlob->Release();
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	swapchain->Release();
	backbuffer->Release();
	device->Release();
	deviceContext->Release();
}

void OnSwapChainResized(HWND hWnd)
{
	// Get the new window size
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	WIDTH = clientRect.right - clientRect.left;
	HEIGHT = clientRect.bottom - clientRect.top;

	CleanUpRenderData();
	backbuffer->Release();

	HRESULT hr = swapchain->ResizeBuffers(0, WIDTH, HEIGHT, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr)) {
		// Handle error
	}

	ID3D11Texture2D* pBackBuffer = nullptr;
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

	InitRenderData();
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;
	if (camera.m_mouse) // Ensure m_mouse is accessible here
	{
		camera.m_mouse->ProcessMessage(message, wParam, lParam);
	}
	// Handle mouse messages
	switch (message)
	{
	case WM_SIZE:
	{
		if (wParam != SIZE_MINIMIZED && finishInit)
		{
			OnSwapChainResized(hWnd);
		}
	}
	break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	break;
	}

	// Handle messages that the switch statement did not handle
	return DefWindowProc(hWnd, message, wParam, lParam);
}

