#include <DX3D/Game/Game.h>
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Graphics/RenderSystem.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Graphics/ConstantBuffer.h>
#include <DX3D/Graphics/DepthBuffer.h>
#include <DX3D/Graphics/Primitives/AGameObject.h>
#include <DX3D/Graphics/Primitives/Cube.h>
#include <DX3D/Graphics/Primitives/Plane.h>
#include <DX3D/Graphics/Shaders/Rainbow3DShader.h>      // Use 3D rainbow shader
#include <DX3D/Graphics/Shaders/WhiteShader.h>          // White shader for plane
#include <DX3D/Math/Math.h>
#include <cmath>
#include <random>
#include <DirectXMath.h>

dx3d::Game::Game(const GameDesc& desc) :
    Base({ *std::make_unique<Logger>(desc.logLevel).release() }),
    m_loggerPtr(&m_logger)
{
    m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{ m_logger });
    m_display = std::make_unique<Display>(DisplayDesc{ {m_logger,desc.windowSize},m_graphicsEngine->getRenderSystem() });

    m_previousTime = std::chrono::steady_clock::now();

    createRenderingResources();

    DX3DLogInfo("Game initialized with 3D rainbow cube and white plane.");
}

dx3d::Game::~Game()
{
    DX3DLogInfo("Game deallocation started.");
}

void dx3d::Game::createRenderingResources()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto resourceDesc = renderSystem.getGraphicsResourceDesc();

    // Create vertex and index buffers
    m_cubeVertexBuffer = Cube::CreateVertexBuffer(resourceDesc);
    m_cubeIndexBuffer = Cube::CreateIndexBuffer(resourceDesc);
    m_planeVertexBuffer = Plane::CreateVertexBuffer(resourceDesc);
    m_planeIndexBuffer = Plane::CreateIndexBuffer(resourceDesc);

    // CREATE DEPTH BUFFER for proper 3D depth testing
    const auto& windowSize = m_display->getSize();
    m_depthBuffer = std::make_shared<DepthBuffer>(
        windowSize.width,
        windowSize.height,
        resourceDesc
    );

    // CREATE MULTIPLE SHADERS
    // 3D Rainbow shader for cubes (FIXED - now has proper 3D transformations)
    m_rainbowVertexShader = std::make_shared<VertexShader>(resourceDesc, Rainbow3DShader::GetVertexShaderCode());
    m_rainbowPixelShader = std::make_shared<PixelShader>(resourceDesc, Rainbow3DShader::GetPixelShaderCode());

    // White shader for planes
    m_whiteVertexShader = std::make_shared<VertexShader>(resourceDesc, WhiteShader::GetVertexShaderCode());
    m_whitePixelShader = std::make_shared<PixelShader>(resourceDesc, WhiteShader::GetPixelShaderCode());

    m_transformConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(TransformationMatrices), resourceDesc);

    // Reserve space for objects
    m_gameObjects.reserve(2);
    m_objectRotationDeltas.reserve(2);

    // 1. Create rainbow cube at origin
    m_gameObjects.push_back(std::make_shared<Cube>(
        Vector3(0.0f, 0.0f, 0.0f),      // Cube at center
        Vector3(0.0f, 0.0f, 0.0f),      // No initial rotation
        Vector3(2.0f, 2.0f, 2.0f)       // Large cube
    ));
    m_objectRotationDeltas.push_back(Vector3(0.0f, 0.8f, 0.0f)); // Rotate for rainbow effect

    // 2. Create white plane cutting through the cube
    m_gameObjects.push_back(std::make_shared<Plane>(
        Vector3(0.0f, 0.0f, 0.0f),      // Same position as cube
        Vector3(-1.5708f, 0.0f, 0.0f),  // Horizontal cut
        Vector3(4.0f, 4.0f, 1.0f)       // Large plane
    ));
    m_objectRotationDeltas.push_back(Vector3(0.0f, 0.0f, 0.0f)); // Static plane

    // Setup camera
    float aspectRatio = static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height);

    // Camera positioned to see the colorful intersection
    m_viewMatrix = Matrix4x4::CreateLookAtLH(
        Vector3(6.0f, 4.0f, -6.0f),     // Camera position
        Vector3(0.0f, 0.0f, 0.0f),      // Look at intersection
        Vector3(0.0f, 1.0f, 0.0f)       // Up vector
    );

    m_projectionMatrix = Matrix4x4::CreatePerspectiveFovLH(
        1.0472f,        // 60 degrees
        aspectRatio,
        0.1f,
        100.0f
    );

    DX3DLogInfo("3D Rainbow cube and white plane created with proper transformations.");
}

void dx3d::Game::update()
{
    auto currentTime = std::chrono::steady_clock::now();
    m_deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_previousTime).count() / 1000000.0f;
    m_previousTime = currentTime;

    // Update all game objects
    for (size_t i = 0; i < m_gameObjects.size(); ++i)
    {
        m_gameObjects[i]->rotate(m_objectRotationDeltas[i] * m_deltaTime);
        m_gameObjects[i]->update(m_deltaTime);
    }
}

void dx3d::Game::render()
{
    update();

    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto& swapChain = m_display->getSwapChain();

    // Clear both color and depth buffers
    deviceContext.clearRenderTargetColor(swapChain, 0.1f, 0.1f, 0.2f, 1.0f);

    // Clear depth buffer
    ID3D11DeviceContext* d3dContext = deviceContext.getDeviceContext();
    d3dContext->ClearDepthStencilView(
        m_depthBuffer->getDepthStencilView(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.0f,
        0
    );

    // Set render targets with depth buffer
    ID3D11RenderTargetView* renderTargetView = swapChain.getRenderTargetView();
    ID3D11DepthStencilView* depthStencilView = m_depthBuffer->getDepthStencilView();
    d3dContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

    deviceContext.setViewportSize(m_display->getSize().width, m_display->getSize().height);

    // Set constant buffer (same for all shaders)
    ID3D11Buffer* cb = m_transformConstantBuffer->getBuffer();
    d3dContext->VSSetConstantBuffers(0, 1, &cb);

    // Render all objects with appropriate shaders
    for (const auto& gameObject : m_gameObjects)
    {
        // Set buffers and shaders based on object type
        if (auto cube = std::dynamic_pointer_cast<Cube>(gameObject))
        {
            // 3D RAINBOW CUBE
            deviceContext.setVertexBuffer(*m_cubeVertexBuffer);
            deviceContext.setIndexBuffer(*m_cubeIndexBuffer);

            // Use 3D rainbow shaders
            deviceContext.setVertexShader(m_rainbowVertexShader->getShader());
            deviceContext.setPixelShader(m_rainbowPixelShader->getShader());
            deviceContext.setInputLayout(m_rainbowVertexShader->getInputLayout());
        }
        else if (auto plane = std::dynamic_pointer_cast<Plane>(gameObject))
        {
            // WHITE PLANE
            deviceContext.setVertexBuffer(*m_planeVertexBuffer);
            deviceContext.setIndexBuffer(*m_planeIndexBuffer);

            // Use white shaders
            deviceContext.setVertexShader(m_whiteVertexShader->getShader());
            deviceContext.setPixelShader(m_whitePixelShader->getShader());
            deviceContext.setInputLayout(m_whiteVertexShader->getInputLayout());
        }

        // Set up transformation matrices (same for all objects)
        TransformationMatrices transformMatrices;

        DirectX::XMMATRIX world = gameObject->getWorldMatrix().toXMMatrix();
        DirectX::XMMATRIX view = m_viewMatrix.toXMMatrix();
        DirectX::XMMATRIX projection = m_projectionMatrix.toXMMatrix();

        transformMatrices.world = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(world));
        transformMatrices.view = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(view));
        transformMatrices.projection = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(projection));

        m_transformConstantBuffer->update(deviceContext, &transformMatrices);

        // Draw the object
        if (auto cube = std::dynamic_pointer_cast<Cube>(gameObject))
        {
            deviceContext.drawIndexed(Cube::GetIndexCount(), 0, 0);
        }
        else if (auto plane = std::dynamic_pointer_cast<Plane>(gameObject))
        {
            deviceContext.drawIndexed(Plane::GetIndexCount(), 0, 0);
        }
    }

    deviceContext.present(swapChain);
}