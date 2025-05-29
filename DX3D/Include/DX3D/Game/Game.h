#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/Core/Core.h>
#include <vector>
#include <chrono>

// Forward declarations
namespace dx3d
{
    class VertexBuffer;
    class VertexShader;
    class PixelShader;
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
        void updateAnimation();
        void updateRectangleVertices();
        float lerp(float a, float b, float t);

    private:
        std::unique_ptr<Logger> m_loggerPtr{};
        std::unique_ptr<GraphicsEngine> m_graphicsEngine{};
        std::unique_ptr<Display> m_display{};
        bool m_isRunning{ true };

        // Single animated rectangle
        std::vector<std::shared_ptr<VertexBuffer>> m_rectangles{};

        // Shaders
        std::shared_ptr<VertexShader> m_rainbowVertexShader{};
        std::shared_ptr<PixelShader> m_rainbowPixelShader{};

        // Animation variables
        std::chrono::steady_clock::time_point m_startTime;
        float m_animationTime{ 0.0f };

        // Rectangle shape parameters for animation
        float m_currentWidth{ 0.4f };
        float m_currentHeight{ 0.8f };
        float m_currentX{ 0.0f };
        float m_currentY{ 0.0f };
    };
}