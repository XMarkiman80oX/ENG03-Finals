#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/Core/Core.h>

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
        void createTriangleResources();

    private:
        std::unique_ptr<Logger> m_loggerPtr{};
        std::unique_ptr<GraphicsEngine> m_graphicsEngine{};
        std::unique_ptr<Display> m_display{};
        bool m_isRunning{ true };

        // Triangle rendering resources
        std::shared_ptr<VertexBuffer> m_triangleVertexBuffer{};
        std::shared_ptr<VertexShader> m_vertexShader{};
        std::shared_ptr<PixelShader> m_pixelShader{};
    };
}