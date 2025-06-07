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

    DX3DLogInfo("Game initialized with triangle to parallelogram morphing animation.");
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

    // Create initial triangle shape (will be updated by animation)
    updateRectangleVertices(0.0f); // Start with triangle shape

    // Create rainbow shader for reliable color display
    m_transitionVertexShader = std::make_shared<VertexShader>(resourceDesc, RainbowShader::GetVertexShaderCode());
    m_transitionPixelShader = std::make_shared<PixelShader>(resourceDesc, RainbowShader::GetPixelShaderCode());

    DX3DLogInfo("Triangle to parallelogram morphing animation resources created successfully.");
}

float dx3d::Game::lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float dx3d::Game::simplifiedEasing(float t)
{
    // Simple smoothstep for consistent smooth animation without weird slowdowns
    return t * t * (3.0f - 2.0f * t);
}

void dx3d::Game::updateAnimation()
{
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_startTime);
    m_animationTime = elapsed.count() / 1000.0f;

    // 30-second total animation with 6 cycles (5 seconds each)
    float totalDuration = 30.0f;
    float cycleDuration = 5.0f;

    // Calculate which cycle we're in (0-5)
    float totalTime = fmod(m_animationTime, totalDuration);
    int currentCycle = static_cast<int>(totalTime / cycleDuration);
    float cycleTime = fmod(totalTime, cycleDuration) / cycleDuration;

    // Determine animation speed based on cycle
    float animationSpeed;
    if (currentCycle < 2) {
        // Cycles 0-1: Fast (complete 2 animations per cycle)
        animationSpeed = 2.0f;
    }
    else if (currentCycle < 4) {
        // Cycles 2-3: Slow (complete 0.5 animations per cycle)
        animationSpeed = 1.0f;
    }
    else {
        // Cycles 4-5: Very Fast (complete 4 animations per cycle)
        animationSpeed = 4.0f;
    }

    // Calculate the animation phase based on speed
    float animationTime = cycleTime * animationSpeed;
    float adjustedCycleTime = fmod(animationTime, 1.0f);

    // Create smooth back-and-forth motion within each cycle
    float animPhase;
    if (adjustedCycleTime < 0.5f) {
        // First half: triangle to parallelogram
        animPhase = adjustedCycleTime * 2.0f;
    }
    else {
        // Second half: parallelogram back to triangle
        animPhase = 2.0f - (adjustedCycleTime * 2.0f);
    }

    // Apply simplified easing
    float easedPhase = simplifiedEasing(animPhase);

    // Calculate morph amount (0.0 = triangle, 1.0 = parallelogram)
    float morphAmount = easedPhase;

    updateRectangleVertices(morphAmount);
}

void dx3d::Game::updateRectangleVertices()
{
    // This is the old method - we'll override it
    updateRectangleVertices(0.0f);
}

void dx3d::Game::updateRectangleVertices(float morphAmount)
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto resourceDesc = renderSystem.getGraphicsResourceDesc();

    // Define the triangle shape (Image 1) - right-pointing triangle (larger for better gradient visibility)
    float triangleVertices[4][3] = {
        {-0.6f, 0.4f, 0.0f},   // Top-left
        {0.8f, 0.0f, 0.0f},    // Right point
        {-0.6f, -0.4f, 0.0f},  // Bottom-left
        {0.8f, 0.0f, 0.0f}     // Duplicate right point for triangle strip
    };

    // Define the parallelogram shape (Image 2) - rotated parallelogram (larger)
    float parallelogramVertices[4][3] = {
        {-0.2f, 0.5f, 0.0f},   // Top-left
        {0.6f, 0.3f, 0.0f},    // Top-right
        {-0.6f, -0.3f, 0.0f},  // Bottom-left
        {0.2f, -0.5f, 0.0f}    // Bottom-right
    };

    // Interpolate between triangle and parallelogram based on morphAmount
    Vertex vertices[4];
    for (int i = 0; i < 4; i++) {
        vertices[i].position[0] = lerp(triangleVertices[i][0], parallelogramVertices[i][0], morphAmount);
        vertices[i].position[1] = lerp(triangleVertices[i][1], parallelogramVertices[i][1], morphAmount);
        vertices[i].position[2] = 0.0f;

        // White color for the transition shader to handle gradients
        vertices[i].color[0] = 0.0f;
        vertices[i].color[1] = 1.0f;
        vertices[i].color[2] = 1.0f;
        vertices[i].color[3] = 1.0f;
    }

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

    // Clear screen to dark teal (like in your images) instead of black
    deviceContext.clearRenderTargetColor(swapChain, 0.0f, 0.4f, 0.4f, 1.0f);
    deviceContext.setRenderTargets(swapChain);
    deviceContext.setViewportSize(m_display->getSize().width, m_display->getSize().height);

    // Render the morphing shape with smooth color transition
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