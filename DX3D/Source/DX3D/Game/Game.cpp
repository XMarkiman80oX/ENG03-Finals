#include <DX3D/Game/Game.h>
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Graphics/RenderSystem.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/Primitives/Triangle.h>
#include <DX3D/Graphics/Shaders/ColorShader.h>

dx3d::Game::Game(const GameDesc& desc) :
    Base({ *std::make_unique<Logger>(desc.logLevel).release() }),
    m_loggerPtr(&m_logger)
{
    m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{ m_logger });
    m_display = std::make_unique<Display>(DisplayDesc{ {m_logger,desc.windowSize},m_graphicsEngine->getRenderSystem() });

    createTriangleResources();

    DX3DLogInfo("Game initialized.");
}

dx3d::Game::~Game()
{
    DX3DLogInfo("Game deallocation started.");
}

void dx3d::Game::createTriangleResources()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto resourceDesc = renderSystem.getGraphicsResourceDesc();

    // Create triangle vertices
    m_triangleVertexBuffer = Triangle::Create(resourceDesc);

    // Create shaders
    m_vertexShader = std::make_shared<VertexShader>(resourceDesc, ColorShader::GetVertexShaderCode());
    m_pixelShader = std::make_shared<PixelShader>(resourceDesc, ColorShader::GetPixelShaderCode());

    DX3DLogInfo("Triangle resources created successfully.");
}

void dx3d::Game::render()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto& swapChain = m_display->getSwapChain();

    // Clear the render target
    deviceContext.clearRenderTargetColor(swapChain, 0.0f, 0.2f, 0.4f, 1.0f);

    // Set the render target view
    deviceContext.setRenderTargets(swapChain);

    // Set up the viewport
    deviceContext.setViewportSize(m_display->getSize().width, m_display->getSize().height);

    // Set the vertex buffer
    deviceContext.setVertexBuffer(*m_triangleVertexBuffer);

    // Set the shaders and input layout
    deviceContext.setVertexShader(m_vertexShader->getShader());
    deviceContext.setPixelShader(m_pixelShader->getShader());
    deviceContext.setInputLayout(m_vertexShader->getInputLayout());

    // Draw the triangle
    deviceContext.drawTriangleList(m_triangleVertexBuffer->getVertexCount(), 0);

    // Present the frame
    deviceContext.present(swapChain);
}