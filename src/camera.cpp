#include "camera.h"

Camera::Camera() : position(XMVectorSet(0.0f, 0.4f, 0.75f, 1.0f)),
    forward(XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)),
    up(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)), yaw(0.0f), pitch(0.0f), mouseSensitivity(0.1f) {}

void Camera::MoveForward(float distance) { position += forward * distance; }
void Camera::MoveRight(float distance) { position += XMVector3Cross(forward, up) * distance; }
void Camera::MoveUp(float distance) { position += up * distance; }
void Camera::RotateCamera(float dx, float dy) {
	constexpr float limit = XM_PIDIV2 - 0.01f;
	pitch = std::max(-limit, pitch);
	pitch = std::min(+limit, pitch);

	// keep longitude in sane range by wrapping
	if (yaw > XM_PI)
	{
		yaw -= XM_2PI;
	}
	else if (yaw < -XM_PI)
	{
		yaw += XM_2PI;
	}

	float y = sinf(pitch);
	float r = cosf(pitch);
	float z = r * cosf(yaw);
	float x = r * sinf(yaw);

    // Update the forward vector
    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(pitch), XMConvertToRadians(yaw), 0);
    forward = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotationMatrix);
    forward = XMVector3Normalize(forward);

    // Recalculate the right and up vectors
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(forward, XMVectorSet(0, 1, 0, 0)));
    up = XMVector3Cross(right, forward);
}
void Camera::InitializeMouse(HWND window) {
	//m_mouse = std::make_unique<Mouse>();
	//m_mouse->SetWindow(window);
}
XMMATRIX Camera::GetViewMatrix()
{
    return XMMatrixLookToLH(position, forward, up);
}
