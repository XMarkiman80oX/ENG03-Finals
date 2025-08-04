#include <DX3D/Graphics/Primitives/Sphere.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <vector>
#include <cmath>

using namespace dx3d;

Sphere::Sphere() : AGameObject()
{
}

Sphere::Sphere(const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : AGameObject(position, rotation, scale)
{
}

void Sphere::update(float deltaTime)
{
    AGameObject::update(deltaTime);
}

std::shared_ptr<VertexBuffer> Sphere::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc,
    ui32 latitudeSegments, ui32 longitudeSegments)
{
    std::vector<Vertex> vertices;
    const float radius = 0.5f;

    // Generate vertices
    for (ui32 lat = 0; lat <= latitudeSegments; ++lat)
    {
        float theta = lat * 3.14159265f / latitudeSegments; // 0 to PI
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (ui32 lon = 0; lon <= longitudeSegments; ++lon)
        {
            float phi = lon * 2.0f * 3.14159265f / longitudeSegments; // 0 to 2PI
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            // Calculate position
            float x = radius * sinTheta * cosPhi;
            float y = radius * cosTheta;
            float z = radius * sinTheta * sinPhi;

            // Map position to color for rainbow effect
            float r = (x + radius) / (2.0f * radius);
            float g = (y + radius) / (2.0f * radius);
            float b = (z + radius) / (2.0f * radius);

            vertices.push_back({ {x, y, z}, {r, g, b, 1.0f} });
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
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    return std::make_shared<IndexBuffer>(
        indices.data(),
        static_cast<ui32>(indices.size()),
        resourceDesc
    );
}