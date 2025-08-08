#pragma once
#include <../Graphics/VertexBuffer.h>
#include <memory>

namespace dx3d
{
    class Triangle
    {
    public:
        // Create a triangle positioned on the left (matches original Game.cpp layout)
        static std::shared_ptr<VertexBuffer> Create(const GraphicsResourceDesc& resourceDesc);

        // Create a triangle at a specific position with custom size
        static std::shared_ptr<VertexBuffer> CreateAt(
            const GraphicsResourceDesc& resourceDesc,
            float centerX, float centerY,
            float scale = 1.0f
        );
    };
}