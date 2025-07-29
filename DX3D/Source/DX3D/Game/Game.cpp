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

#include <DX3D/Graphics/Primitives/CameraObject.h>
#include <DX3D/Graphics/Shaders/Rainbow3DShader.h>
#include <DX3D/Graphics/Shaders/WhiteShader.h>
#include <DX3D/Graphics/Shaders/FogShader.h>
#include <DX3D/Graphics/Shaders/ModelShader.h>
#include <DX3D/Graphics/Shaders/ModelVertexShader.h>
#include <DX3D/Math/Math.h>
#include <DX3D/Game/ViewportManager.h>
#include <DX3D/Game/SelectionSystem.h>
#include <DX3D/Scene/SceneStateManager.h>
#include <DX3D/Game/FPSCameraController.h>
#include <DX3D/Game/UndoRedoSystem.h>

#include <DX3D/Graphics/Primitives/AGameObject.h>
#include <DX3D/Graphics/Primitives/Cube.h>
#include <DX3D/Graphics/Primitives/Plane.h>
#include <DX3D/Graphics/Primitives/Sphere.h>
#include <DX3D/Graphics/Primitives/Capsule.h>
#include <DX3D/Graphics/Primitives/Cylinder.h>
#include <DX3D/Graphics/Primitives/Model.h>
#include <DX3D/Assets/ModelLoader.h>

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

    m_sceneStateManager = std::make_unique<SceneStateManager>();
    m_fpsController = std::make_unique<FPSCameraController>();

    m_undoRedoSystem = std::make_unique<UndoRedoSystem>(10);

    createRenderingResources();

    m_fpsController->setCamera(m_sceneCamera.get());
    m_sceneStateManager->addStateChangeCallback([this](SceneState oldState, SceneState newState) {
        onSceneStateChanged(oldState, newState);
        });

    DX3DLogInfo("Game initialized with ECS, Physics, and Scene State systems.");
}

dx3d::Game::~Game()
{
    DX3DLogInfo("Game deallocation started.");

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

    auto& componentManager = ComponentManager::getInstance();
    componentManager.registerComponent<TransformComponent>();
    componentManager.registerComponent<PhysicsComponent>();

    PhysicsSystem::getInstance().initialize();

    DX3DLogInfo("ECS and Physics systems initialized successfully.");

    m_cubeVertexBuffer = Cube::CreateVertexBuffer(resourceDesc);
    m_cubeIndexBuffer = Cube::CreateIndexBuffer(resourceDesc);
    m_planeVertexBuffer = Plane::CreateVertexBuffer(resourceDesc);
    m_planeIndexBuffer = Plane::CreateIndexBuffer(resourceDesc);
    m_sphereVertexBuffer = Sphere::CreateVertexBuffer(resourceDesc);
    m_sphereIndexBuffer = Sphere::CreateIndexBuffer(resourceDesc);
    m_cylinderVertexBuffer = Cylinder::CreateVertexBuffer(resourceDesc);
    m_cylinderIndexBuffer = Cylinder::CreateIndexBuffer(resourceDesc);
    m_capsuleVertexBuffer = Capsule::CreateVertexBuffer(resourceDesc);
    m_capsuleIndexBuffer = Capsule::CreateIndexBuffer(resourceDesc);

    m_modelVertexShader = createModelVertexShader(resourceDesc);
    m_modelPixelShader = std::make_shared<PixelShader>(resourceDesc, ModelShader::GetPixelShaderCode());
    m_rainbowVertexShader = std::make_shared<VertexShader>(resourceDesc, Rainbow3DShader::GetVertexShaderCode());
    m_rainbowPixelShader = std::make_shared<PixelShader>(resourceDesc, Rainbow3DShader::GetPixelShaderCode());
    m_whiteVertexShader = std::make_shared<VertexShader>(resourceDesc, WhiteShader::GetVertexShaderCode());
    m_whitePixelShader = std::make_shared<PixelShader>(resourceDesc, WhiteShader::GetPixelShaderCode());
    m_fogVertexShader = std::make_shared<VertexShader>(resourceDesc, FogShader::GetVertexShaderCode());
    m_fogPixelShader = std::make_shared<PixelShader>(resourceDesc, FogShader::GetPixelShaderCode());

    m_fogConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(FogShaderConstants), resourceDesc);
    m_materialConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(FogMaterialConstants), resourceDesc);
    m_modelMaterialConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(ModelMaterialConstants), resourceDesc);
    m_transformConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(TransformationMatrices), resourceDesc);

    const auto& windowSize = m_display->getSize();
    m_depthBuffer = std::make_shared<DepthBuffer>(
        windowSize.width,
        windowSize.height,
        resourceDesc
    );

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

    m_gameObjects.clear();
    m_gameObjects.reserve(100);

    m_sceneCamera = std::make_unique<Camera>(
        Vector3(10.0f, 5.0f, -10.0f),
        Vector3(0.0f, 0.0f, 0.0f)
    );

    float aspectRatio = static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height);
    m_projectionMatrix = Matrix4x4::CreatePerspectiveFovLH(
        1.0472f,
        aspectRatio,
        0.1f,
        100.0f
    );

    m_viewportManager = std::make_unique<ViewportManager>();
    m_viewportManager->initialize(*m_graphicsEngine, 640, 480);
    m_selectionSystem = std::make_unique<SelectionSystem>();

    m_fogDesc.enabled = false;
    m_fogDesc.color = Vector4(0.2f, 0.3f, 0.4f, 1.0f);
    m_fogDesc.start = 10.0f;
    m_fogDesc.end = 50.0f;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    HWND hwnd = m_display->getWindowHandle();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, d3dContext);

    device->Release();

    m_gameCamera = std::make_shared<CameraObject>(
        Vector3(15.0f, 10.0f, -15.0f),
        Vector3(0.0f, 0.0f, 0.0f)
    );
    m_gameCamera->getCamera().lookAt(Vector3(0.0f, 2.0f, 0.0f));
    m_gameObjects.push_back(m_gameCamera);

    DX3DLogInfo("Empty scene initialized - use GameObjects menu to add objects!");
}

void dx3d::Game::processInput(float deltaTime)
{
    auto& input = Input::getInstance();

    // Undo/Redo shortcuts (only in edit mode)
    if (m_sceneStateManager->isEditMode())
    {
        if (input.isKeyJustPressedWithShiftCtrl(KeyCode::Z))
        {
            // Redo: Shift + Ctrl + Z
            if (m_undoRedoSystem->canRedo())
            {
                m_undoRedoSystem->redo();
                DX3DLogInfo("Redo action performed");
            }
        }
        else if (input.isKeyJustPressedWithCtrl(KeyCode::Z))
        {
            // Undo: Ctrl + Z
            if (m_undoRedoSystem->canUndo())
            {
                m_undoRedoSystem->undo();
                DX3DLogInfo("Undo action performed");
            }
        }

        // Delete selected object
        if (input.isKeyJustPressed(KeyCode::Delete))
        {
            auto selectedObject = m_selectionSystem->getSelectedObject();
            if (selectedObject)
            {
                // Create delete action and execute it through undo system
                auto deleteAction = std::make_unique<DeleteAction>(selectedObject, m_gameObjects);
                m_undoRedoSystem->executeAction(std::move(deleteAction));

                // Clear selection since object is deleted
                m_selectionSystem->setSelectedObject(nullptr);
                DX3DLogInfo("Deleted selected object");
            }
        }
    }

    if (input.isKeyJustPressed(KeyCode::F5))
    {
        if (m_sceneStateManager->isEditMode())
        {
            m_sceneStateManager->transitionToPlay();
        }
        else if (m_sceneStateManager->isPlayMode() || m_sceneStateManager->isPauseMode())
        {
            m_sceneStateManager->transitionToEdit();
        }
    }

    if (input.isKeyJustPressed(KeyCode::Space) && m_sceneStateManager->isPlayMode())
    {
        m_sceneStateManager->transitionToPause();
    }

    if (input.isKeyJustPressed(KeyCode::F10) && m_sceneStateManager->isPauseMode())
    {
        m_sceneStateManager->frameStep();
    }

    if (input.isKeyJustPressed(KeyCode::F5) && m_sceneStateManager->isPauseMode())
    {
        m_sceneStateManager->transitionToPlay();
    }

    auto& sceneViewport = m_viewportManager->getViewport(ViewportType::Scene);

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

    // Object selection only in edit mode
    if (m_sceneStateManager->isEditMode())
    {
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
    }


    // Create new cube (only in edit mode)
    if (input.isKeyJustPressed(KeyCode::Space) && m_sceneStateManager->isEditMode())
    {
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

        // Create through undo system
        auto createAction = std::make_unique<CreateAction>(cube, m_gameObjects);
        m_undoRedoSystem->executeAction(std::move(createAction));
        DX3DLogInfo("Added new physics cube!");
    }

    if (input.isKeyJustPressed(KeyCode::F))
    {
        auto selectedObject = m_selectionSystem->getSelectedObject();
        if (selectedObject && selectedObject->hasPhysics())
        {
            selectedObject->applyImpulse(Vector3(0.0f, 10.0f, 0.0f));
            DX3DLogInfo("Applied upward impulse to selected object!");
        }
    }

    if (input.isKeyJustPressed(KeyCode::R))
    {
        m_sceneCamera->setPosition(Vector3(20.0f, 15.0f, -20.0f));
        m_sceneCamera->lookAt(Vector3(0.0f, 2.0f, 0.0f));
    }

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

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_sceneStateManager->update(m_deltaTime);

    processInput(m_deltaTime);

    m_sceneCamera->update();

    if (m_sceneStateManager->isPlayMode())
    {
        m_fpsController->update(m_deltaTime);
    }

    updatePhysics(m_deltaTime);

    for (auto& gameObject : m_gameObjects)
    {
        gameObject->update(m_deltaTime);
    }

    static float debugTimer = 0.0f;
    debugTimer += m_deltaTime;
    if (debugTimer >= 5.0f)
    {
        DX3DLogInfo(("Physics demo running - " + std::to_string(m_gameObjects.size()) + " objects").c_str());
        debugTimer = 0.0f;
    }
}

void dx3d::Game::onSceneStateChanged(SceneState oldState, SceneState newState)
{
    switch (newState)
    {
    case SceneState::Edit:
        if (oldState == SceneState::Play || oldState == SceneState::Pause)
        {
            m_sceneStateManager->restoreObjectStates(m_gameObjects);
        }
        m_fpsController->disable();
        m_fpsController->lockCursor(false);
        m_physicsUpdateEnabled = false;
        ShowCursor(TRUE);
        break;

    case SceneState::Play:
        if (oldState == SceneState::Edit)
        {
            m_sceneStateManager->saveObjectStates(m_gameObjects);
        }
        m_fpsController->enable();
        m_fpsController->lockCursor(false);
        m_physicsUpdateEnabled = true;
        ShowCursor(TRUE);
        break;

    case SceneState::Pause:
        m_fpsController->lockCursor(false);
        m_physicsUpdateEnabled = false;
        ShowCursor(TRUE);
        break;
    }
}

void dx3d::Game::updatePhysics(float deltaTime)
{
    if (!m_physicsUpdateEnabled)
    {
        // Handle frame step in pause mode
        if (m_sceneStateManager->isPauseMode() && m_sceneStateManager->isFrameStepRequested())
        {
            PhysicsSystem::getInstance().update(1.0f / 60.0f);
            // Clear the frame step request after physics update
            m_sceneStateManager->clearFrameStepRequest();
        }
        return;
    }

    if (m_sceneStateManager->isPlayMode())
    {
        PhysicsSystem::getInstance().update(deltaTime);
    }
}

void dx3d::Game::renderScene(Camera& camera, const Matrix4x4& projMatrix, RenderTexture* renderTarget)
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto d3dContext = deviceContext.getDeviceContext();

    if (renderTarget)
    {
        renderTarget->clear(deviceContext, 0.1f, 0.1f, 0.2f, 1.0f);
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

    ID3D11Buffer* transformCb = m_transformConstantBuffer->getBuffer();
    d3dContext->VSSetConstantBuffers(0, 1, &transformCb);
    d3dContext->OMSetDepthStencilState(m_solidDepthState, 0);

    bool isSceneView = (&camera == m_sceneCamera.get());

    for (const auto& gameObject : m_gameObjects)
    {
        bool isCamera = std::dynamic_pointer_cast<CameraObject>(gameObject) != nullptr;
        if (!isSceneView && isCamera)
            continue;

        deviceContext.setVertexShader(m_fogVertexShader->getShader());
        deviceContext.setPixelShader(m_fogPixelShader->getShader());
        deviceContext.setInputLayout(m_fogVertexShader->getInputLayout());

        FogShaderConstants fsc = {};
        fsc.fogColor = m_fogDesc.color;
        fsc.cameraPosition = camera.getPosition();
        fsc.fogStart = m_fogDesc.start;
        fsc.fogEnd = m_fogDesc.end;
        fsc.fogEnabled = m_fogDesc.enabled;
        m_fogConstantBuffer->update(deviceContext, &fsc);

        ID3D11Buffer* fogCb = m_fogConstantBuffer->getBuffer();
        d3dContext->PSSetConstantBuffers(1, 1, &fogCb);

        FogMaterialConstants fmc = {};
        fmc.useVertexColor = true;
        fmc.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        m_materialConstantBuffer->update(deviceContext, &fmc);
        ID3D11Buffer* materialCb = m_materialConstantBuffer->getBuffer();
        d3dContext->PSSetConstantBuffers(2, 1, &materialCb);

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
        else if (auto sphere = std::dynamic_pointer_cast<Sphere>(gameObject))
        {
            deviceContext.setVertexBuffer(*m_sphereVertexBuffer);
            deviceContext.setIndexBuffer(*m_sphereIndexBuffer);
            indexCount = Sphere::GetIndexCount();
            bufferSet = true;
        }
        else if (auto cylinder = std::dynamic_pointer_cast<Cylinder>(gameObject))
        {
            deviceContext.setVertexBuffer(*m_cylinderVertexBuffer);
            deviceContext.setIndexBuffer(*m_cylinderIndexBuffer);
            indexCount = Cylinder::GetIndexCount();
            bufferSet = true;
        }
        else if (auto capsule = std::dynamic_pointer_cast<Capsule>(gameObject))
        {
            deviceContext.setVertexBuffer(*m_capsuleVertexBuffer);
            deviceContext.setIndexBuffer(*m_capsuleIndexBuffer);
            indexCount = Capsule::GetIndexCount();
            bufferSet = true;
        }

        if (bufferSet)
        {
            TransformationMatrices transformMatrices;
            transformMatrices.world = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(gameObject->getWorldMatrix().toXMMatrix()));
            transformMatrices.view = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(camera.getViewMatrix().toXMMatrix()));
            transformMatrices.projection = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(projMatrix.toXMMatrix()));
            m_transformConstantBuffer->update(deviceContext, &transformMatrices);

            deviceContext.drawIndexed(indexCount, 0, 0);
        }
    }
}

void dx3d::Game::renderUI()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Edit"))
        {
            bool canUndo = m_undoRedoSystem->canUndo();
            bool canRedo = m_undoRedoSystem->canRedo();
            bool isEditMode = m_sceneStateManager->isEditMode();

            if (!canUndo || !isEditMode)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
            }

            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo && isEditMode))
            {
                m_undoRedoSystem->undo();
                DX3DLogInfo("Undo action performed");
            }

            if (!canUndo || !isEditMode)
            {
                ImGui::PopStyleVar();
            }

            if (canUndo && isEditMode)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)", m_undoRedoSystem->getUndoDescription().c_str());
            }

            if (!canRedo || !isEditMode)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
            }

            if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, canRedo && isEditMode))
            {
                m_undoRedoSystem->redo();
                DX3DLogInfo("Redo action performed");
            }

            if (!canRedo || !isEditMode)
            {
                ImGui::PopStyleVar();
            }

            if (canRedo && isEditMode)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)", m_undoRedoSystem->getRedoDescription().c_str());
            }

            ImGui::Separator();

            auto selectedObject = m_selectionSystem->getSelectedObject();
            bool hasSelection = selectedObject != nullptr;

            if (!hasSelection || !isEditMode)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
            }

            if (ImGui::MenuItem("Delete", "Delete", false, hasSelection && isEditMode))
            {
                if (selectedObject)
                {
                    auto deleteAction = std::make_unique<DeleteAction>(selectedObject, m_gameObjects);
                    m_undoRedoSystem->executeAction(std::move(deleteAction));
                    m_selectionSystem->setSelectedObject(nullptr);
                    DX3DLogInfo("Deleted selected object");
                }
            }

            if (!hasSelection || !isEditMode)
            {
                ImGui::PopStyleVar();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObjects"))
        {
            bool isEditMode = m_sceneStateManager->isEditMode();

            if (ImGui::BeginMenu("Primitives"))
            {
                if (ImGui::MenuItem("Cube", nullptr, false, isEditMode))
                {
                    spawnCube();
                }

                if (ImGui::MenuItem("Sphere", nullptr, false, isEditMode))
                {
                    spawnSphere();
                }

                if (ImGui::MenuItem("Capsule", nullptr, false, isEditMode))
                {
                    spawnCapsule();
                }

                if (ImGui::MenuItem("Cylinder", nullptr, false, isEditMode))
                {
                    spawnCylinder();
                }

                if (ImGui::MenuItem("Plane", nullptr, false, isEditMode))
                {
                    spawnPlane();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Models"))
            {
                if (ImGui::MenuItem("Bunny", nullptr, false, isEditMode))
                {
                    spawnModel("bunny.obj");
                }

                if (ImGui::MenuItem("Armadillo", nullptr, false, isEditMode))
                {
                    spawnModel("armadillo.obj");
                }

                if (ImGui::MenuItem("Teapot", nullptr, false, isEditMode))
                {
                    spawnModel("teapot.obj");
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Cube Demo"))
            {
                spawnCubeDemo();
            }

            ImGui::EndMenu();
        }

        if (m_sceneStateManager->isEditMode())
        {
            ImGui::SameLine(ImGui::GetWindowWidth() - 200);
            ImGui::Text("Undo: %d | Redo: %d",
                m_undoRedoSystem->getUndoCount(),
                m_undoRedoSystem->getRedoCount());
        }

        ImGui::EndMainMenuBar();
    }

    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;
    float halfWidth = windowWidth * 0.5f;
    float halfHeight = windowHeight * 0.5f;
    float inspectorHeight = halfHeight * 0.6f;
    float debugHeight = halfHeight * 0.4f;

    ImGui::SetNextWindowPos(ImVec2(0, 20));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight - 20));
    ImGui::Begin("Game View", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImVec2 gameViewportSize = ImGui::GetContentRegionAvail();
    if (gameViewportSize.x > 0 && gameViewportSize.y > 0)
    {
        m_viewportManager->resize(ViewportType::Game, static_cast<ui32>(gameViewportSize.x), static_cast<ui32>(gameViewportSize.y));
        auto& gameViewport = m_viewportManager->getViewport(ViewportType::Game);

        ImVec2 imagePos = ImGui::GetCursorScreenPos();
        ImGui::Image((void*)gameViewport.renderTexture->getShaderResourceView(), gameViewportSize);

        bool isImageHovered = ImGui::IsItemHovered();
        ImVec2 mousePos = ImGui::GetMousePos();
        float localX = mousePos.x - imagePos.x;
        float localY = mousePos.y - imagePos.y;

        bool isMouseInImageBounds = (localX >= 0 && localY >= 0 &&
            localX < gameViewportSize.x && localY < gameViewportSize.y);

        m_viewportManager->updateViewportStates(ViewportType::Game,
            isImageHovered && isMouseInImageBounds,
            ImGui::IsWindowFocused(),
            localX, localY);
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0, halfHeight));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight));
    ImGui::Begin("Scene View", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImVec2 sceneViewportSize = ImGui::GetContentRegionAvail();
    if (sceneViewportSize.x > 0 && sceneViewportSize.y > 0)
    {
        m_viewportManager->resize(ViewportType::Scene, static_cast<ui32>(sceneViewportSize.x), static_cast<ui32>(sceneViewportSize.y));
        auto& sceneViewport = m_viewportManager->getViewport(ViewportType::Scene);

        ImVec2 imagePos = ImGui::GetCursorScreenPos();
        ImGui::Image((void*)sceneViewport.renderTexture->getShaderResourceView(), sceneViewportSize);

        bool isImageHovered = ImGui::IsItemHovered();
        ImVec2 mousePos = ImGui::GetMousePos();
        float localX = mousePos.x - imagePos.x;
        float localY = mousePos.y - imagePos.y;

        bool isMouseInImageBounds = (localX >= 0 && localY >= 0 &&
            localX < sceneViewportSize.x && localY < sceneViewportSize.y);

        m_viewportManager->updateViewportStates(ViewportType::Scene,
            isImageHovered && isMouseInImageBounds,
            ImGui::IsWindowFocused(),
            localX, localY);
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(halfWidth, 20));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, 100));
    ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    const char* stateText = "";
    switch (m_sceneStateManager->getCurrentState())
    {
    case SceneState::Edit: stateText = "Edit Mode"; break;
    case SceneState::Play: stateText = "Play Mode"; break;
    case SceneState::Pause: stateText = "Pause Mode"; break;
    }

    ImGui::Text("Current State: %s", stateText);
    ImGui::Separator();

    bool isPlaying = m_sceneStateManager->isPlayMode();
    if (isPlaying)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
    }

    if (ImGui::Button("Play"))
    {
        if (m_sceneStateManager->isEditMode())
        {
            m_sceneStateManager->transitionToPlay();
        }
        else if (m_sceneStateManager->isPauseMode())
        {
            m_sceneStateManager->transitionToPlay();
        }
        else if (m_sceneStateManager->isPlayMode())
        {
            m_sceneStateManager->transitionToPause();
        }
    }

    if (isPlaying)
    {
        ImGui::PopStyleColor(3);
    }

    ImGui::SameLine();

    bool canFrameStep = m_sceneStateManager->isPauseMode();
    if (!canFrameStep)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
    }

    if (ImGui::Button("Frame Step"))
    {
        if (canFrameStep)
        {
            m_sceneStateManager->frameStep();
        }
    }

    if (!canFrameStep)
    {
        ImGui::PopStyleVar();
    }

    ImGui::SameLine();

    bool canStop = !m_sceneStateManager->isEditMode();
    if (!canStop)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
    }

    if (ImGui::Button("Stop"))
    {
        if (canStop)
        {
            m_sceneStateManager->transitionToEdit();
        }
    }

    if (!canStop)
    {
        ImGui::PopStyleVar();
    }

    if (m_sceneStateManager->isEditMode())
    {
        ImGui::Separator();
        ImGui::Text("Quick Actions:");

        bool canUndo = m_undoRedoSystem->canUndo();
        bool canRedo = m_undoRedoSystem->canRedo();

        if (!canUndo)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
        }

        if (ImGui::Button("Undo"))
        {
            if (canUndo)
            {
                m_undoRedoSystem->undo();
                DX3DLogInfo("Undo action performed");
            }
        }

        if (!canUndo)
        {
            ImGui::PopStyleVar();
        }

        ImGui::SameLine();

        if (!canRedo)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
        }

        if (ImGui::Button("Redo"))
        {
            if (canRedo)
            {
                m_undoRedoSystem->redo();
                DX3DLogInfo("Redo action performed");
            }
        }

        if (!canRedo)
        {
            ImGui::PopStyleVar();
        }
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(halfWidth, 120));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight - 120));
    ImGui::Begin("Scene Outliner", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Physics Demo");
    ImGui::Separator();

    ImGui::Text("Objects: %zu", m_gameObjects.size());
    ImGui::Text("Delta Time: %.3f ms", m_deltaTime * 1000.0f);
    ImGui::Text("FPS: %.1f", 1.0f / m_deltaTime);

    if (m_sceneStateManager->isEditMode())
    {
        ImGui::Text("Undo Stack: %d actions", m_undoRedoSystem->getUndoCount());
        ImGui::Text("Redo Stack: %d actions", m_undoRedoSystem->getRedoCount());
    }

    ImGui::Separator();
    ImGui::Text("Scene Hierarchy");
    ImGui::BeginChild("Outliner", ImVec2(0, 0), true);

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

    ImGui::SetNextWindowPos(ImVec2(halfWidth, halfHeight));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, inspectorHeight));
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    auto selectedObject = m_selectionSystem->getSelectedObject();
    if (selectedObject)
    {
        auto cameraObject = std::dynamic_pointer_cast<CameraObject>(selectedObject);

        ImGui::Text("Selected Object");
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Vector3 pos = selectedObject->getPosition();
            Vector3 rot = selectedObject->getRotation();
            Vector3 scale = selectedObject->getScale();

            Vector3 rotDegrees = Vector3(
                rot.x * 180.0f / 3.14159f,
                rot.y * 180.0f / 3.14159f,
                rot.z * 180.0f / 3.14159f
            );

            bool transformChanged = false;

            if (m_transformTracking.trackedObject.lock() != selectedObject)
            {
                m_transformTracking.isDragging = false;
                m_transformTracking.trackedObject = selectedObject;
            }

            if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
            {
                if (!m_transformTracking.isDragging)
                {
                    m_transformTracking.originalPosition = selectedObject->getPosition();
                    m_transformTracking.originalRotation = selectedObject->getRotation();
                    m_transformTracking.originalScale = selectedObject->getScale();
                    m_transformTracking.isDragging = true;
                }

                selectedObject->setPosition(pos);
                transformChanged = true;
            }

            bool positionActive = ImGui::IsItemActive();

            if (ImGui::DragFloat3("Rotation", &rotDegrees.x, 1.0f))
            {
                if (!m_transformTracking.isDragging)
                {
                    m_transformTracking.originalPosition = selectedObject->getPosition();
                    m_transformTracking.originalRotation = selectedObject->getRotation();
                    m_transformTracking.originalScale = selectedObject->getScale();
                    m_transformTracking.isDragging = true;
                }

                Vector3 rotRadians = Vector3(
                    rotDegrees.x * 3.14159f / 180.0f,
                    rotDegrees.y * 3.14159f / 180.0f,
                    rotDegrees.z * 3.14159f / 180.0f
                );
                selectedObject->setRotation(rotRadians);
                transformChanged = true;
            }

            bool rotationActive = ImGui::IsItemActive();

            if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.01f, 10.0f))
            {
                if (!m_transformTracking.isDragging)
                {
                    m_transformTracking.originalPosition = selectedObject->getPosition();
                    m_transformTracking.originalRotation = selectedObject->getRotation();
                    m_transformTracking.originalScale = selectedObject->getScale();
                    m_transformTracking.isDragging = true;
                }

                selectedObject->setScale(scale);
                transformChanged = true;
            }

            bool scaleActive = ImGui::IsItemActive();
            bool anyControlActive = positionActive || rotationActive || scaleActive;

            if (m_transformTracking.isDragging && !anyControlActive)
            {
                if (m_sceneStateManager->isEditMode())
                {
                    Vector3 newPos = selectedObject->getPosition();
                    Vector3 newRot = selectedObject->getRotation();
                    Vector3 newScale = selectedObject->getScale();

                    const float epsilon = 0.001f;
                    auto isChanged = [epsilon](float a, float b) { return std::abs(a - b) > epsilon; };

                    if (isChanged(m_transformTracking.originalPosition.x, newPos.x) ||
                        isChanged(m_transformTracking.originalPosition.y, newPos.y) ||
                        isChanged(m_transformTracking.originalPosition.z, newPos.z) ||
                        isChanged(m_transformTracking.originalRotation.x, newRot.x) ||
                        isChanged(m_transformTracking.originalRotation.y, newRot.y) ||
                        isChanged(m_transformTracking.originalRotation.z, newRot.z) ||
                        isChanged(m_transformTracking.originalScale.x, newScale.x) ||
                        isChanged(m_transformTracking.originalScale.y, newScale.y) ||
                        isChanged(m_transformTracking.originalScale.z, newScale.z))
                    {
                        auto transformAction = std::make_unique<TransformAction>(
                            selectedObject,
                            m_transformTracking.originalPosition, newPos,
                            m_transformTracking.originalRotation, newRot,
                            m_transformTracking.originalScale, newScale
                        );

                        m_undoRedoSystem->recordAction(std::move(transformAction));
                        DX3DLogInfo("Transform change recorded for undo/redo");
                    }
                }

                m_transformTracking.isDragging = false;
            }

            if (transformChanged)
            {
                if (selectedObject->hasPhysics() && m_sceneStateManager->isEditMode())
                {
                    PhysicsBodyType bodyType = PhysicsBodyType::Dynamic;
                    auto& componentManager = ComponentManager::getInstance();
                    auto* physicsComp = componentManager.getComponent<PhysicsComponent>(selectedObject->getEntity().getID());
                    if (physicsComp)
                    {
                        bodyType = physicsComp->bodyType;
                    }

                    selectedObject->disablePhysics();
                    selectedObject->enablePhysics(bodyType);
                }
            }
        }

        if (cameraObject)
        {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                float fov = cameraObject->getFOV() * 180.0f / 3.14159f;
                float nearPlane = cameraObject->getNearPlane();
                float farPlane = cameraObject->getFarPlane();

                bool cameraChanged = false;

                if (ImGui::SliderFloat("Field of View", &fov, 10.0f, 120.0f, "%.1f°"))
                {
                    cameraObject->setFOV(fov * 3.14159f / 180.0f);
                    cameraChanged = true;
                }

                if (ImGui::DragFloat("Near Plane", &nearPlane, 0.01f, 0.01f, 10.0f, "%.3f"))
                {
                    cameraObject->setNearPlane(nearPlane);
                    cameraChanged = true;
                }

                if (ImGui::DragFloat("Far Plane", &farPlane, 1.0f, 1.0f, 1000.0f, "%.1f"))
                {
                    cameraObject->setFarPlane(farPlane);
                    cameraChanged = true;
                }

                ImGui::Separator();

                if (ImGui::Button("Align with View", ImVec2(-1, 0)))
                {
                    cameraObject->alignWithView(*m_sceneCamera);
                    DX3DLogInfo("Game camera aligned with scene view");
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Align the game camera with the current scene view camera");
                }
            }
        }

        if (selectedObject->hasPhysics())
        {
            if (ImGui::CollapsingHeader("Physics"))
            {
                Vector3 vel = selectedObject->getLinearVelocity();
                ImGui::Text("Linear Velocity: (%.2f, %.2f, %.2f)", vel.x, vel.y, vel.z);
                ImGui::Text("Speed: %.2f", std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z));

                if (m_sceneStateManager->isEditMode())
                {
                    if (ImGui::Button("Apply Upward Impulse"))
                    {
                        selectedObject->applyImpulse(Vector3(0.0f, 10.0f, 0.0f));
                    }
                }
            }
        }

        if (ImGui::CollapsingHeader("Object Info"))
        {
            ImGui::Text("Entity ID: %u", selectedObject->getEntity().getID());

            std::string objectType = "Unknown";
            if (std::dynamic_pointer_cast<Cube>(selectedObject)) objectType = "Cube";
            else if (std::dynamic_pointer_cast<Plane>(selectedObject)) objectType = "Plane";
            else if (std::dynamic_pointer_cast<Sphere>(selectedObject)) objectType = "Sphere";
            else if (std::dynamic_pointer_cast<Cylinder>(selectedObject)) objectType = "Cylinder";
            else if (std::dynamic_pointer_cast<Capsule>(selectedObject)) objectType = "Capsule";
            else if (std::dynamic_pointer_cast<CameraObject>(selectedObject)) objectType = "Camera";

            ImGui::Text("Type: %s", objectType.c_str());
            ImGui::Text("Has Physics: %s", selectedObject->hasPhysics() ? "Yes" : "No");
        }

        if (m_sceneStateManager->isEditMode())
        {
            ImGui::Separator();
            if (ImGui::Button("Delete Object", ImVec2(-1, 0)))
            {
                auto deleteAction = std::make_unique<DeleteAction>(selectedObject, m_gameObjects);
                m_undoRedoSystem->executeAction(std::move(deleteAction));
                m_selectionSystem->setSelectedObject(nullptr);
                DX3DLogInfo("Deleted selected object");
            }
        }
    }
    else
    {
        ImGui::Text("No object selected");
        ImGui::Text("Click on an object in the Scene View or Outliner to select it");
    }

    ImGui::End();

    renderDebugWindow();

    bool canUndo = m_undoRedoSystem->canUndo();
    bool canRedo = m_undoRedoSystem->canRedo();
    bool isEditMode = m_sceneStateManager->isEditMode();

    static int debugCounter = 0;
    if (++debugCounter % 60 == 0)
    {
        printf("Debug: canUndo=%s, canRedo=%s, isEditMode=%s, undoCount=%d, redoCount=%d\n",
            canUndo ? "true" : "false",
            canRedo ? "true" : "false",
            isEditMode ? "true" : "false",
            m_undoRedoSystem->getUndoCount(),
            m_undoRedoSystem->getRedoCount());
    }
}

void dx3d::Game::render()
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto& swapChain = m_display->getSwapChain();

    auto& sceneViewport = m_viewportManager->getViewport(ViewportType::Scene);
    auto& gameViewport = m_viewportManager->getViewport(ViewportType::Game);

    float aspectRatio = static_cast<float>(sceneViewport.width) / static_cast<float>(sceneViewport.height);
    Matrix4x4 sceneProjMatrix = Matrix4x4::CreatePerspectiveFovLH(1.0472f, aspectRatio, 0.1f, 100.0f);
    renderScene(*m_sceneCamera, sceneProjMatrix, sceneViewport.renderTexture.get());

    aspectRatio = static_cast<float>(gameViewport.width) / static_cast<float>(gameViewport.height);
    Matrix4x4 gameProjMatrix = m_gameCamera->getProjectionMatrix(aspectRatio);
    renderScene(m_gameCamera->getCamera(), gameProjMatrix, gameViewport.renderTexture.get());

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

        cube->enablePhysics(PhysicsBodyType::Dynamic);
        cube->setPhysicsMass(2.0f);
        cube->setPhysicsRestitution(0.8f);
        cube->setPhysicsFriction(0.8f);

        m_gameObjects.push_back(cube);
    }

    DX3DLogInfo(("Spawned a new Cube Demo with " + std::to_string(numCubes) + " cubes").c_str());
}

void dx3d::Game::spawnCube()
{
    Vector3 position(0.0f, 5.0f, 0.0f);
    Vector3 scale(1.0f, 1.0f, 1.0f);

    auto cube = std::make_shared<Cube>(position, Vector3(0, 0, 0), scale);
    cube->enablePhysics(PhysicsBodyType::Dynamic);
    cube->setPhysicsMass(1.0f);
    cube->setPhysicsRestitution(0.5f);
    cube->setPhysicsFriction(0.5f);

    auto createAction = std::make_unique<CreateAction>(cube, m_gameObjects);
    m_undoRedoSystem->executeAction(std::move(createAction));

    m_selectionSystem->setSelectedObject(cube);
    DX3DLogInfo("Spawned Cube");
}

void dx3d::Game::spawnSphere()
{
    Vector3 position(0.0f, 5.0f, 0.0f);
    Vector3 scale(1.0f, 1.0f, 1.0f);

    auto sphere = std::make_shared<Sphere>(position, Vector3(0, 0, 0), scale);
    sphere->enablePhysics(PhysicsBodyType::Dynamic);
    sphere->setPhysicsMass(1.0f);
    sphere->setPhysicsRestitution(0.7f);
    sphere->setPhysicsFriction(0.3f);

    auto createAction = std::make_unique<CreateAction>(sphere, m_gameObjects);
    m_undoRedoSystem->executeAction(std::move(createAction));

    m_selectionSystem->setSelectedObject(sphere);
    DX3DLogInfo("Spawned Sphere");
}

void dx3d::Game::spawnCapsule()
{
    Vector3 position(0.0f, 5.0f, 0.0f);
    Vector3 scale(1.0f, 1.0f, 1.0f);

    auto capsule = std::make_shared<Capsule>(position, Vector3(0, 0, 0), scale);
    capsule->enablePhysics(PhysicsBodyType::Dynamic);
    capsule->setPhysicsMass(1.0f);
    capsule->setPhysicsRestitution(0.4f);
    capsule->setPhysicsFriction(0.6f);

    auto createAction = std::make_unique<CreateAction>(capsule, m_gameObjects);
    m_undoRedoSystem->executeAction(std::move(createAction));

    m_selectionSystem->setSelectedObject(capsule);
    DX3DLogInfo("Spawned Capsule");
}

void dx3d::Game::spawnCylinder()
{
    Vector3 position(0.0f, 5.0f, 0.0f);
    Vector3 scale(1.0f, 1.0f, 1.0f);

    auto cylinder = std::make_shared<Cylinder>(position, Vector3(0, 0, 0), scale);
    cylinder->enablePhysics(PhysicsBodyType::Dynamic);
    cylinder->setPhysicsMass(1.0f);
    cylinder->setPhysicsRestitution(0.3f);
    cylinder->setPhysicsFriction(0.7f);

    auto createAction = std::make_unique<CreateAction>(cylinder, m_gameObjects);
    m_undoRedoSystem->executeAction(std::move(createAction));

    m_selectionSystem->setSelectedObject(cylinder);
    DX3DLogInfo("Spawned Cylinder");
}

void dx3d::Game::spawnModel(const std::string& filename)
{
    try
    {
        auto& renderSystem = m_graphicsEngine->getRenderSystem();
        auto resourceDesc = renderSystem.getGraphicsResourceDesc();

        auto model = Model::LoadFromFile(filename, resourceDesc);

        if (model && model->isReadyForRendering())
        {
            Vector3 position(0.0f, 5.0f, 0.0f);
            Vector3 scale(1.0f, 1.0f, 1.0f);

            model->setPosition(position);
            model->setScale(scale);
            model->enablePhysics(PhysicsBodyType::Dynamic);
            model->setPhysicsMass(1.0f);
            model->setPhysicsRestitution(0.3f);
            model->setPhysicsFriction(0.6f);

            auto createAction = std::make_unique<CreateAction>(model, m_gameObjects);
            m_undoRedoSystem->executeAction(std::move(createAction));

            m_selectionSystem->setSelectedObject(model);
            DX3DLogInfo(("Spawned Model: " + filename).c_str());
        }
        else
        {
            DX3DLogError(("Failed to load model: " + filename).c_str());
        }
    }
    catch (const std::exception& e)
    {
        DX3DLogError(("Error loading model " + filename + ": " + e.what()).c_str());
    }
}

void dx3d::Game::spawnPlane()
{
    Vector3 position(0.0f, 0.0f, 0.0f);
    Vector3 scale(10.0f, 1.0f, 10.0f);

    auto plane = std::make_shared<Plane>(position, Vector3(0, 0, 0), scale);
    plane->enablePhysics(PhysicsBodyType::Static);
    plane->setPhysicsRestitution(0.0f);
    plane->setPhysicsFriction(0.7f);

    auto createAction = std::make_unique<CreateAction>(plane, m_gameObjects);
    m_undoRedoSystem->executeAction(std::move(createAction));

    m_selectionSystem->setSelectedObject(plane);
    DX3DLogInfo("Spawned Plane");
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