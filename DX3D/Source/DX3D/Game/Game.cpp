#include <DX3D/Game/Game.h>
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Graphics/RenderSystem.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/Primitives/Rectangle.h>
#include <DX3D/Graphics/Shaders/TransitionShader.h>
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

    DX3DLogInfo("Game initialized with rectangle to parallelogram morphing animation.");
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

    // Create initial rectangle
    m_rectangles.push_back(Rectangle::CreateAt(resourceDesc, 0.0f, 0.0f, 0.6f, 0.8f));

    // Create transition shader that handles the color blending internally
    m_transitionVertexShader = std::make_shared<VertexShader>(resourceDesc, TransitionShader::GetVertexShaderCode());
    m_transitionPixelShader = std::make_shared<PixelShader>(resourceDesc, TransitionShader::GetPixelShaderCode());

    DX3DLogInfo("Rectangle morphing animation resources created successfully.");
}

float dx3d::Game::lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float dx3d::Game::smoothstep(float t)
{
    // Smooth cubic interpolation for natural easing
    return t * t * (3.0f - 2.0f * t);
}

void dx3d::Game::updateAnimation()
{
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_startTime);
    m_animationTime = elapsed.count() / 1000.0f;

    // 8-second cycle: 4 seconds to transform, 4 seconds to transform back (slower)
    float cycleDuration = 8.0f;
    float cycleTime = fmod(m_animationTime, cycleDuration) / cycleDuration;

    // Create smooth back-and-forth motion
    float animPhase;
    if (cycleTime < 0.5f) {
        // First half: rectangle to parallelogram
        animPhase = cycleTime * 2.0f;
    }
    else {
        // Second half: parallelogram back to rectangle
        animPhase = 2.0f - (cycleTime * 2.0f);
    }

    // Apply smooth easing
    float smoothPhase = smoothstep(animPhase);

    // Calculate skew amount (0.0 = rectangle, 1.0 = parallelogram)
    float skewAmount = smoothPhase;

    // Update shape parameters with rightward movement
    m_currentX = lerp(-0.3f, 0.3f, smoothPhase); // Move from left to right
    m_currentY = 0.0f;
    m_currentWidth = 0.6f;
    m_currentHeight = 0.8f;

    updateRectangleVertices(skewAmount);
}

void dx3d::Game::updateRectangleVertices()
{
    // This is the old method - we'll override it
    updateRectangleVertices(0.0f);
}

void dx3d::Game::updateRectangleVertices(float skewAmount)
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto resourceDesc = renderSystem.getGraphicsResourceDesc();

    // Calculate vertices manually with skew applied
    float halfWidth = m_currentWidth * 0.5f;
    float halfHeight = m_currentHeight * 0.5f;

    // Skew offset - applied to top vertices to create parallelogram
    float skewOffset = skewAmount * 0.3f; // Maximum skew

    // Create vertices for triangle strip (same order as Rectangle class)
    Vertex vertices[] = {
        // Top-left (with skew)
        { {m_currentX - halfWidth + skewOffset, m_currentY + halfHeight, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },
        // Top-right (with skew)
        { {m_currentX + halfWidth + skewOffset, m_currentY + halfHeight, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },
        // Bottom-left (no skew)
        { {m_currentX - halfWidth, m_currentY - halfHeight, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },
        // Bottom-right (no skew)
        { {m_currentX + halfWidth, m_currentY - halfHeight, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }
    };

    // Recreate the vertex buffer with new vertices
    m_rectangles.clear();
    m_rectangles.push_back(
        std::make_shared<VertexBuffer>(
            vertices,
            sizeof(Vertex),
            4,
            resourceDesc
        )
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

    // Render the morphing rectangle with smooth color transition
    if (!m_rectangles.empty())
    {
        deviceContext.setVertexBuffer(*m_rectangles[0]);

        // Use the transition shader that handles color blending internally
        deviceContext.setVertexShader(m_transitionVertexShader->getShader());
        deviceContext.setPixelShader(m_transitionPixelShader->getShader());
        deviceContext.setInputLayout(m_transitionVertexShader->getInputLayout());

        deviceContext.drawTriangleStrip(m_rectangles[0]->getVertexCount(), 0);
    }

    deviceContext.present(swapChain);
}