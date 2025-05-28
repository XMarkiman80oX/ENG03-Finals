#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/Core/Core.h>
#include <vector>

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

    private:
        std::unique_ptr<Logger> m_loggerPtr{};
        std::unique_ptr<GraphicsEngine> m_graphicsEngine{};
        std::unique_ptr<Display> m_display{};
        bool m_isRunning{ true };

        // Only three rectangles: Left, Center, Right
        std::vector<std::shared_ptr<VertexBuffer>> m_rectangles{};

        // Shaders
        std::shared_ptr<VertexShader> m_rainbowVertexShader{};
        std::shared_ptr<PixelShader> m_rainbowPixelShader{};
        std::shared_ptr<VertexShader> m_greenVertexShader{};
        std::shared_ptr<PixelShader> m_greenPixelShader{};
    };
}