#pragma once
#include <stdexcept>
#include <memory>

namespace dx3d
{
    class Core;
    class Logger;
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
}