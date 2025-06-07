#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/Core/Core.h>
#include <DX3D/Math/Math.h>
#include <chrono>
#include <memory>
#include <vector>

// Forward declarations
namespace dx3d
{
    class VertexBuffer;
    class IndexBuffer;
    class ConstantBuffer;
    class VertexShader;
    class PixelShader;
    class Cube;
}

namespace dx3d
{
    class Game : public Base
    {
    public:
        explicit Game(const GameDesc& desc);
        virtual ~Game() override;

        virtual void run() final;
    private:
        void render();
        void createRenderingResources();
        void update();

    private:
        std::unique_ptr<Logger> m_loggerPtr{};
        std::unique_ptr<GraphicsEngine> m_graphicsEngine{};
        std::unique_ptr<Display> m_display{};
        bool m_isRunning{ true };

        // --- Cube Rendering Resources ---
        std::vector<std::shared_ptr<Cube>> m_cubes;
        std::vector<Vector3> m_cubeRotationDeltas; // Stores unique rotation vectors
        std::shared_ptr<VertexBuffer> m_cubeVertexBuffer;
        std::shared_ptr<IndexBuffer> m_cubeIndexBuffer;

        // Shaders for 3D transformation
        std::shared_ptr<VertexShader> m_transform3DVertexShader;
        std::shared_ptr<PixelShader> m_transform3DPixelShader;

        // Constant buffer for transformation matrices
        std::shared_ptr<ConstantBuffer> m_transformConstantBuffer;

        // View and Projection matrices
        Matrix4x4 m_viewMatrix;
        Matrix4x4 m_projectionMatrix;
        // --- End Cube Rendering Resources ---

        // Animation variables
        std::chrono::steady_clock::time_point m_previousTime;
        float m_deltaTime{ 0.0f };
    };
}