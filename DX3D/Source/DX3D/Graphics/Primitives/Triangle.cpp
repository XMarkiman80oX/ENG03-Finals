#include <DX3D/Graphics/Primitives/Triangle.h>

std::shared_ptr<dx3d::VertexBuffer> dx3d::Triangle::Create(const GraphicsResourceDesc& resourceDesc)
{
    // Create triangle positioned on the left to match original layout
    return CreateAt(resourceDesc, -0.55f, 0.0f, 0.6f);
}

std::shared_ptr<dx3d::VertexBuffer> dx3d::Triangle::CreateAt(
    const GraphicsResourceDesc& resourceDesc,
    float centerX, float centerY,
    float scale)
{
    // Define the triangle vertices relative to center, then scale and translate
    float halfScale = scale * 0.5f;

    Vertex triangleVertices[] = {
        { {centerX, centerY + halfScale, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },           // Top vertex (red)
        { {centerX + halfScale, centerY - halfScale, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f} }, // Bottom right vertex (green)  
        { {centerX - halfScale, centerY - halfScale, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f} }  // Bottom left vertex (blue)
    };

    // Create the vertex buffer
    return std::make_shared<VertexBuffer>(
        triangleVertices,
        sizeof(Vertex),
        3,
        resourceDesc
    );
}