#pragma once

#include <array>
#include <vector>
#include <DirectXMath.h>

struct VertexPosNormal
{
    float x, y, z;      // position
    float nx, ny, nz;   // normal
};


struct VertexData
{
    DirectX::XMFLOAT3 position;
    float data;
    float curvature;
};

constexpr std::array<VertexData, 6> VertexTube = {
    VertexData{ DirectX::XMFLOAT3(-0.f,  1.5f, 0.f), 0.5f, 0.f},
    VertexData{ DirectX::XMFLOAT3(1.f, -1.f, 0.f), 1.f, 1.f },
    VertexData{ DirectX::XMFLOAT3(-1.5f, -1.f, 0.f), 5.f, 1.5f },
    VertexData{ DirectX::XMFLOAT3(-1.f,  1.f, 0.5f), 0.f, 7.f },
    VertexData{ DirectX::XMFLOAT3(1.f,  1.5f, 0.f), 4.f, 0.f },
    VertexData{ DirectX::XMFLOAT3(1.5f, -1.f, 0.f), 1.f, 1.f }
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

/**
 * Loads the obj mesh from the given path and stores the vertices in the given vector.
 *
 * Notes:
 * - positions will be normalized to center (0, 0, 0) and extent [-0.5, 0.5] in the dimension with the max extent
 * - normals will be computed from the face vertex positions, not read from the file
 * - colors and texture coordinates will not be read
 */
bool LoadObjFile(const char* inputFile, std::vector<VertexPosNormal>& data);