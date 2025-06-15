#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/Core/Core.h>
#include <DX3D/Math/Math.h>
#include <chrono>
#include <memory>
#include <vector>
#include <d3d11.h> // FIX: Include d3d11 for ID3D11DepthStencilState

// Forward declarations
namespace dx3d
{
    class VertexBuffer;
    class IndexBuffer;
    class ConstantBuffer;
    class VertexShader;
    class PixelShader;
    class AGameObject;
    class Cube;
    class Plane;
    class DepthBuffer;
    class Camera;
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
        void processInput(float deltaTime);

    private:
        std::unique_ptr<Logger> m_loggerPtr{};
        std::unique_ptr<GraphicsEngine> m_graphicsEngine{};
        std::unique_ptr<Display> m_display{};
        bool m_isRunning{ true };

        // Camera
        std::unique_ptr<Camera> m_camera{};
        float m_cameraSpeed{ 5.0f };  // Units per second
        float m_mouseSensitivity{ 0.3f };

        // --- 3D Object Rendering Resources ---
        std::vector<std::shared_ptr<AGameObject>> m_gameObjects;
        std::vector<Vector3> m_objectRotationDeltas;

        std::shared_ptr<VertexBuffer> m_cubeVertexBuffer;
        std::shared_ptr<IndexBuffer> m_cubeIndexBuffer;
        std::shared_ptr<VertexBuffer> m_planeVertexBuffer;
        std::shared_ptr<IndexBuffer> m_planeIndexBuffer;

        // MULTIPLE SHADERS for different objects
        // Rainbow shader for cubes
        std::shared_ptr<VertexShader> m_rainbowVertexShader;
        std::shared_ptr<PixelShader> m_rainbowPixelShader;

        // White shader for planes
        std::shared_ptr<VertexShader> m_whiteVertexShader;
        std::shared_ptr<PixelShader> m_whitePixelShader;

        // Constant buffer for transformation matrices
        std::shared_ptr<ConstantBuffer> m_transformConstantBuffer;

        // Depth buffer for proper 3D depth testing
        std::shared_ptr<DepthBuffer> m_depthBuffer;

        // FIX: Declare the depth stencil states as member variables
        ID3D11DepthStencilState* m_solidDepthState = nullptr;
        ID3D11DepthStencilState* m_particleDepthState = nullptr;

        // Projection matrix (view matrix now comes from Camera)
        Matrix4x4 m_projectionMatrix;

        // Animation variables
        std::chrono::steady_clock::time_point m_previousTime;
        float m_deltaTime{ 0.0f };
    };
}