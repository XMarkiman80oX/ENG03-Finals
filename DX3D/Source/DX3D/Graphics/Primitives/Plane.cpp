#include <DX3D/Graphics/Primitives/Plane.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <vector>

using namespace dx3d;

// Plane implementation
Plane::Plane() : AGameObject()
{
}

Plane::Plane(const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : AGameObject(position, rotation, scale)
{
}

void Plane::update(float deltaTime)
{
    // Override this method to add plane-specific update logic if needed
    // Call base class update if needed
    AGameObject::update(deltaTime);
}

std::shared_ptr<VertexBuffer> Plane::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc)
{
    // Create a default 1x1 plane
    return CreateVertexBuffer(resourceDesc, 1.0f, 1.0f);
}

std::shared_ptr<VertexBuffer> Plane::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc, float width, float height)
{
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    // Define plane vertices (quad in XY plane, facing positive Z direction)
    std::vector<Vertex> vertices = {
        // Bottom-left (red)
        { {-halfWidth, -halfHeight, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} },
        // Bottom-right (green)
        { { halfWidth, -halfHeight, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
        // Top-right (blue)
        { { halfWidth,  halfHeight, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },
        // Top-left (yellow)
        { {-halfWidth,  halfHeight, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} }
    };

    return std::make_shared<VertexBuffer>(
        vertices.data(),
        sizeof(Vertex),
        static_cast<ui32>(vertices.size()),
        resourceDesc
    );
}

std::shared_ptr<VertexBuffer> Plane::CreateWhiteVertexBuffer(const GraphicsResourceDesc& resourceDesc, float width, float height)
{
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    // Create a LARGE, BRIGHT colored plane that's impossible to miss
    std::vector<Vertex> vertices = {
        // Make it in XY plane first (vertical) to test visibility
        // Front face vertices with BRIGHT colors
        { {-halfWidth, -halfHeight, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} }, // Bottom-left RED
        { { halfWidth, -halfHeight, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} }, // Bottom-right GREEN  
        { { halfWidth,  halfHeight, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} }, // Top-right BLUE
        { {-halfWidth,  halfHeight, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} }  // Top-left YELLOW
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
    // Define plane indices (2 triangles making a quad)
    // Triangle 1: bottom-left, bottom-right, top-right
    // Triangle 2: bottom-left, top-right, top-left
    std::vector<ui32> indices = {
        0, 1, 2,    // First triangle
        0, 2, 3     // Second triangle
    };

    return std::make_shared<IndexBuffer>(
        indices.data(),
        static_cast<ui32>(indices.size()),
        resourceDesc
    );
}