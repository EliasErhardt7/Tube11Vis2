#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

class Camera {
public:
	Camera();
	void MoveForward(float distance);
	void MoveRight(float distance);
	void MoveUp(float distance);
	XMMATRIX GetViewMatrix();

private:
	XMVECTOR position;
	XMVECTOR forward;

	XMVECTOR up;
};
