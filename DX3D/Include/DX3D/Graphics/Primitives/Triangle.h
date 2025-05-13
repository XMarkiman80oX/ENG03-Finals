#pragma once
#include <DX3D/Graphics/VertexBuffer.h>
#include <memory>

namespace dx3d
{
    class Triangle
    {
    public:
        static std::shared_ptr<VertexBuffer> Create(const GraphicsResourceDesc& resourceDesc);
    };
}