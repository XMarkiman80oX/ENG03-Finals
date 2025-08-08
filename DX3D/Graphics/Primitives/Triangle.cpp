#include <../Graphics/Primitives/Triangle.h>

std::shared_ptr<dx3d::VertexBuffer> dx3d::Triangle::Create(const GraphicsResourceDesc& resourceDesc)
{
    return CreateAt(resourceDesc, -0.55f, 0.0f, 0.6f);
}

std::shared_ptr<dx3d::VertexBuffer> dx3d::Triangle::CreateAt(
    const GraphicsResourceDesc& resourceDesc,
    float centerX, float centerY,
    float scale)
{
    float halfScale = scale * 0.5f;

    Vertex triangleVertices[] = {
        Vertex({centerX, centerY + halfScale, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}),
        Vertex({centerX + halfScale, centerY - halfScale, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}),
        Vertex({centerX - halfScale, centerY - halfScale, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f})
    };

    return std::make_shared<VertexBuffer>(
        triangleVertices,
        sizeof(Vertex),
        3,
        resourceDesc
    );
}