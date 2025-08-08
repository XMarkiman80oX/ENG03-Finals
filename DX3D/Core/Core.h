#pragma once
#include <stdexcept>
#include <memory>
#include "../Core/Common.h"

namespace dx3d
{
    class EventLog;
    class Game;

    class GraphicsEngine;
    class RenderSystem;
    class SwapChain;
    class Display;

    class VertexBuffer;
    class IndexBuffer;
    class ConstantBuffer;
    class DepthBuffer;
    class RenderTexture;
    class Window;

    class AGameObject;
    class CameraObject;
    class SelectionSystem;
    class ViewportManager;
    class Cube;
    class Plane;

    using i32 = int;
    using ui32 = unsigned int;
    using f32 = float;
    using d64 = double;

    using SwapChainPtr = std::shared_ptr<SwapChain>;

    class Core
    {
    public:
        explicit Core(const BaseDesc& desc);
        virtual EventLog& getLoggerInstance() const noexcept final;
        virtual ~Core();

    protected:
        Core(const Core&) = delete;
        Core(Core&&) = delete;
        Core& operator = (const Core&) = delete;
        Core& operator=(Core&&) = delete;

    protected:
        EventLog& m_loggerInstance;
    };
}