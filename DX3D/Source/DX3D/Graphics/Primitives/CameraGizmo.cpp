#include <DX3D/Graphics/Primitives/CameraGizmo.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Graphics/Vertex.h>
#include <vector>

// It's best practice to not use "using namespace" in global scope in source files.
// We'll fully qualify the names or put the "using" declaration inside the namespace.

namespace dx3d // <-- Add this opening brace
{
    std::shared_ptr<VertexBuffer> CameraGizmo::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc)
    {
        std::vector<Vertex> vertices;

        vertices.push_back({ {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f} });
        vertices.push_back({ {-0.3f, 0.2f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f} });
        vertices.push_back({ {0.3f, 0.2f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f} });
        vertices.push_back({ {0.3f, -0.2f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f} });
        vertices.push_back({ {-0.3f, -0.2f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f} });

        vertices.push_back({ {-0.5f, 0.3f, -0.8f}, {0.8f, 0.8f, 0.0f, 1.0f} });
        vertices.push_back({ {0.5f, 0.3f, -0.8f}, {0.8f, 0.8f, 0.0f, 1.0f} });
        vertices.push_back({ {0.5f, -0.3f, -0.8f}, {0.8f, 0.8f, 0.0f, 1.0f} });
        vertices.push_back({ {-0.5f, -0.3f, -0.8f}, {0.8f, 0.8f, 0.0f, 1.0f} });

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
            0, 1, 2,
            0, 2, 3,
            0, 3, 4,
            0, 4, 1,

            5, 6, 7,
            5, 7, 8
        };

        return std::make_shared<IndexBuffer>(
            indices.data(),
            static_cast<ui32>(indices.size()),
            resourceDesc
        );
    }

} // <--- And this closing brace