#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "camera.h"

Camera::Camera() : position(XMVectorSet(0.0f, 0.4f, 0.75f, 1.0f)),
    forward(XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)),
    up(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)) {}

void Camera::MoveForward(float distance) { position += forward * distance; }
void Camera::MoveRight(float distance) { position += XMVector3Cross(forward, up) * distance; }
void Camera::MoveUp(float distance) { position += up * distance; }

XMMATRIX Camera::GetViewMatrix()
{
    return XMMatrixLookToLH(position, forward, up);
}
