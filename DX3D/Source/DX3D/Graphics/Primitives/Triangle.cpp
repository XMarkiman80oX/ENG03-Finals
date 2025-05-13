#include <DX3D/Graphics/Primitives/Triangle.h>

std::shared_ptr<dx3d::VertexBuffer> dx3d::Triangle::Create(const GraphicsResourceDesc& resourceDesc)
{
    // Define the triangle vertices
    Vertex triangleVertices[] = {
        { {0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },  // Top vertex (red)
        { {0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f} }, // Bottom right vertex (green)
        { {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f} } // Bottom left vertex (blue)
    };

    // Create the vertex buffer
    return std::make_shared<VertexBuffer>(
        triangleVertices,
        sizeof(Vertex),
        3,
        resourceDesc
    );
}