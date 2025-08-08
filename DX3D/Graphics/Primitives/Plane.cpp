#include <../Graphics/Primitives/Plane.h>
#include <../Graphics/IndexBuffer.h>
#include <vector>

using namespace dx3d;

// Plane implementation
Plane::Plane() : BaseGameObject()
{
}

Plane::Plane(const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : BaseGameObject(position, rotation, scale)
{
}

void Plane::update(float deltaTime)
{
    BaseGameObject::update(deltaTime);
}

std::shared_ptr<VertexBuffer> Plane::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc)
{
    // Create a default 1x1 plane
    return CreateVertexBuffer(resourceDesc, 1.0f, 1.0f);
}

std::shared_ptr<VertexBuffer> Plane::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc, float width, float height)
{
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f; // This now represents depth on the Z-axis

    // Define plane vertices in the XZ plane, with normals pointing up (in the +Y direction)
    std::vector<Vertex> vertices = {
        //                       Position                  Color                  Normal              TexCoords
        { {-halfWidth, 0.0f, -halfHeight}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} }, // Bottom-left
        { { halfWidth, 0.0f, -halfHeight}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} }, // Bottom-right
        { { halfWidth, 0.0f,  halfHeight}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} }, // Top-right
        { {-halfWidth, 0.0f,  halfHeight}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} }  // Top-left
    };

    return std::make_shared<VertexBuffer>(
        vertices.data(),
        sizeof(Vertex),
        static_cast<ui32>(vertices.size()),
        resourceDesc
    );
}

std::shared_ptr<IndexBuffer> Plane::CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc)
{
    std::vector<ui32> indices = {
        0, 2, 1,    // First triangle (Counter-Clockwise)
        0, 3, 2     // Second triangle (Counter-Clockwise)
    };

    return std::make_shared<IndexBuffer>(
        indices.data(),
        static_cast<ui32>(indices.size()),
        resourceDesc
    );
}