#include <DX3D/Graphics/Primitives/Cylinder.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <vector>
#include <cmath>

using namespace dx3d;

Cylinder::Cylinder() : AGameObject()
{
}

Cylinder::Cylinder(const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : AGameObject(position, rotation, scale)
{
}

void Cylinder::update(float deltaTime)
{
    AGameObject::update(deltaTime);
}

std::shared_ptr<VertexBuffer> Cylinder::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc, ui32 segments)
{
    std::vector<Vertex> vertices;
    const float radius = 0.5f;
    const float height = 1.0f;
    const float halfHeight = height * 0.5f;

    // Top center vertex
    vertices.push_back({ {0.0f, halfHeight, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f} });

    // Bottom center vertex
    vertices.push_back({ {0.0f, -halfHeight, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f} });

    // Top ring vertices (for caps and sides)
    for (ui32 i = 0; i <= segments; ++i)
    {
        float angle = i * 2.0f * 3.14159265f / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        float u = static_cast<float>(i) / segments;

        // Top cap vertex
        vertices.push_back({ {x, halfHeight, z}, {0.2f, 0.5f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {u, 0.0f} });
    }

    // Bottom ring vertices (for caps and sides)
    for (ui32 i = 0; i <= segments; ++i)
    {
        float angle = i * 2.0f * 3.14159265f / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        float u = static_cast<float>(i) / segments;

        // Bottom cap vertex
        vertices.push_back({ {x, -halfHeight, z}, {1.0f, 0.5f, 0.2f, 1.0f}, {0.0f, -1.0f, 0.0f}, {u, 1.0f} });
    }

    // Side vertices (separate from cap vertices for proper normals)
    for (ui32 i = 0; i <= segments; ++i)
    {
        float angle = i * 2.0f * 3.14159265f / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        float u = static_cast<float>(i) / segments;

        Vector3 sideNormal = Vector3::Normalize(Vector3(x, 0.0f, z));

        // Top side vertex
        vertices.push_back({ {x, halfHeight, z}, {1.0f, 0.5f, 0.5f, 1.0f}, sideNormal, {u, 0.0f} });

        // Bottom side vertex
        vertices.push_back({ {x, -halfHeight, z}, {0.5f, 1.0f, 0.5f, 1.0f}, sideNormal, {u, 1.0f} });
    }

    return std::make_shared<VertexBuffer>(
        vertices.data(),
        sizeof(Vertex),
        static_cast<ui32>(vertices.size()),
        resourceDesc
    );
}

std::shared_ptr<IndexBuffer> Cylinder::CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc, ui32 segments)
{
    std::vector<ui32> indices;

    // Starting index for the vertices of the bottom cap ring
    const ui32 bottomStart = 2 + segments + 1;
    // Starting index for the vertices of the cylinder's side faces
    const ui32 sideStart = bottomStart + segments + 1;

    // Top cap indices (CCW from outside/above)
    for (ui32 i = 0; i < segments; ++i)
    {
        indices.push_back(0);                   // Top center
        indices.push_back(2 + i + 1);           // Next top vertex
        indices.push_back(2 + i);               // Current top vertex
    }

    // Bottom cap indices (CCW from outside/below)
    for (ui32 i = 0; i < segments; ++i)
    {
        indices.push_back(1);                   // Bottom center
        indices.push_back(bottomStart + i);     // Current bottom vertex
        indices.push_back(bottomStart + i + 1); // Next bottom vertex
    }

    // Side faces indices
    for (ui32 i = 0; i < segments; ++i)
    {
        ui32 topCurrent = sideStart + i * 2;
        ui32 bottomCurrent = topCurrent + 1;
        ui32 topNext = sideStart + (i + 1) * 2;
        ui32 bottomNext = topNext + 1;

        // First triangle of the quad
        indices.push_back(topCurrent);
        indices.push_back(topNext);
        indices.push_back(bottomCurrent);

        // Second triangle of the quad
        indices.push_back(topNext);
        indices.push_back(bottomNext);
        indices.push_back(bottomCurrent);
    }

    return std::make_shared<IndexBuffer>(
        indices.data(),
        static_cast<ui32>(indices.size()),
        resourceDesc
    );
}