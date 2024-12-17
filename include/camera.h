#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <Mouse.h>
//#include <Mouse.h>
#include <d3dcompiler.h>
#include <SimpleMath.h>
using namespace DirectX;

class Camera {
public:
	Camera();
	void MoveForward(float distance);
	void MoveRight(float distance);
	void MoveUp(float distance);
	void RotateCamera(float dx, float dy);
	void RotateY(float angle);
	XMVECTOR GetPosition();
	XMVECTOR GetForward();
	XMMATRIX GetViewMatrix();
	void Init(HWND window);
	void UpdateMouse(float rotationSpeed);

	std::unique_ptr<DirectX::Mouse> m_mouse;
	Mouse::ButtonStateTracker tracker;




private:
	XMVECTOR position;
	XMVECTOR forward;
	XMVECTOR up;
	XMVECTOR right;
	float yaw = 0.0f;
	float pitch = 0.0f;
	const float ROTATION_GAIN = 0.004f;
	const float MOVEMENT_GAIN = 0.07f;
	HWND _window;
};
