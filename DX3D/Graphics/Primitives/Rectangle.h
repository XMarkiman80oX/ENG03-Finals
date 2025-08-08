#pragma once
#include <../Graphics/VertexBuffer.h>
#include <memory>

namespace dx3d
{
    class Rectangle
    {
    public:
        static std::shared_ptr<VertexBuffer> Create(const GraphicsResourceDesc& resourceDesc);

        
        static std::shared_ptr<VertexBuffer> CreateAt(
            const GraphicsResourceDesc& resourceDesc,
            float centerX, float centerY,
            float width = 1.0f, float height = 1.0f
        );
    };
}