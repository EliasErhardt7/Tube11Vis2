#pragma once

#include <array>
#include <vector>
#include <DirectXMath.h>

struct VertexPosNormal
{
    float x, y, z;      // position
    float nx, ny, nz;   // normal
};


constexpr std::array<DirectX::XMFLOAT3, 6> LineTube = {
    DirectX::XMFLOAT3(-1.f,  1.0f, 0.f),
    DirectX::XMFLOAT3(1.f, -1.f, 0.f),
    DirectX::XMFLOAT3(-1.f, -1.f, 0.f),
    DirectX::XMFLOAT3(-1.f,  1.f, 0.f),
    DirectX::XMFLOAT3(1.f,  1.f, 0.f),
    DirectX::XMFLOAT3(1.f, -1.f, 0.f)
};

struct VertexData
{
    DirectX::XMFLOAT3 position;
    float data;
    float curvature;
};

struct Poly {

    /// <summary>
    /// The vertices that make up this Polyline
    /// </summary>
    std::vector<VertexData> vertices;

};

struct draw_call_t
{
    /// <summary>
    /// The index of the first vertex inside the buffer array
    /// </summary>
    uint32_t firstIndex;
    /// <summary>
    /// The number of vertices that belong to this polyline
    /// </summary>
    uint32_t numberOfPrimitives;
};

struct VertexPosTexCoord
{
    float x, y, z;      // position
    float u, v;         // tex coord
};

constexpr std::array<VertexPosTexCoord, 6> ScreenAlignedQuad = {
    VertexPosTexCoord{ -1.f,  1.f, 0.f, 0.f, 0.f },
    VertexPosTexCoord{  1.f, -1.f, 0.f, 1.f, 1.f },
    VertexPosTexCoord{ -1.f, -1.f, 0.f, 0.f, 1.f },
    VertexPosTexCoord{ -1.f,  1.f, 0.f, 0.f, 0.f },
    VertexPosTexCoord{  1.f,  1.f, 0.f, 1.f, 0.f },
    VertexPosTexCoord{  1.f, -1.f, 0.f, 1.f, 1.f }
};