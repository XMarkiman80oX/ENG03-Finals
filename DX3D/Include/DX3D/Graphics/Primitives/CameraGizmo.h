#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Math/Math.h>
#include <memory>

namespace dx3d
{
    class VertexBuffer;
    class IndexBuffer;
    class GraphicsResourceDesc;

    class CameraGizmo
    {
    public:
        static std::shared_ptr<VertexBuffer> CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc);
        static std::shared_ptr<IndexBuffer> CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc);
        static ui32 GetIndexCount() { return 18; }
    };
}