#include <DX3D/Graphics/Primitives/CameraIcon.h>
#include <vector>

using namespace dx3d;

CameraIcon::CameraIcon(const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : AGameObject(position, rotation, scale)
{
}

std::shared_ptr<VertexBuffer> CameraIcon::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc)
{
    // A simple quad with texture coordinates.
    const std::vector<VertexTex> vertices = {
        { {-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f} }, // Bottom-left
        { {-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f} }, // Top-left
        { { 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f} }, // Top-right
        { { 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f} }  // Bottom-right
    };

    return std::make_shared<VertexBuffer>(
        vertices.data(),
        sizeof(VertexTex),
        static_cast<ui32>(vertices.size()),
        resourceDesc
    );
}

std::shared_ptr<IndexBuffer> CameraIcon::CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc)
{
    const std::vector<ui32> indices = {
        0, 1, 2, // First triangle
        0, 2, 3  // Second triangle
    };

    return std::make_shared<IndexBuffer>(
        indices.data(),
        static_cast<ui32>(indices.size()),
        resourceDesc
    );
}