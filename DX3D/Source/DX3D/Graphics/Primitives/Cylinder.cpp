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

    // Center vertices for top and bottom caps
    vertices.push_back({ {0.0f, halfHeight, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f} }); // Top center (green)
    vertices.push_back({ {0.0f, -halfHeight, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f} }); // Bottom center (yellow)

    // Generate vertices for top and bottom circles and sides
    for (ui32 i = 0; i <= segments; ++i)
    {
        float angle = i * 2.0f * 3.14159265f / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);

        // Top circle vertex (blue tint)
        vertices.push_back({ {x, halfHeight, z}, {0.2f, 0.5f, 1.0f, 1.0f} });

        // Bottom circle vertex (orange tint)
        vertices.push_back({ {x, -halfHeight, z}, {1.0f, 0.5f, 0.2f, 1.0f} });

        // Side vertices with gradient color
        float t = static_cast<float>(i) / segments;
        vertices.push_back({ {x, halfHeight, z}, {1.0f - t, t, 0.5f, 1.0f} }); // Top side
        vertices.push_back({ {x, -halfHeight, z}, {t, 0.5f, 1.0f - t, 1.0f} }); // Bottom side
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

    // Top cap indices (looking down, clockwise)
    for (ui32 i = 0; i < segments; ++i)
    {
        indices.push_back(0); // Center
        indices.push_back(2 + i * 4); // Current top vertex
        indices.push_back(2 + ((i + 1) % segments) * 4); // Next top vertex
    }

    // Bottom cap indices (looking up, counter-clockwise which appears clockwise from outside)
    for (ui32 i = 0; i < segments; ++i)
    {
        indices.push_back(1); // Center
        indices.push_back(3 + ((i + 1) % segments) * 4); // Next bottom vertex
        indices.push_back(3 + i * 4); // Current bottom vertex
    }

    // Side indices
    for (ui32 i = 0; i < segments; ++i)
    {
        ui32 topCurrent = 4 + i * 4;
        ui32 bottomCurrent = 5 + i * 4;
        ui32 topNext = 4 + ((i + 1) % segments) * 4;
        ui32 bottomNext = 5 + ((i + 1) % segments) * 4;

        indices.push_back(topCurrent);
        indices.push_back(bottomCurrent);
        indices.push_back(topNext);

        indices.push_back(topNext);
        indices.push_back(bottomCurrent);
        indices.push_back(bottomNext);
    }

    return std::make_shared<IndexBuffer>(
        indices.data(),
        static_cast<ui32>(indices.size()),
        resourceDesc
    );
}