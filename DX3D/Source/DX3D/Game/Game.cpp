#include <DX3D/Game/Game.h>
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Graphics/RenderSystem.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/Primitives/Rectangle.h>
#include <DX3D/Graphics/Shaders/RainbowShader.h>
#include <DX3D/Graphics/Shaders/GreenShader.h>

dx3d::Game::Game(const GameDesc& desc) :
    Base({ *std::make_unique<Logger>(desc.logLevel).release() }),
    m_loggerPtr(&m_logger)
{
    m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{ m_logger });
    m_display = std::make_unique<Display>(DisplayDesc{ {m_logger,desc.windowSize},m_graphicsEngine->getRenderSystem() });

    createRenderingResources();

    DX3DLogInfo("Game initialized with three rectangles (Left, Center, Right) using triangle strips.");
}

dx3d::Game::~Game()
{
    DX3DLogInfo("Game deallocation started.");
}

void dx3d::Game::createRenderingResources()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto resourceDesc = renderSystem.getGraphicsResourceDesc();

    m_rectangles.clear();

    m_rectangles.push_back(
        Rectangle::CreateAt(resourceDesc, -0.6f, 0.0f, 0.4f, 0.8f)
        
    );

    m_rectangles.push_back(
        Rectangle::CreateAt(resourceDesc, 0.0f, 0.0f, 0.4f, 0.8f)
        
    );

    m_rectangles.push_back(
        Rectangle::CreateAt(resourceDesc, 0.6f, 0.0f, 0.4f, 0.8f)
        
    );

    m_rainbowVertexShader = std::make_shared<VertexShader>(resourceDesc, RainbowShader::GetVertexShaderCode());
    m_rainbowPixelShader = std::make_shared<PixelShader>(resourceDesc, RainbowShader::GetPixelShaderCode());

    m_greenVertexShader = std::make_shared<VertexShader>(resourceDesc, GreenShader::GetVertexShaderCode());
    m_greenPixelShader = std::make_shared<PixelShader>(resourceDesc, GreenShader::GetPixelShaderCode());

    DX3DLogInfo("Three rectangles created successfully: Left, Center, Right.");
}

void dx3d::Game::render()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto& swapChain = m_display->getSwapChain();

    deviceContext.clearRenderTargetColor(swapChain, 0.0f, 0.0f, 0.0f, 1.0f);
    deviceContext.setRenderTargets(swapChain);
    deviceContext.setViewportSize(m_display->getSize().width, m_display->getSize().height);

    for (size_t i = 0; i < m_rectangles.size(); ++i)
    {
        deviceContext.setVertexBuffer(*m_rectangles[i]);

        
        if (i == 1) 
        {
            deviceContext.setVertexShader(m_greenVertexShader->getShader());
            deviceContext.setPixelShader(m_greenPixelShader->getShader());
            deviceContext.setInputLayout(m_greenVertexShader->getInputLayout());
        }
        else
        {
            deviceContext.setVertexShader(m_rainbowVertexShader->getShader());
            deviceContext.setPixelShader(m_rainbowPixelShader->getShader());
            deviceContext.setInputLayout(m_rainbowVertexShader->getInputLayout());
        }
        deviceContext.drawTriangleStrip(m_rectangles[i]->getVertexCount(), 0);
    }

    deviceContext.present(swapChain);
}