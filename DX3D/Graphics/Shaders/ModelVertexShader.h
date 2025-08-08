#pragma once
#include <memory>

namespace dx3d {
    class VertexShader;
    struct GraphicsResourceDesc;

    std::shared_ptr<VertexShader> createModelVertexShader(const GraphicsResourceDesc& desc);
}