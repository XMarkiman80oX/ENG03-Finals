#pragma once
#include <stdexcept>
#include <memory>

namespace dx3d
{
    class Base;
    class Window;
    class Game;
    class GraphicsEngine;
    class RenderSystem;
    class Logger;
    class SwapChain;
    class Display;
    class VertexBuffer;
    class IndexBuffer;
    class ConstantBuffer;
    class DepthBuffer;
    class RenderTexture;

    class AGameObject;
    class Cube;
    class Plane;
    class CameraObject;
    class ViewportManager;
    class SelectionSystem;

    using i32 = int;
    using ui32 = unsigned int;
    using f32 = float;
    using d64 = double;

    using SwapChainPtr = std::shared_ptr<SwapChain>;
}