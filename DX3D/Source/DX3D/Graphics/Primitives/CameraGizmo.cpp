#include <DX3D/Graphics/Primitives/CameraGizmo.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Graphics/Vertex.h>
#include <vector>

using namespace dx3d;

std::shared_ptr<VertexBuffer> CameraGizmo::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc)
{
    std::vector<Vertex> vertices;

    // Main camera body (a simple box)
    vertices.push_back({ {-0.5f, -0.25f, -0.3f}, {0.7f, 0.7f, 0.7f, 1.0f} }); // 0
    vertices.push_back({ {0.5f, -0.25f, -0.3f}, {0.7f, 0.7f, 0.7f, 1.0f} }); // 1
    vertices.push_back({ {0.5f, 0.25f, -0.3f}, {0.7f, 0.7f, 0.7f, 1.0f} }); // 2
    vertices.push_back({ {-0.5f, 0.25f, -0.3f}, {0.7f, 0.7f, 0.7f, 1.0f} }); // 3
    vertices.push_back({ {-0.5f, -0.25f, 0.3f}, {0.7f, 0.7f, 0.7f, 1.0f} }); // 4
    vertices.push_back({ {0.5f, -0.25f, 0.3f}, {0.7f, 0.7f, 0.7f, 1.0f} }); // 5
    vertices.push_back({ {0.5f, 0.25f, 0.3f}, {0.7f, 0.7f, 0.7f, 1.0f} }); // 6
    vertices.push_back({ {-0.5f, 0.25f, 0.3f}, {0.7f, 0.7f, 0.7f, 1.0f} }); // 7

    // Top "viewfinder" triangle
    vertices.push_back({ {-0.2f, 0.25f, 0.0f}, {0.2f, 0.8f, 0.2f, 1.0f} }); // 8
    vertices.push_back({ {0.2f, 0.25f, 0.0f}, {0.2f, 0.8f, 0.2f, 1.0f} }); // 9
    vertices.push_back({ {0.0f, 0.45f, 0.0f}, {0.2f, 0.8f, 0.2f, 1.0f} }); // 10

    // Frustum lines (representing the camera's view)
    vertices.push_back({ {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f} }); // 11 (Center of camera)
    vertices.push_back({ {-0.3f, 0.2f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f} }); // 12 (Top-left)
    vertices.push_back({ {0.3f, 0.2f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f} }); // 13 (Top-right)
    vertices.push_back({ {0.3f, -0.2f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f} }); // 14 (Bottom-right)
    vertices.push_back({ {-0.3f, -0.2f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f} }); // 15 (Bottom-left)

    return std::make_shared<VertexBuffer>(
        vertices.data(),
        sizeof(Vertex),
        static_cast<ui32>(vertices.size()),
        resourceDesc
    );
}

std::shared_ptr<IndexBuffer> CameraGizmo::CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc)
{
    std::vector<ui32> indices = {
        // Camera body
        0, 1, 2, 0, 2, 3, // Front
        4, 5, 1, 4, 1, 0, // Bottom
        3, 2, 6, 3, 6, 7, // Top
        7, 6, 5, 7, 5, 4, // Back
        1, 5, 6, 1, 6, 2, // Right
        4, 0, 3, 4, 3, 7, // Left

        // Viewfinder
        8, 9, 10,

        // Frustum lines
        11, 12, 11, 13, 11, 14, 11, 15,

        // Frustum square
        12, 13, 13, 14, 14, 15, 15, 12
    };

    return std::make_shared<IndexBuffer>(
        indices.data(),
        static_cast<ui32>(indices.size()),
        resourceDesc
    );
}