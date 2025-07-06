#include <DX3D/Graphics/Primitives/CameraIcon.h>
#include <DX3D/Graphics/Vertex.h> // We will use the standard Vertex struct
#include <vector>

using namespace dx3d;

CameraIcon::CameraIcon(const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : AGameObject(position, rotation, scale)
{
}

std::shared_ptr<VertexBuffer> CameraIcon::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc)
{
    // Use the standard Vertex struct and place texture coordinates into the color field.
    // This removes any possibility of a custom input layout being the problem.
    // Texcoords: U is in color.r, V is in color.g
    const std::vector<Vertex> vertices = {
        // Position                 // Color (used for Tex Coords: u, v, 0, 0)
        { {-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f} }, // 0: Bottom-left
        { {-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f} }, // 1: Top-left
        { { 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f} }, // 2: Top-right
        { { 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.0f} }  // 3: Bottom-right
    };

    return std::make_shared<VertexBuffer>(
        vertices.data(),
        sizeof(Vertex),
        static_cast<ui32>(vertices.size()),
        resourceDesc
    );
}

std::shared_ptr<IndexBuffer> CameraIcon::CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc)
{
    // This is the most common and error-proof indexing for a quad.
    const std::vector<ui32> indices = { 0, 1, 2,  0, 2, 3 };

    return std::make_shared<IndexBuffer>(
        indices.data(),
        static_cast<ui32>(indices.size()),
        resourceDesc
    );
}