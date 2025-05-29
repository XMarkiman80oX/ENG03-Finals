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
#include <cmath>

dx3d::Game::Game(const GameDesc& desc) :
    Base({ *std::make_unique<Logger>(desc.logLevel).release() }),
    m_loggerPtr(&m_logger)
{
    m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{ m_logger });
    m_display = std::make_unique<Display>(DisplayDesc{ {m_logger,desc.windowSize},m_graphicsEngine->getRenderSystem() });

    // Initialize animation timer
    m_startTime = std::chrono::steady_clock::now();

    createRenderingResources();

    DX3DLogInfo("Game initialized with animated center rectangle using lerp.");
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

    m_rectangles.push_back(Rectangle::CreateAt(resourceDesc, 0.0f, 0.0f, 0.4f, 0.8f));

    m_rainbowVertexShader = std::make_shared<VertexShader>(resourceDesc, RainbowShader::GetVertexShaderCode());
    m_rainbowPixelShader = std::make_shared<PixelShader>(resourceDesc, RainbowShader::GetPixelShaderCode());

    DX3DLogInfo("Animated rectangle created successfully.");
}

float dx3d::Game::lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

void dx3d::Game::updateAnimation()
{
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_startTime);
    m_animationTime = elapsed.count() / 1000.0f;

    float time1 = m_animationTime * 0.5f;  
    float time2 = m_animationTime * 0.7f;  
    float time3 = m_animationTime * 0.3f;  

    float widthT = (std::sin(time1) + 1.0f) * 0.5f; 
    m_currentWidth = lerp(0.2f, 0.8f, widthT);

    float heightT = (std::sin(time2) + 1.0f) * 0.5f;
    m_currentHeight = lerp(0.3f, 1.2f, heightT);

    m_currentX = std::sin(time3) * 0.3f;           
    m_currentY = std::sin(time3 * 2.0f) * 0.2f;   

    updateRectangleVertices();
}

void dx3d::Game::updateRectangleVertices()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto resourceDesc = renderSystem.getGraphicsResourceDesc();

    // Clear and recreate the rectangle with new parameters
    m_rectangles.clear();
    m_rectangles.push_back(
        Rectangle::CreateAt(resourceDesc, m_currentX, m_currentY, m_currentWidth, m_currentHeight)
    );
}

void dx3d::Game::render()
{
    // Update animation before rendering
    updateAnimation();

    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto& swapChain = m_display->getSwapChain();

    // Clear screen to black
    deviceContext.clearRenderTargetColor(swapChain, 0.0f, 0.0f, 0.0f, 1.0f);
    deviceContext.setRenderTargets(swapChain);
    deviceContext.setViewportSize(m_display->getSize().width, m_display->getSize().height);

    // Render the animated rectangle
    if (!m_rectangles.empty())
    {
        deviceContext.setVertexBuffer(*m_rectangles[0]);
        deviceContext.setVertexShader(m_rainbowVertexShader->getShader());
        deviceContext.setPixelShader(m_rainbowPixelShader->getShader());
        deviceContext.setInputLayout(m_rainbowVertexShader->getInputLayout());
        deviceContext.drawTriangleStrip(m_rectangles[0]->getVertexCount(), 0);
    }

    deviceContext.present(swapChain);
}