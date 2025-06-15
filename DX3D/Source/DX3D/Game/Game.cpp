#include <DX3D/Game/Game.h>
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Game/Camera.h>
#include <DX3D/Input/Input.h>
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
#include <DX3D/Graphics/Shaders/Rainbow3DShader.h>
#include <DX3D/Graphics/Shaders/WhiteShader.h>
#include <DX3D/Math/Math.h>
#include <DX3D/Particles/ParticleSystem.h>
#include <DX3D/Particles/ParticleEffects/SnowParticle.h>
#include <cmath>
#include <random>
#include <string>
#include <cstdio>
#include <DirectXMath.h>

dx3d::Game::Game(const GameDesc& desc) :
    Base({ *std::make_unique<Logger>(desc.logLevel).release() }),
    m_loggerPtr(&m_logger)
{
    m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{ m_logger });
    m_display = std::make_unique<Display>(DisplayDesc{ {m_logger,desc.windowSize},m_graphicsEngine->getRenderSystem() });

    m_previousTime = std::chrono::steady_clock::now();

    createRenderingResources();

    DX3DLogInfo("Game initialized with Camera, Input system, and Particle system.");
}

dx3d::Game::~Game()
{
    DX3DLogInfo("Game deallocation started.");

    // Shutdown particle system
    ParticleSystem::getInstance().shutdown();
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

    // CREATE DEPTH BUFFER
    const auto& windowSize = m_display->getSize();
    m_depthBuffer = std::make_shared<DepthBuffer>(
        windowSize.width,
        windowSize.height,
        resourceDesc
    );

    m_rainbowVertexShader = std::make_shared<VertexShader>(resourceDesc, Rainbow3DShader::GetVertexShaderCode());
    m_rainbowPixelShader = std::make_shared<PixelShader>(resourceDesc, Rainbow3DShader::GetPixelShaderCode());
    m_whiteVertexShader = std::make_shared<VertexShader>(resourceDesc, WhiteShader::GetVertexShaderCode());
    m_whitePixelShader = std::make_shared<PixelShader>(resourceDesc, WhiteShader::GetPixelShaderCode());

    m_transformConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(TransformationMatrices), resourceDesc);

    m_gameObjects.reserve(2);
    m_objectRotationDeltas.reserve(2);

    m_gameObjects.push_back(std::make_shared<Cube>(
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(2.0f, 2.0f, 2.0f)
    ));
    m_objectRotationDeltas.push_back(Vector3(0.0f, 0.8f, 0.0f));

    // 2. Create white plane cutting through the cube
    m_gameObjects.push_back(std::make_shared<Plane>(
        Vector3(0.0f, 0.0f, 0.0f),      // Same position as cube
        Vector3(-1.5708f, 0.0f, 0.0f),  // Horizontal cut
        Vector3(4.0f, 4.0f, 1.0f)       // Large plane
    ));
    m_objectRotationDeltas.push_back(Vector3(0.0f, 0.0f, 0.0f)); // Static plane

    // Create camera
    m_camera = std::make_unique<Camera>(
        Vector3(6.0f, 4.0f, -6.0f),     // Initial position
        Vector3(0.0f, 0.0f, 0.0f)       // Look at origin
    );

    // Setup projection matrix
    float aspectRatio = static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height);
    m_projectionMatrix = Matrix4x4::CreatePerspectiveFovLH(
        1.0472f,        // 60 degrees
        aspectRatio,
        0.1f,
        100.0f
    );

    // Initialize particle system
    ParticleSystem::getInstance().initialize(*m_graphicsEngine);

    // Create a snow emitter
    ParticleEmitter::EmitterConfig snowConfig;
    snowConfig.position = Vector3(0.0f, 10.0f, 0.0f);  // Emit from above
    snowConfig.positionVariance = Vector3(20.0f, 0.0f, 20.0f);  // Spread over a wide area
    snowConfig.velocity = Vector3(0.0f, -2.0f, 0.0f);  // Fall downward
    snowConfig.velocityVariance = Vector3(0.5f, 0.5f, 0.5f);
    snowConfig.acceleration = Vector3(0.0f, -0.5f, 0.0f);  // Gentle gravity
    snowConfig.startColor = Vector4(1.0f, 1.0f, 1.0f, 0.8f);  // White with slight transparency
    snowConfig.endColor = Vector4(0.9f, 0.9f, 1.0f, 0.0f);   // Fade to transparent
    snowConfig.startSize = 0.2f;
    snowConfig.endSize = 0.1f;
    snowConfig.lifetime = 8.0f;
    snowConfig.lifetimeVariance = 2.0f;
    snowConfig.emissionRate = 50.0f;  // 50 snowflakes per second
    snowConfig.maxParticles = 2000;

    auto snowEmitter = ParticleSystem::getInstance().createEmitter(
        "snow",
        snowConfig,
        createSnowParticle  // Factory function from SnowParticle.h
    );

    DX3DLogInfo("Snow particle emitter created.");
    DX3DLogInfo("Camera created. Hold right mouse button + WASD to move camera.");
}

void dx3d::Game::processInput(float deltaTime)
{
    auto& input = Input::getInstance();

    // Only process camera movement when right mouse button is held
    if (input.isMouseButtonPressed(MouseButton::Right))
    {
        // Camera movement
        float moveSpeed = m_cameraSpeed * deltaTime;
        bool moved = false;

        if (input.isKeyPressed(KeyCode::W))
        {
            m_camera->moveForward(moveSpeed);
            moved = true;
        }
        if (input.isKeyPressed(KeyCode::S))
        {
            m_camera->moveBackward(moveSpeed);
            moved = true;
        }
        if (input.isKeyPressed(KeyCode::A))
        {
            m_camera->moveLeft(moveSpeed);
            moved = true;
        }
        if (input.isKeyPressed(KeyCode::D))
        {
            m_camera->moveRight(moveSpeed);
            moved = true;
        }

        // Vertical movement
        if (input.isKeyPressed(KeyCode::Q))
        {
            m_camera->moveDown(moveSpeed);
            moved = true;
        }
        if (input.isKeyPressed(KeyCode::E))
        {
            m_camera->moveUp(moveSpeed);
            moved = true;
        }

        // Log camera position if moved
        if (moved)
        {
            const auto& pos = m_camera->getPosition();
            printf("[Camera] Position: (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
        }

        // Mouse look (only when right button is held)
        float mouseDeltaX = static_cast<float>(input.getMouseDeltaX());
        float mouseDeltaY = static_cast<float>(input.getMouseDeltaY());

        if (mouseDeltaX != 0.0f || mouseDeltaY != 0.0f)
        {
            m_camera->onMouseMove(mouseDeltaX, mouseDeltaY, m_mouseSensitivity * 0.01f);
        }
    }

    // R key to reset camera position
    if (input.isKeyJustPressed(KeyCode::R))
    {
        m_camera->setPosition(Vector3(6.0f, 4.0f, -6.0f));
        m_camera->lookAt(Vector3(0.0f, 0.0f, 0.0f));
        DX3DLogInfo("Camera reset to initial position");

        const auto& pos = m_camera->getPosition();
        const auto& forward = m_camera->getForward();
        printf("[Camera] Reset - Position: (%.2f, %.2f, %.2f), Forward: (%.2f, %.2f, %.2f)\n",
            pos.x, pos.y, pos.z, forward.x, forward.y, forward.z);
    }

    // ESC to exit
    if (input.isKeyPressed(KeyCode::Escape))
    {
        m_isRunning = false;
    }
}

void dx3d::Game::update()
{
    auto currentTime = std::chrono::steady_clock::now();
    m_deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_previousTime).count() / 1000000.0f;
    m_previousTime = currentTime;

    // Process input
    processInput(m_deltaTime);

    // Update camera
    m_camera->update();

    // Update all game objects
    for (size_t i = 0; i < m_gameObjects.size(); ++i)
    {
        m_gameObjects[i]->rotate(m_objectRotationDeltas[i] * m_deltaTime);
        m_gameObjects[i]->update(m_deltaTime);
    }

    // Update particle system
    ParticleSystem::getInstance().update(m_deltaTime);

    // Optional: Update emitter position to follow camera
    if (auto snowEmitter = ParticleSystem::getInstance().getEmitter("snow"))
    {
        Vector3 emitterPos = m_camera->getPosition();
        emitterPos.y += 10.0f;  // Place emitter above camera
        snowEmitter->setPosition(emitterPos);
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
        DirectX::XMMATRIX view = m_camera->getViewMatrix().toXMMatrix();  // Use camera's view matrix
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

    // Render particles (after solid objects, before UI)
    // Enable alpha blending for particles
    ID3D11BlendState* blendState = nullptr;
    ID3D11BlendState* previousBlendState = nullptr;
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    UINT sampleMask = 0;

    // Create and set blend state for particles
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    auto device = d3dContext->GetDevice();
    device->CreateBlendState(&blendDesc, &blendState);

    // Save current blend state and set particle blend state
    d3dContext->OMGetBlendState(&previousBlendState, blendFactor, &sampleMask);
    d3dContext->OMSetBlendState(blendState, blendFactor, 0xffffffff);

    // Render particles
    ParticleSystem::getInstance().render(deviceContext, *m_camera, m_projectionMatrix);

    // Restore previous blend state
    d3dContext->OMSetBlendState(previousBlendState, blendFactor, sampleMask);

    // Clean up
    if (blendState) blendState->Release();
    if (previousBlendState) previousBlendState->Release();

    deviceContext.present(swapChain);

    // Update input system at the end of frame
    Input::getInstance().update();
}