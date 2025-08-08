#pragma once
#include <memory>
#include <stdexcept>

namespace dx3d
{
    // Game Objects
    class AGameObject;
    class CameraObject;
    class Cube;
    class Plane;

    // Core Systems
    class Core;
    class Game;
    class Logger;
    class SelectionSystem;
    class ViewportManager;

    // Graphics
    class GraphicsEngine;
    class RenderSystem;
    class SwapChain;
    class Display;
    class Window;

    // Buffers
    class VertexBuffer;
    class IndexBuffer;
    class ConstantBuffer;
    class DepthBuffer;
    class RenderTexture;

    // Type Aliases
    using i32 = int;
    using ui32 = unsigned int;
    using f32 = float;
    using d64 = double;

    // Pointer Alias
    using SwapChainPtr = std::shared_ptr<SwapChain>;
}