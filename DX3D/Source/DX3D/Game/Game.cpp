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
#include <DX3D/Graphics/Primitives/Cube.h>
#include <DX3D/Graphics/Shaders/GradientCubeShader.h> // Include the new shader
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

    DX3DLogInfo("Game initialized with 100 rotating cubes.");
}

dx3d::Game::~Game()
{
    DX3DLogInfo("Game deallocation started.");
}

void dx3d::Game::createRenderingResources()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto resourceDesc = renderSystem.getGraphicsResourceDesc();

    m_cubeVertexBuffer = Cube::CreateVertexBuffer(resourceDesc);
    m_cubeIndexBuffer = Cube::CreateIndexBuffer(resourceDesc);

    // Use the new GradientCubeShader instead of the old one
    m_transform3DVertexShader = std::make_shared<VertexShader>(resourceDesc, GradientCubeShader::GetVertexShaderCode());
    m_transform3DPixelShader = std::make_shared<PixelShader>(resourceDesc, GradientCubeShader::GetPixelShaderCode());

    m_transformConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(TransformationMatrices), resourceDesc);

    m_cubes.reserve(100);
    m_cubeRotationDeltas.reserve(100);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> pos_dis(-5.0, 5.0);
    std::uniform_real_distribution<> rot_dis(-0.8f, 0.8f);
    Vector3 cubeScale(2.0f, 2.0f, 2.0f);

    for (int i = 0; i < 100; ++i)
    {
        m_cubes.push_back(std::make_shared<Cube>(
            Vector3(static_cast<float>(pos_dis(gen)), static_cast<float>(pos_dis(gen)), static_cast<float>(pos_dis(gen))),
            Vector3(0, 0, 0),
            cubeScale
        ));
        m_cubeRotationDeltas.push_back(Vector3(
            static_cast<float>(rot_dis(gen)),
            static_cast<float>(rot_dis(gen)),
            static_cast<float>(rot_dis(gen))
        ));
    }

    // Setup View and Projection Matrices
    const auto& windowSize = m_display->getSize();
    float aspectRatio = static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height);

    m_viewMatrix = Matrix4x4::CreateLookAtLH(
        Vector3(0.0f, 0.0f, -12.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(0.0f, 1.0f, 0.0f)
    );

    m_projectionMatrix = Matrix4x4::CreatePerspectiveFovLH(
        1.5708f,
        aspectRatio,
        0.1f,
        100.0f
    );

    DX3DLogInfo("100 rotating cube resources created successfully.");
}


void dx3d::Game::update()
{
    auto currentTime = std::chrono::steady_clock::now();
    m_deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_previousTime).count() / 1000000.0f;
    m_previousTime = currentTime;

    for (size_t i = 0; i < m_cubes.size(); ++i)
    {
        m_cubes[i]->rotate(m_cubeRotationDeltas[i] * m_deltaTime);
    }
}


void dx3d::Game::render()
{
    update();

    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto& swapChain = m_display->getSwapChain();

    deviceContext.clearRenderTargetColor(swapChain, 0.0f, 0.4f, 0.4f, 1.0f);
    deviceContext.setRenderTargets(swapChain);
    deviceContext.setViewportSize(m_display->getSize().width, m_display->getSize().height);

    deviceContext.setVertexBuffer(*m_cubeVertexBuffer);
    deviceContext.setIndexBuffer(*m_cubeIndexBuffer);

    // Make sure to use the new gradient shader
    deviceContext.setVertexShader(m_transform3DVertexShader->getShader());
    deviceContext.setPixelShader(m_transform3DPixelShader->getShader());
    deviceContext.setInputLayout(m_transform3DVertexShader->getInputLayout());

    ID3D11DeviceContext* d3dContext = deviceContext.getDeviceContext();
    ID3D11Buffer* cb = m_transformConstantBuffer->getBuffer();
    d3dContext->VSSetConstantBuffers(0, 1, &cb);

    for (const auto& cube : m_cubes)
    {
        TransformationMatrices transformMatrices;

        DirectX::XMMATRIX world = cube->getWorldMatrix().toXMMatrix();
        DirectX::XMMATRIX view = m_viewMatrix.toXMMatrix();
        DirectX::XMMATRIX projection = m_projectionMatrix.toXMMatrix();

        transformMatrices.world = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(world));
        transformMatrices.view = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(view));
        transformMatrices.projection = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(projection));

        m_transformConstantBuffer->update(deviceContext, &transformMatrices);

        deviceContext.drawIndexed(m_cubeIndexBuffer->getIndexCount(), 0, 0);
    }

    deviceContext.present(swapChain);
}