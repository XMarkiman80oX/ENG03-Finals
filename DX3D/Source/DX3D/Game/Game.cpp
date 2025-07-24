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
#include <DX3D/Graphics/RenderTexture.h>
#include <DX3D/Graphics/Primitives/AGameObject.h>
#include <DX3D/Graphics/Primitives/Cube.h>
#include <DX3D/Graphics/Primitives/Plane.h>
#include <DX3D/Graphics/Primitives/CameraObject.h>
#include <DX3D/Graphics/Shaders/Rainbow3DShader.h>
#include <DX3D/Graphics/Shaders/WhiteShader.h>
#include <DX3D/Graphics/Shaders/FogShader.h>
#include <DX3D/Graphics/Shaders/ModelShader.h>
#include <DX3D/Graphics/Shaders/ModelVertexShader.h>
#include <DX3D/Math/Math.h>
#include <DX3D/Game/ViewportManager.h>
#include <DX3D/Game/SelectionSystem.h>

#include <DX3D/ECS/ComponentManager.h>
#include <DX3D/ECS/Components/TransformComponent.h>
#include <DX3D/ECS/Components/PhysicsComponent.h>
#include <DX3D/Physics/PhysicsSystem.h>

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

    DX3DLogInfo("Game initialized with ECS and Physics systems.");
}

dx3d::Game::~Game()
{
    DX3DLogInfo("Game deallocation started.");

    // Shutdown physics system
    PhysicsSystem::getInstance().shutdown();

    if (m_particleDepthState) m_particleDepthState->Release();
    if (m_solidDepthState) m_solidDepthState->Release();
}

void dx3d::Game::createRenderingResources()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto resourceDesc = renderSystem.getGraphicsResourceDesc();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto d3dContext = deviceContext.getDeviceContext();
    ID3D11Device* device = nullptr;
    d3dContext->GetDevice(&device);

    // =========================================================================
    // INITIALIZE ECS AND PHYSICS SYSTEMS
    // =========================================================================

    // Register ECS components
    auto& componentManager = ComponentManager::getInstance();
    componentManager.registerComponent<TransformComponent>();
    componentManager.registerComponent<PhysicsComponent>();

    // Initialize physics system
    PhysicsSystem::getInstance().initialize();

    DX3DLogInfo("ECS and Physics systems initialized successfully.");

    // =========================================================================
    // CREATE RENDERING RESOURCES
    // =========================================================================

    // Create vertex/index buffers for primitives
    m_cubeVertexBuffer = Cube::CreateVertexBuffer(resourceDesc);
    m_cubeIndexBuffer = Cube::CreateIndexBuffer(resourceDesc);
    m_planeVertexBuffer = Plane::CreateVertexBuffer(resourceDesc);
    m_planeIndexBuffer = Plane::CreateIndexBuffer(resourceDesc);

    // Create shaders
    m_modelVertexShader = createModelVertexShader(resourceDesc);
    m_modelPixelShader = std::make_shared<PixelShader>(resourceDesc, ModelShader::GetPixelShaderCode());
    m_rainbowVertexShader = std::make_shared<VertexShader>(resourceDesc, Rainbow3DShader::GetVertexShaderCode());
    m_rainbowPixelShader = std::make_shared<PixelShader>(resourceDesc, Rainbow3DShader::GetPixelShaderCode());
    m_whiteVertexShader = std::make_shared<VertexShader>(resourceDesc, WhiteShader::GetVertexShaderCode());
    m_whitePixelShader = std::make_shared<PixelShader>(resourceDesc, WhiteShader::GetPixelShaderCode());
    m_fogVertexShader = std::make_shared<VertexShader>(resourceDesc, FogShader::GetVertexShaderCode());
    m_fogPixelShader = std::make_shared<PixelShader>(resourceDesc, FogShader::GetPixelShaderCode());

    // Create constant buffers
    m_fogConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(FogShaderConstants), resourceDesc);
    m_materialConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(FogMaterialConstants), resourceDesc);
    m_modelMaterialConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(ModelMaterialConstants), resourceDesc);
    m_transformConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(TransformationMatrices), resourceDesc);

    // Create depth buffer
    const auto& windowSize = m_display->getSize();
    m_depthBuffer = std::make_shared<DepthBuffer>(
        windowSize.width,
        windowSize.height,
        resourceDesc
    );

    // Create depth states
    D3D11_DEPTH_STENCIL_DESC solidDesc = {};
    solidDesc.DepthEnable = TRUE;
    solidDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    solidDesc.DepthFunc = D3D11_COMPARISON_LESS;
    solidDesc.StencilEnable = FALSE;
    device->CreateDepthStencilState(&solidDesc, &m_solidDepthState);

    D3D11_DEPTH_STENCIL_DESC particleDesc = {};
    particleDesc.DepthEnable = TRUE;
    particleDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    particleDesc.DepthFunc = D3D11_COMPARISON_LESS;
    particleDesc.StencilEnable = FALSE;
    device->CreateDepthStencilState(&particleDesc, &m_particleDepthState);

    // =========================================================================
    // CREATE PHYSICS DEMO SCENE
    // =========================================================================

    // Clear game objects
    m_gameObjects.clear();
    m_gameObjects.reserve(100); // Reserve space for ground plane + cubes + camera

    // Create ground plane (static physics body)
    auto groundPlane = std::make_shared<Plane>(
        Vector3(0.0f, 0.0f, 0.0f),      // Position slightly below origin
        Vector3(0.0f, 90.0f, 0.0f),   // 90 degrees rotation to make it horizontal
        Vector3(50.0f, 1.0f, 50.0f)      // Large scale for ground
    );
    groundPlane->enablePhysics(PhysicsBodyType::Static);
    groundPlane->setPhysicsRestitution(0.0f);  // Some bounciness
    groundPlane->setPhysicsFriction(0.7f);     // Good friction to stop sliding
    m_gameObjects.push_back(groundPlane);

    DX3DLogInfo("Created ground plane with static physics");

    spawnCubeDemo();

    DX3DLogInfo(("Created " + std::to_string(15) + " physics-enabled cubes").c_str());

    // Add game camera
    m_gameCamera = std::make_shared<CameraObject>(
        Vector3(15.0f, 10.0f, -15.0f),
        Vector3(0.0f, 0.0f, 0.0f)
    );
    m_gameCamera->getCamera().lookAt(Vector3(0.0f, 2.0f, 0.0f));
    m_gameObjects.push_back(m_gameCamera);

    // Set up scene camera
    m_sceneCamera = std::make_unique<Camera>(
        Vector3(20.0f, 15.0f, -20.0f),
        Vector3(0.0f, 2.0f, 0.0f)
    );

    // Create projection matrix
    float aspectRatio = static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height);
    m_projectionMatrix = Matrix4x4::CreatePerspectiveFovLH(
        1.0472f,
        aspectRatio,
        0.1f,
        100.0f
    );

    // Initialize viewport manager and selection system
    m_viewportManager = std::make_unique<ViewportManager>();
    m_viewportManager->initialize(*m_graphicsEngine, 640, 480);
    m_selectionSystem = std::make_unique<SelectionSystem>();

    // Initialize fog settings
    m_fogDesc.enabled = false;  // Disable fog for physics demo
    m_fogDesc.color = Vector4(0.2f, 0.3f, 0.4f, 1.0f);
    m_fogDesc.start = 10.0f;
    m_fogDesc.end = 50.0f;

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    HWND hwnd = m_display->getWindowHandle();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, d3dContext);

    device->Release();

    DX3DLogInfo("Physics demo scene creation complete!");
}

void dx3d::Game::processInput(float deltaTime)
{
    auto& input = Input::getInstance();
    auto& sceneViewport = m_viewportManager->getViewport(ViewportType::Scene);

    // Scene camera controls (when right mouse button is held and scene viewport is focused)
    if (sceneViewport.isFocused && input.isMouseButtonPressed(MouseButton::Right))
    {
        float moveSpeed = m_cameraSpeed * deltaTime;
        if (input.isKeyPressed(KeyCode::W)) m_sceneCamera->moveForward(moveSpeed);
        if (input.isKeyPressed(KeyCode::S)) m_sceneCamera->moveBackward(moveSpeed);
        if (input.isKeyPressed(KeyCode::A)) m_sceneCamera->moveLeft(moveSpeed);
        if (input.isKeyPressed(KeyCode::D)) m_sceneCamera->moveRight(moveSpeed);
        if (input.isKeyPressed(KeyCode::Q)) m_sceneCamera->moveDown(moveSpeed);
        if (input.isKeyPressed(KeyCode::E)) m_sceneCamera->moveUp(moveSpeed);

        float mouseDeltaX = static_cast<float>(input.getMouseDeltaX());
        float mouseDeltaY = static_cast<float>(input.getMouseDeltaY());

        if (mouseDeltaX != 0.0f || mouseDeltaY != 0.0f)
        {
            m_sceneCamera->onMouseMove(mouseDeltaX, mouseDeltaY, m_mouseSensitivity * 0.01f);
        }
    }

    // Object selection in scene viewport
    if (sceneViewport.isHovered && input.isMouseButtonJustPressed(MouseButton::Left))
    {
        auto picked = m_selectionSystem->pickObject(
            m_gameObjects,
            *m_sceneCamera,
            sceneViewport.mousePos.x,
            sceneViewport.mousePos.y,
            sceneViewport.width,
            sceneViewport.height
        );
        m_selectionSystem->setSelectedObject(picked);
    }

    // Physics interaction controls
    if (input.isKeyJustPressed(KeyCode::Space))
    {
        // Add a new cube at a random position
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> posX(-5.0f, 5.0f);
        std::uniform_real_distribution<float> posZ(-5.0f, 5.0f);
        std::uniform_real_distribution<float> scale(0.5f, 2.0f);

        Vector3 position(posX(gen), 15.0f, posZ(gen));
        Vector3 cubeScale(scale(gen), scale(gen), scale(gen));

        auto cube = std::make_shared<Cube>(position, Vector3(0, 0, 0), cubeScale);
        cube->enablePhysics(PhysicsBodyType::Dynamic);
        cube->setPhysicsMass(1.0f);
        cube->setPhysicsRestitution(0.5f);
        cube->setPhysicsFriction(0.5f);

        m_gameObjects.push_back(cube);
        DX3DLogInfo("Added new physics cube!");
    }

    // Apply impulse to selected object
    if (input.isKeyJustPressed(KeyCode::F))
    {
        auto selectedObject = m_selectionSystem->getSelectedObject();
        if (selectedObject && selectedObject->hasPhysics())
        {
            selectedObject->applyImpulse(Vector3(0.0f, 10.0f, 0.0f)); // Upward impulse
            DX3DLogInfo("Applied upward impulse to selected object!");
        }
    }

    // Delete selected object with Delete key
    if (input.isKeyJustPressed(KeyCode::Delete))
    {
        auto selectedObject = m_selectionSystem->getSelectedObject();
        if (selectedObject)
        {
            auto it = std::find(m_gameObjects.begin(), m_gameObjects.end(), selectedObject);
            if (it != m_gameObjects.end())
            {
                m_gameObjects.erase(it);
                m_selectionSystem->setSelectedObject(nullptr);
                DX3DLogInfo("Deleted selected object!");
            }
        }
    }

    // Reset scene camera
    if (input.isKeyJustPressed(KeyCode::R))
    {
        m_sceneCamera->setPosition(Vector3(20.0f, 15.0f, -20.0f));
        m_sceneCamera->lookAt(Vector3(0.0f, 2.0f, 0.0f));
    }

    // Exit application
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

    // Initialize ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Process input
    processInput(m_deltaTime);

    // Update camera
    m_sceneCamera->update();

    // UPDATE PHYSICS SYSTEM (This is the key!)
    PhysicsSystem::getInstance().update(m_deltaTime);

    // Update game objects (transforms are now automatically synced from physics)
    for (auto& gameObject : m_gameObjects)
    {
        gameObject->update(m_deltaTime);
    }

    // Debug output (only occasionally)
    static float debugTimer = 0.0f;
    debugTimer += m_deltaTime;
    if (debugTimer >= 5.0f) // Print debug info every 5 seconds
    {
        DX3DLogInfo(("Physics demo running - " + std::to_string(m_gameObjects.size()) + " objects").c_str());
        debugTimer = 0.0f;
    }
}

void dx3d::Game::renderScene(Camera& camera, const Matrix4x4& projMatrix, RenderTexture* renderTarget)
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto d3dContext = deviceContext.getDeviceContext();

    // Set up render target
    if (renderTarget)
    {
        renderTarget->clear(deviceContext, 0.1f, 0.1f, 0.2f, 1.0f); // Dark blue background
        renderTarget->setAsRenderTarget(deviceContext);
    }
    else
    {
        auto& swapChain = m_display->getSwapChain();
        deviceContext.clearRenderTargetColor(swapChain, 0.1f, 0.1f, 0.2f, 1.0f);
        deviceContext.clearDepthBuffer(*m_depthBuffer);
        deviceContext.setRenderTargetsWithDepth(swapChain, *m_depthBuffer);
    }

    ui32 viewportWidth = renderTarget ? 640 : m_display->getSize().width;
    ui32 viewportHeight = renderTarget ? 480 : m_display->getSize().height;
    deviceContext.setViewportSize(viewportWidth, viewportHeight);

    // Set up constant buffers
    ID3D11Buffer* transformCb = m_transformConstantBuffer->getBuffer();
    d3dContext->VSSetConstantBuffers(0, 1, &transformCb);
    d3dContext->OMSetDepthStencilState(m_solidDepthState, 0);

    bool isSceneView = (&camera == m_sceneCamera.get());

    // Render all game objects
    for (const auto& gameObject : m_gameObjects)
    {
        // Skip camera in scene view
        bool isCamera = std::dynamic_pointer_cast<CameraObject>(gameObject) != nullptr;
        if (!isSceneView && isCamera)
            continue;

        // Use fog shader for all objects
        deviceContext.setVertexShader(m_fogVertexShader->getShader());
        deviceContext.setPixelShader(m_fogPixelShader->getShader());
        deviceContext.setInputLayout(m_fogVertexShader->getInputLayout());

        // Set up fog constants
        FogShaderConstants fsc = {};
        fsc.fogColor = m_fogDesc.color;
        fsc.cameraPosition = camera.getPosition();
        fsc.fogStart = m_fogDesc.start;
        fsc.fogEnd = m_fogDesc.end;
        fsc.fogEnabled = m_fogDesc.enabled;
        m_fogConstantBuffer->update(deviceContext, &fsc);

        ID3D11Buffer* fogCb = m_fogConstantBuffer->getBuffer();
        d3dContext->PSSetConstantBuffers(1, 1, &fogCb);

        // Set up material constants
        FogMaterialConstants fmc = {};
        fmc.useVertexColor = true;
        fmc.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        m_materialConstantBuffer->update(deviceContext, &fmc);
        ID3D11Buffer* materialCb = m_materialConstantBuffer->getBuffer();
        d3dContext->PSSetConstantBuffers(2, 1, &materialCb);

        // Set vertex and index buffers based on object type
        bool bufferSet = false;
        ui32 indexCount = 0;

        if (auto cube = std::dynamic_pointer_cast<Cube>(gameObject))
        {
            deviceContext.setVertexBuffer(*m_cubeVertexBuffer);
            deviceContext.setIndexBuffer(*m_cubeIndexBuffer);
            indexCount = Cube::GetIndexCount();
            bufferSet = true;
        }
        else if (auto plane = std::dynamic_pointer_cast<Plane>(gameObject))
        {
            deviceContext.setVertexBuffer(*m_planeVertexBuffer);
            deviceContext.setIndexBuffer(*m_planeIndexBuffer);
            indexCount = Plane::GetIndexCount();
            bufferSet = true;
        }

        if (bufferSet)
        {
            // Set up transformation matrices
            TransformationMatrices transformMatrices;
            transformMatrices.world = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(gameObject->getWorldMatrix().toXMMatrix()));
            transformMatrices.view = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(camera.getViewMatrix().toXMMatrix()));
            transformMatrices.projection = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(projMatrix.toXMMatrix()));
            m_transformConstantBuffer->update(deviceContext, &transformMatrices);

            // Draw the object
            deviceContext.drawIndexed(indexCount, 0, 0);
        }
    }
}

void dx3d::Game::renderUI()
{
    
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("GameObjects"))
        {
            if (ImGui::MenuItem("Cube Demo"))
            {
                spawnCubeDemo(); // Spawn 15 new cubes
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;
    float halfWidth = windowWidth * 0.5f;
    float halfHeight = windowHeight * 0.5f;

    // Game View - Top Left
    ImGui::SetNextWindowPos(ImVec2(0, 20)); // Y offset for the menu bar
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight - 20));
    ImGui::Begin("Game View", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImVec2 gameViewportSize = ImGui::GetContentRegionAvail();
    if (gameViewportSize.x > 0 && gameViewportSize.y > 0)
    {
        m_viewportManager->resize(ViewportType::Game, static_cast<ui32>(gameViewportSize.x), static_cast<ui32>(gameViewportSize.y));
        auto& gameViewport = m_viewportManager->getViewport(ViewportType::Game);
        ImGui::Image((void*)gameViewport.renderTexture->getShaderResourceView(), gameViewportSize);
    }
    ImGui::End();

    // Scene View - Bottom Left
    ImGui::SetNextWindowPos(ImVec2(0, halfHeight));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight));
    ImGui::Begin("Scene View", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImVec2 sceneViewportSize = ImGui::GetContentRegionAvail();
    if (sceneViewportSize.x > 0 && sceneViewportSize.y > 0)
    {
        m_viewportManager->resize(ViewportType::Scene, static_cast<ui32>(sceneViewportSize.x), static_cast<ui32>(sceneViewportSize.y));
        auto& sceneViewport = m_viewportManager->getViewport(ViewportType::Scene);
        ImGui::Image((void*)sceneViewport.renderTexture->getShaderResourceView(), sceneViewportSize);

        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 windowPos = ImGui::GetWindowPos();
        float localX = mousePos.x - windowPos.x - 8;
        float localY = mousePos.y - windowPos.y - ImGui::GetFrameHeight() - 4;

        m_viewportManager->updateViewportStates(ViewportType::Scene, ImGui::IsWindowHovered(), ImGui::IsWindowFocused(), localX, localY);
    }
    ImGui::End();

    // Physics Controls & Scene Outliner - Top Right
    ImGui::SetNextWindowPos(ImVec2(halfWidth, 20)); // Y offset for the menu bar
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight - 20));
    ImGui::Begin("Physics Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Physics Demo Controls");
    ImGui::Separator();

    ImGui::Text("Objects: %zu", m_gameObjects.size());
    ImGui::Text("Delta Time: %.3f ms", m_deltaTime * 1000.0f);
    ImGui::Text("FPS: %.1f", 1.0f / m_deltaTime);

    
    ImGui::Separator();
    ImGui::Text("Scene Outliner");
    ImGui::BeginChild("Outliner", ImVec2(0, 0), true); // Scrollable region

    int objectId = 0;
    for (const auto& gameObject : m_gameObjects)
    {
        std::string objectName = "Object";
        if (std::dynamic_pointer_cast<Cube>(gameObject)) objectName = "Cube";
        else if (std::dynamic_pointer_cast<Plane>(gameObject)) objectName = "Plane";
        else if (std::dynamic_pointer_cast<CameraObject>(gameObject)) objectName = "Game Camera";

        std::string label = objectName + " " + std::to_string(objectId++);

        bool isSelected = (gameObject == m_selectionSystem->getSelectedObject());
        if (ImGui::Selectable(label.c_str(), isSelected))
        {
            m_selectionSystem->setSelectedObject(gameObject);
        }
    }
    ImGui::EndChild();

    ImGui::End();

    // Inspector - Bottom Right
    ImGui::SetNextWindowPos(ImVec2(halfWidth, halfHeight));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight));
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    auto selectedObject = m_selectionSystem->getSelectedObject();
    if (selectedObject)
    {
        ImGui::Text("Selected Object");
        ImGui::Separator();

        Vector3 pos = selectedObject->getPosition();
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);

        if (selectedObject->hasPhysics())
        {
            Vector3 vel = selectedObject->getLinearVelocity();
            ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", vel.x, vel.y, vel.z);
            ImGui::Text("Speed: %.2f", std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z));
        }
    }
    else
    {
        ImGui::Text("No object selected");
        ImGui::Text("Click on an object in the Scene View or Outliner to select it");
    }

    ImGui::End();
}

void dx3d::Game::render()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto& swapChain = m_display->getSwapChain();

    // Render to viewports
    auto& sceneViewport = m_viewportManager->getViewport(ViewportType::Scene);
    auto& gameViewport = m_viewportManager->getViewport(ViewportType::Game);

    float aspectRatio = static_cast<float>(sceneViewport.width) / static_cast<float>(sceneViewport.height);
    Matrix4x4 sceneProjMatrix = Matrix4x4::CreatePerspectiveFovLH(1.0472f, aspectRatio, 0.1f, 100.0f);
    renderScene(*m_sceneCamera, sceneProjMatrix, sceneViewport.renderTexture.get());

    aspectRatio = static_cast<float>(gameViewport.width) / static_cast<float>(gameViewport.height);
    Matrix4x4 gameProjMatrix = m_gameCamera->getProjectionMatrix(aspectRatio);
    renderScene(m_gameCamera->getCamera(), gameProjMatrix, gameViewport.renderTexture.get());

    // Render UI
    deviceContext.clearRenderTargetColor(swapChain, 0.1f, 0.1f, 0.1f, 1.0f);
    deviceContext.clearDepthBuffer(*m_depthBuffer);
    deviceContext.setRenderTargetsWithDepth(swapChain, *m_depthBuffer);
    deviceContext.setViewportSize(m_display->getSize().width, m_display->getSize().height);

    renderUI();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    deviceContext.present(swapChain);
}

void dx3d::Game::spawnCubeDemo()
{
    // Create falling cubes with physics
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posX(1.0f, 5.0f);
    std::uniform_real_distribution<float> posZ(1.0f, 5.0f);
    std::uniform_real_distribution<float> posY(13.0f, 15.0f);

    const int numCubes = 25;
    for (int i = 0; i < numCubes; ++i)
    {
        Vector3 position(posX(gen), posY(gen), posZ(gen));
        Vector3 cubeScale(2.0f, 2.0f, 2.0f);

        auto cube = std::make_shared<Cube>(position, Vector3(0, 0, 0), cubeScale);

        // Enable physics with random properties
        cube->enablePhysics(PhysicsBodyType::Dynamic);
        cube->setPhysicsMass(2.0f);
        cube->setPhysicsRestitution(0.8f);
        cube->setPhysicsFriction(0.8f);

        m_gameObjects.push_back(cube);
    }

    DX3DLogInfo(("Spawned a new Cube Demo with " + std::to_string(numCubes) + " cubes").c_str());
}

void dx3d::Game::run()
{
    MSG msg{};
    while (m_isRunning)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                m_isRunning = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!m_isRunning) break;

        update();
        render();
        Input::getInstance().update();
    }
}