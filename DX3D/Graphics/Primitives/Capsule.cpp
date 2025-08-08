#include <../Graphics/Primitives/Capsule.h>
#include <../Graphics/IndexBuffer.h>
#include <vector>
#include <cmath>

using namespace dx3d;

Capsule::Capsule() : AGameObject()
{
}

Capsule::Capsule(const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : AGameObject(position, rotation, scale)
{
}

void Capsule::update(float deltaTime)
{
    AGameObject::update(deltaTime);
}

std::shared_ptr<VertexBuffer> Capsule::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc,
    ui32 segments, ui32 rings)
{
    std::vector<Vertex> vertices;
    const float radius = 0.5f;
    const float cylinderHeight = 0.5f;
    const float halfCylinderHeight = cylinderHeight * 0.5f;

    for (ui32 ring = 0; ring <= rings / 2; ++ring)
    {
        float phi = ring * 3.14159265f / rings;
        float y = halfCylinderHeight + radius * std::cos(phi);
        float ringRadius = radius * std::sin(phi);

        for (ui32 seg = 0; seg <= segments; ++seg)
        {
            float theta = seg * 2.0f * 3.14159265f / segments;
            float x = ringRadius * std::cos(theta);
            float z = ringRadius * std::sin(theta);

            Vector3 position(x, y, z);
            Vector3 normal = Vector3::Normalize(Vector3(x, y - halfCylinderHeight, z));

            float u = static_cast<float>(seg) / segments;
            float v = static_cast<float>(ring) / (rings / 2);
            Vector2 texCoord(u, v * 0.25f);

            float t = static_cast<float>(ring) / (rings / 2);
            vertices.push_back({ position, {0.5f + 0.5f * t, 0.2f, 1.0f - 0.5f * t, 1.0f}, normal, texCoord });
        }
    }

    for (ui32 i = 0; i <= 1; ++i)
    {
        float y = halfCylinderHeight - i * cylinderHeight;

        for (ui32 seg = 0; seg <= segments; ++seg)
        {
            float theta = seg * 2.0f * 3.14159265f / segments;
            float x = radius * std::cos(theta);
            float z = radius * std::sin(theta);

            Vector3 position(x, y, z);
            Vector3 normal = Vector3::Normalize(Vector3(x, 0.0f, z));

            float u = static_cast<float>(seg) / segments;
            float v = 0.25f + i * 0.5f;
            Vector2 texCoord(u, v);

            float t = static_cast<float>(i);
            vertices.push_back({ position, {0.2f, 1.0f - 0.5f * t, 0.8f + 0.2f * t, 1.0f}, normal, texCoord });
        }
    }

    for (ui32 ring = rings / 2; ring <= rings; ++ring)
    {
        float phi = ring * 3.14159265f / rings;
        float y = -halfCylinderHeight + radius * std::cos(phi);
        float ringRadius = radius * std::sin(phi);

        for (ui32 seg = 0; seg <= segments; ++seg)
        {
            float theta = seg * 2.0f * 3.14159265f / segments;
            float x = ringRadius * std::cos(theta);
            float z = ringRadius * std::sin(theta);

            Vector3 position(x, y, z);
            Vector3 normal = Vector3::Normalize(Vector3(x, y + halfCylinderHeight, z));

            float u = static_cast<float>(seg) / segments;
            float v = 0.75f + static_cast<float>(ring - rings / 2) / (rings / 2) * 0.25f;
            Vector2 texCoord(u, v);

            float t = static_cast<float>(ring - rings / 2) / (rings / 2);
            vertices.push_back({ position, {0.5f + 0.5f * t, 1.0f - 0.3f * t, 0.2f, 1.0f}, normal, texCoord });
        }
    }

    return std::make_shared<VertexBuffer>(
        vertices.data(),
        sizeof(Vertex),
        static_cast<ui32>(vertices.size()),
        resourceDesc
    );
}

std::shared_ptr<IndexBuffer> Capsule::CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc,
    ui32 segments, ui32 rings)
{
    std::vector<ui32> indices;
    ui32 verticesPerRing = segments + 1;

    for (ui32 ring = 0; ring < rings / 2; ++ring)
    {
        for (ui32 seg = 0; seg < segments; ++seg)
        {
            ui32 current = ring * verticesPerRing + seg;
            ui32 next = current + verticesPerRing;

            indices.push_back(current);
            indices.push_back(current + 1);
            indices.push_back(next);

            indices.push_back(current + 1);
            indices.push_back(next + 1);
            indices.push_back(next);
        }
    }

    ui32 cylinderStart = (rings / 2 + 1) * verticesPerRing;
    for (ui32 seg = 0; seg < segments; ++seg)
    {
        ui32 topCurrent = cylinderStart + seg;
        ui32 bottomCurrent = topCurrent + verticesPerRing;

        indices.push_back(topCurrent);
        indices.push_back(topCurrent + 1);
        indices.push_back(bottomCurrent);

        indices.push_back(topCurrent + 1);
        indices.push_back(bottomCurrent + 1);
        indices.push_back(bottomCurrent);
    }

    ui32 bottomStart = cylinderStart + 2 * verticesPerRing;
    for (ui32 ring = 0; ring < rings / 2; ++ring)
    {
        for (ui32 seg = 0; seg < segments; ++seg)
        {
            ui32 current = bottomStart + ring * verticesPerRing + seg;
            ui32 next = current + verticesPerRing;

            indices.push_back(current);
            indices.push_back(current + 1);
            indices.push_back(next);

            indices.push_back(current + 1);
            indices.push_back(next + 1);
            indices.push_back(next);
        }
    }

    return std::make_shared<IndexBuffer>(
        indices.data(),
        static_cast<ui32>(indices.size()),
        resourceDesc
    );
}