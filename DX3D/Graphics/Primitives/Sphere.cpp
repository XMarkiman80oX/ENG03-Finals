#include <../Graphics/Primitives/Sphere.h>
#include <../Graphics/IndexBuffer.h>
#include <vector>
#include <cmath>

using namespace dx3d;

Sphere::Sphere() : BaseGameObject()
{
}

Sphere::Sphere(const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : BaseGameObject(position, rotation, scale)
{
}

void Sphere::update(float deltaTime)
{
    BaseGameObject::update(deltaTime);
}

std::shared_ptr<VertexBuffer> Sphere::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc,
    ui32 latitudeSegments, ui32 longitudeSegments)
{
    std::vector<Vertex> vertices;
    const float radius = 0.5f;

    for (ui32 lat = 0; lat <= latitudeSegments; ++lat)
    {
        float theta = lat * 3.14159265f / latitudeSegments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (ui32 lon = 0; lon <= longitudeSegments; ++lon)
        {
            float phi = lon * 2.0f * 3.14159265f / longitudeSegments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = radius * sinTheta * cosPhi;
            float y = radius * cosTheta;
            float z = radius * sinTheta * sinPhi;

            Vector3 position(x, y, z);
            Vector3 normal = Vector3::Normalize(position);

            float u = static_cast<float>(lon) / longitudeSegments;
            float v = static_cast<float>(lat) / latitudeSegments;
            Vector2 texCoord(u, v);

            float r = (x + radius) / (2.0f * radius);
            float g = (y + radius) / (2.0f * radius);
            float b = (z + radius) / (2.0f * radius);

            vertices.push_back({ position, {r, g, b, 1.0f}, normal, texCoord });
        }
    }

    return std::make_shared<VertexBuffer>(
        vertices.data(),
        sizeof(Vertex),
        static_cast<ui32>(vertices.size()),
        resourceDesc
    );
}

std::shared_ptr<IndexBuffer> Sphere::CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc,
    ui32 latitudeSegments, ui32 longitudeSegments)
{
    std::vector<ui32> indices;

    for (ui32 lat = 0; lat < latitudeSegments; ++lat)
    {
        for (ui32 lon = 0; lon < longitudeSegments; ++lon)
        {
            ui32 current = lat * (longitudeSegments + 1) + lon;
            ui32 next = current + longitudeSegments + 1;

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

