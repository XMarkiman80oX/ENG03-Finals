#include <DX3D/Game/Game.h>
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Graphics/RenderSystem.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/Primitives/Triangle.h>
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

    DX3DLogInfo("Game initialized with multiple render objects.");
}

dx3d::Game::~Game()
{
    DX3DLogInfo("Game deallocation started.");
}

void dx3d::Game::createRenderingResources()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto resourceDesc = renderSystem.getGraphicsResourceDesc();

    Vertex triangleVertices[] = {
        { {-0.55f, 0.3f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },  
        { {-0.35f, -0.3f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f} }, 
        { {-0.75f, -0.3f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f} }  
    };
    m_triangleVertexBuffer = std::make_shared<VertexBuffer>(triangleVertices, sizeof(Vertex), 3, resourceDesc);

    Vertex rectangleVertices[] = {
        // First triangle
        { {-0.2f, 0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },   
        { {0.2f, 0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },    
        { {-0.2f, -0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },  

        // Second triangle
        { {0.2f, 0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },    
        { {0.2f, -0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },  
        { {-0.2f, -0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }   
    };
    m_rectangleVertexBuffer = std::make_shared<VertexBuffer>(rectangleVertices, sizeof(Vertex), 6, resourceDesc);

    Vertex greenRectVertices[] = {
        // First triangle
        { {0.35f, 0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },   
        { {0.75f, 0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },  
        { {0.35f, -0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },  

        // Second triangle
        { {0.75f, 0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },  
        { {0.75f, -0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },  
        { {0.35f, -0.3f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }   
    };
    m_greenRectangleVertexBuffer = std::make_shared<VertexBuffer>(greenRectVertices, sizeof(Vertex), 6, resourceDesc);

    m_rainbowVertexShader = std::make_shared<VertexShader>(resourceDesc, RainbowShader::GetVertexShaderCode());
    m_rainbowPixelShader = std::make_shared<PixelShader>(resourceDesc, RainbowShader::GetPixelShaderCode());

    m_greenVertexShader = std::make_shared<VertexShader>(resourceDesc, GreenShader::GetVertexShaderCode());
    m_greenPixelShader = std::make_shared<PixelShader>(resourceDesc, GreenShader::GetPixelShaderCode());

    DX3DLogInfo("All rendering resources created successfully.");
}

void dx3d::Game::render()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto& swapChain = m_display->getSwapChain();

    deviceContext.clearRenderTargetColor(swapChain, 0.0f, 0.0f, 0.0f, 1.0f);

    deviceContext.setRenderTargets(swapChain);

    deviceContext.setViewportSize(m_display->getSize().width, m_display->getSize().height);

    {
        deviceContext.setVertexBuffer(*m_triangleVertexBuffer);
        deviceContext.setVertexShader(m_rainbowVertexShader->getShader());
        deviceContext.setPixelShader(m_rainbowPixelShader->getShader());
        deviceContext.setInputLayout(m_rainbowVertexShader->getInputLayout());
        deviceContext.drawTriangleList(m_triangleVertexBuffer->getVertexCount(), 0);
    }

    {
        deviceContext.setVertexBuffer(*m_rectangleVertexBuffer);
        deviceContext.setVertexShader(m_rainbowVertexShader->getShader());
        deviceContext.setPixelShader(m_rainbowPixelShader->getShader());
        deviceContext.setInputLayout(m_rainbowVertexShader->getInputLayout());
        deviceContext.drawTriangleList(m_rectangleVertexBuffer->getVertexCount(), 0);
    }

    {
        deviceContext.setVertexBuffer(*m_greenRectangleVertexBuffer);
        deviceContext.setVertexShader(m_greenVertexShader->getShader());
        deviceContext.setPixelShader(m_greenPixelShader->getShader());
        deviceContext.setInputLayout(m_greenVertexShader->getInputLayout());
        deviceContext.drawTriangleList(m_greenRectangleVertexBuffer->getVertexCount(), 0);
    }

    deviceContext.present(swapChain);
}