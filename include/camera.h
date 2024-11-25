#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <Mouse.h>
using namespace DirectX;

class Camera {
public:
	Camera();
	void MoveForward(float distance);
	void MoveRight(float distance);
	void MoveUp(float distance);
	void RotateCamera(float dx, float dy);

	void InitializeMouse(HWND window);
	XMMATRIX GetViewMatrix();
	

private:
	XMVECTOR position;
	XMVECTOR forward;
	XMVECTOR up;
	XMVECTOR right;
	float yaw = 0.0f;
	float pitch = 0.0f;
	const float mouseSensitivity = 0.1f;
	std::unique_ptr<Mouse> m_mouse;

	const float ROTATION_GAIN = 0.004f;
	const float MOVEMENT_GAIN = 0.07f;
};
