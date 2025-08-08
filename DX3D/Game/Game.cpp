﻿#include <../Game/Game.h>
#include <../Window/Window.h>
#include <../Graphics/GraphicsEngine.h>
#include <../Core/EventLog.h>
#include <../Game/Display.h>
#include <../Game/SceneCamera.h>
#include <../Input/Input.h>
#include <../Graphics/RenderSystem.h>
#include <../Graphics/SwapChain.h>
#include <../Graphics/DeviceContext.h>
#include <../Graphics/VertexBuffer.h>
#include <../Graphics/IndexBuffer.h>
#include <../Graphics/ConstantBuffer.h>
#include <../Graphics/DepthBuffer.h>
#include <../Graphics/RenderTexture.h>

#include <../Graphics/Primitives/CameraObject.h>
#include <../Graphics/Shaders/Rainbow3DShader.h>
#include <../Graphics/Shaders/WhiteShader.h>
#include <../Graphics/Shaders/FogShader.h>
#include <../Graphics/Shaders/ModelShader.h>
#include <../Graphics/Shaders/ModelVertexShader.h>
#include <../Graphics/Shaders/DepthShader.h>
#include <../Graphics/ShadowMap.h>
#include <../Math/Math.h>
#include <../Game/ViewportManager.h>
#include <../Game/SelectionSystem.h>
#include <../Scene/SceneStateManager.h>
#include <../Game/FPSCameraController.h>
#include <../Game/UndoRedoSystem.h>

#include <../Graphics/Primitives/AGameObject.h>
#include <../Graphics/Primitives/Cube.h>
#include <../Graphics/Primitives/Plane.h>
#include <../Graphics/Primitives/Sphere.h>
#include <../Graphics/Primitives/Capsule.h>
#include <../Graphics/Primitives/Cylinder.h>
#include <../Graphics/Primitives/Model.h>
#include <../Assets/ModelParser.h>

#include <../ECS/ComponentManager.h>
#include <../ECS/Components/TransformComponent.h>
#include <../ECS/Components/PhysicsComponent.h>
#include <../Physics/PhysicsSystem.h>

#include <../UI/UIManager.h>
#include <../JSON/json.hpp>

#include <../Graphics/ResourceManager.h>
#include <../ECS/Components/MaterialComponent.h>

#include <chrono>      
#include <iomanip>  
#include <sstream>  
#include <cmath>
#include <fstream>
#include <random>
#include <string>
#include <cstdio>
#include <filesystem>
#include <DirectXMath.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

dx3d::Game::Game(const GameDesc& desc) :
    Core({ *std::make_unique<EventLog>(desc.logLevel).release() }),
    m_loggerPtr(&m_loggerInstance)
{
    m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{ m_loggerInstance });
    m_display = std::make_unique<Display>(DisplayDesc{ {m_loggerInstance,desc.windowSize},m_graphicsEngine->getRenderSystem() });

    m_previousTime = std::chrono::steady_clock::now();

    m_sceneStateManager = std::make_unique<SceneStateManager>();
    m_fpsController = std::make_unique<FPSCameraController>();

    m_undoRedoSystem = std::make_unique<UndoRedoSystem>(10);

    createRenderingResources();

    m_fpsController->setCamera(m_sceneCamera.get());
    m_sceneStateManager->addStateChangeCallback([this](SceneState oldState, SceneState newState) {
        onSceneStateChanged(oldState, newState);
        });

    UIManager::Dependencies uiDeps{
     m_loggerInstance,
     *m_undoRedoSystem,
     *m_selectionSystem,
     *m_sceneStateManager,
     *m_viewportManager,
     m_gameObjects,
     [this]() { return getSavedSceneFiles(); },  // Add this lambda
     [this](const std::string& filename) { loadScene(filename); },  // Add this lambda
     m_lights
    };
    m_uiManager = std::make_unique<UIManager>(uiDeps);

    spawnDirectionalLight();

    DX3DLogInfo("Game initialized with ECS, Physics, and Scene State systems.");
}

dx3d::Game::~Game()
{
    DX3DLogInfo("Game deallocation started.");

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    for (const auto& go : m_gameObjects)
    {
        if (go->hasPhysics())
        {
            go->disablePhysics();
        }
    }
    m_gameObjects.clear();

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
    componentManager.registerComponent<MaterialComponent>();

    PhysicsSystem::getInstance().initialize();
    ResourceManager::getInstance().initialize(resourceDesc);

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
    m_lightConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(LightConstantBuffer), resourceDesc);

    m_lightTransformConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(TransformationMatrices), resourceDesc);
    m_depthVertexShader = std::make_shared<VertexShader>(resourceDesc, DepthShader::GetVertexShaderCode());
    m_shadowMap = std::make_shared<ShadowMap>(2048, 2048, resourceDesc);

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    device->CreateSamplerState(&samplerDesc, &m_shadowSamplerState);

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

    m_sceneCamera = std::make_unique<SceneCamera>(
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
                auto deleteAction = std::make_unique<DeleteAction>(selectedObject, m_gameObjects, m_lights);
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


    if(m_deltaTime > 0.0f)
        updatePhysics(m_deltaTime);

    for (auto& gameObject : m_gameObjects)
    {
        if (gameObject->isEnabled())
        {
            gameObject->update(m_deltaTime);
        }
    }

    static float debugTimer = 0.0f;
    debugTimer += m_deltaTime;
    if (debugTimer >= 5.0f)
    {
        DX3DLogInfo(("Physics demo running - " + std::to_string(m_gameObjects.size()) + " objects").c_str());
        debugTimer = 0.0f;
    }
}

void dx3d::Game::loadScene(const std::string& filename)
{
    const std::string saveDir = "Saved Scenes";
    fs::path fullPath = fs::path(saveDir) / filename;

    std::ifstream i(fullPath);
    if (!i.is_open())
    {
        DX3DLogError(("Failed to open scene file: " + fullPath.string()).c_str());
        return;
    }

    json sceneJson;
    try {
        i >> sceneJson;
    }
    catch (json::parse_error& e) {
        DX3DLogError(("JSON parse error in scene file: " + std::string(e.what())).c_str());
        return;
    }

    // Clear the existing scene
    m_gameObjects.clear();
    m_lights.clear();
    m_selectionSystem->setSelectedObject(nullptr);
    m_undoRedoSystem->clear();

    // --- 1. LOAD SCENE CAMERA ---
    if (sceneJson.contains("SceneCameraData") && sceneJson["SceneCameraData"].is_array() && !sceneJson["SceneCameraData"].empty())
    {
        const auto& sceneCamJson = sceneJson["SceneCameraData"][0];
        if (sceneCamJson.contains("position") && sceneCamJson.contains("yaw") && sceneCamJson.contains("pitch"))
        {
            Vector3 position(
                sceneCamJson["position"]["x"],
                sceneCamJson["position"]["y"],
                sceneCamJson["position"]["z"]
            );
            float yaw = sceneCamJson["yaw"];
            float pitch = sceneCamJson["pitch"];

            m_sceneCamera->setPosition(position);

            // Recalculate the forward vector to orient the camera correctly
            Vector3 forward;
            forward.x = sin(yaw) * cos(pitch);
            forward.y = sin(pitch);
            forward.z = cos(yaw) * cos(pitch);
            m_sceneCamera->lookAt(position + forward);
        }
    }

    // --- 2. LOAD GAME OBJECTS ---
    if (sceneJson.contains("gameObjects") && sceneJson["gameObjects"].is_array())
    {
        for (const auto& goJson : sceneJson["gameObjects"])
        {
            std::string type = goJson.value("type", "Unknown");
            std::shared_ptr<BaseGameObject> newObject = nullptr;

            // --- Object Creation without Factory ---
            if (type == "Cube") {
                newObject = std::make_shared<Cube>();
            }
            else if (type == "Sphere") {
                newObject = std::make_shared<Sphere>();
            }
            else if (type == "Plane") {
                newObject = std::make_shared<Plane>();
            }
            else if (type == "Cylinder") {
                newObject = std::make_shared<Cylinder>();
            }
            else if (type == "Capsule") {
                newObject = std::make_shared<Capsule>();
            }
            else if (type == "DirectionalLight") {
                newObject = std::make_shared<DirectionalLight>();
            }
            else if (type == "PointLight") {
                newObject = std::make_shared<PointLight>();
            }
            else if (type == "SpotLight") {
                newObject = std::make_shared<SpotLight>();
            }
            else if (type == "Model") {
                std::string filePath = goJson.value("filePath", "");
                if (!filePath.empty()) {
                    auto& renderSystem = m_graphicsEngine->getRenderSystem();
                    newObject = Model::LoadFromFile(filePath, renderSystem.getGraphicsResourceDesc());
                }
                else {
                    newObject = std::make_shared<Model>(); // Create a default model if path is missing
                }
            }
            // Add any other object types here in the future

            if (!newObject) {
                DX3DLogWarning(("Unknown or unsupported object type in scene file: " + type).c_str());
                continue; // Skip this object and move to the next
            }

            // Load transform properties
            if (goJson.contains("position") && goJson.contains("rotation") && goJson.contains("scale")) {
                newObject->setPosition(Vector3(goJson["position"]["x"], goJson["position"]["y"], goJson["position"]["z"]));
                newObject->setRotation(Vector3(goJson["rotation"]["x"], goJson["rotation"]["y"], goJson["rotation"]["z"]));
                newObject->setScale(Vector3(goJson["scale"]["x"], goJson["scale"]["y"], goJson["scale"]["z"]));
            }

            // Load physics properties
            if (goJson.contains("physics") && goJson["physics"]["enabled"]) {
                auto bodyTypeFromString = [](const std::string& typeStr) {
                    if (typeStr == "Static") return PhysicsBodyType::Static;
                    if (typeStr == "Kinematic") return PhysicsBodyType::Kinematic;
                    return PhysicsBodyType::Dynamic;
                    };

                PhysicsBodyType bodyType = bodyTypeFromString(goJson["physics"].value("bodyType", "Dynamic"));
                newObject->enablePhysics(bodyType);
                newObject->setPhysicsMass(goJson["physics"].value("mass", 1.0f));
                newObject->setPhysicsRestitution(goJson["physics"].value("restitution", 0.5f));
                newObject->setPhysicsFriction(goJson["physics"].value("friction", 0.5f));
            }

            // Load light properties
            if (auto light = std::dynamic_pointer_cast<LightObject>(newObject))
            {
                auto& lightData = light->getLightData();
                if (goJson.contains("lightPosition"))
                {
                    lightData.position.x = goJson["lightPosition"]["x"];
                    lightData.position.y = goJson["lightPosition"]["y"];
                    lightData.position.z = goJson["lightPosition"]["z"];
                }
                if (goJson.contains("lightColor"))
                {
                    lightData.color.x = goJson["lightColor"]["r"];
                    lightData.color.y = goJson["lightColor"]["g"];
                    lightData.color.z = goJson["lightColor"]["b"];
                }
                if (goJson.contains("lightDirection"))
                {
                    lightData.direction.x = goJson["lightDirection"]["x"];
                    lightData.direction.y = goJson["lightDirection"]["y"];
                    lightData.direction.z = goJson["lightDirection"]["z"];
                }
                lightData.intensity = goJson.value("lightIntensity", 1.0f);
                lightData.radius = goJson.value("lightRange", 10.0f);
                lightData.spot_angle_inner = goJson.value("lightAngle-Inner", 0.785f);
                lightData.spot_angle_outer = goJson.value("lightAngle-Outer", 0.959f);
                lightData.spot_falloff = goJson.value("lightSpotFalloff", 1.0f);
                lightData.padding = goJson.value("lightPadding", 0.0f);

                m_lights.push_back(light);
            }

            m_gameObjects.push_back(newObject);
        }
    }

    m_gameObjects.push_back(m_gameCamera);

    DX3DLogInfo(("Scene loaded successfully from " + filename).c_str());
}

std::vector<std::string> dx3d::Game::getSavedSceneFiles() const
{
    std::vector<std::string> files;
    const std::string saveDir = "Saved Scenes";

    if (fs::exists(saveDir) && fs::is_directory(saveDir))
    {
        for (const auto& entry : fs::directory_iterator(saveDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                files.push_back(entry.path().filename().string());
            }
        }
    }
    return files;
}

std::string dx3d::Game::getCurrentTimeAndDate()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    // Create a tm struct to safely hold the time components
    std::tm tm_buf;
    // Use the thread-safe localtime_s instead of localtime
    localtime_s(&tm_buf, &in_time_t);

    std::stringstream ss;
    // Pass the address of your local tm struct to std::put_time
    ss << std::put_time(&tm_buf, "%Y-%m-%d_%H-%M-%S");
    std::string filename = ss.str() + ".json";

    return filename;
}

void dx3d::Game::saveScene()
{
    const std::string saveDir = "Saved Scenes";
    std::string filename = this->getCurrentTimeAndDate();

    try 
    {
        fs::create_directory(saveDir);
        fs::path fullPath = fs::path(saveDir) / filename;

        json sceneJson;
        sceneJson["sceneName"] = "MyScene";
        sceneJson["SceneCameraData"] = json::array();

        json sceneCamJson;

        sceneCamJson["position"] = { {"x", this->m_sceneCamera->getPosition().x}, {"y", this->m_sceneCamera->getPosition().y}, {"z", this->m_sceneCamera->getPosition().z} };
        sceneCamJson["forward"] = { {"x", this->m_sceneCamera->getForward().x}, {"y", this->m_sceneCamera->getForward().y}, {"z", this->m_sceneCamera->getForward().z} };
        sceneCamJson["right"] = { {"x", this->m_sceneCamera->getRight().x}, {"y", this->m_sceneCamera->getRight().y}, {"z", this->m_sceneCamera->getRight().z} };
        sceneCamJson["up"] = { {"x", this->m_sceneCamera->getUp().x}, {"y", this->m_sceneCamera->getUp().y}, {"z", this->m_sceneCamera->getUp().z} };
        sceneCamJson["worldUp"] = { {"x", this->m_sceneCamera->getWorldUp().x}, {"y", this->m_sceneCamera->getWorldUp().y}, {"z", this->m_sceneCamera->getWorldUp().z} };

        sceneCamJson["yaw"] = this->m_sceneCamera->getYaw();
        sceneCamJson["pitch"] = this->m_sceneCamera->getPitch();
        sceneCamJson["roll"] = this->m_sceneCamera->getRoll();

        json matrixArray = json::array();
        for (int i = 0; i < 4; i++)
        {
            json rowArray = json::array();
            for (int j = 0; j < 4; j++)
            {
                rowArray.push_back(this->m_sceneCamera->getViewMatrix().m[i][j]);
            }
            matrixArray.push_back(rowArray);
        }

        sceneCamJson["viewMatrix"] = matrixArray;
        sceneJson["SceneCameraData"].push_back(sceneCamJson);

        sceneJson["gameObjects"] = json::array();

        sceneJson["gameObjects"] = json::array();

        if (!this->m_gameObjects.empty()) {
            for (const auto& go : m_gameObjects)
            {
                json goJson;
                if (auto model = std::dynamic_pointer_cast<Model>(go))
                {
                    goJson["type"] = "Model";
                    goJson["filePath"] = model->getFilePath();
                }
                else
                {
                    goJson["type"] = go->getObjectType();
                }

                goJson["position"] = { {"x", go->getPosition().x}, {"y", go->getPosition().y}, {"z", go->getPosition().z} };
                goJson["rotation"] = { {"x", go->getRotation().x}, {"y", go->getRotation().y}, {"z", go->getRotation().z} };
                goJson["scale"] = { {"x", go->getScale().x}, {"y", go->getScale().y}, {"z", go->getScale().z} };

                // Save physics
                if (go->hasPhysics())
                {
                    auto* physicsComp = dx3d::ComponentManager::getInstance().getComponent<PhysicsComponent>(go->getEntity().getID());
                    if (physicsComp)
                    {
                        // Helper to convert enum to string
                        auto bodyTypeToString = [](PhysicsBodyType type) {
                            switch (type) {
                            case PhysicsBodyType::Static: return "Static";
                            case PhysicsBodyType::Kinematic: return "Kinematic";
                            case PhysicsBodyType::Dynamic: return "Dynamic";
                            default: return "Unknown";
                            }
                            };

                        goJson["physics"] = {
                            {"enabled", true},
                            {"bodyType", bodyTypeToString(physicsComp->bodyType)},
                            {"mass", physicsComp->mass},
                            {"restitution", physicsComp->restitution},
                            {"friction", physicsComp->friction}
                        };
                    }
                }
                else
                {
                    goJson["physics"] = {
                        {"enabled", false}
                    };
                }

                /*if (go->hasMaterial()) {
                    auto* materialComp = dx3d::ComponentManager::getInstance().getComponent<MaterialComponent>(go->getEntity().getID());
                    if (materialComp) {

                        goJson["material"] = {
                            //
                        }
                        std::shared_ptr<Material> material;
                        std::string textureFileName;
                        bool hasTexture = false;
                    }
                }
                else {
                    goJson["material"] = {
                        {"enabled", false}
                    };
                }*/

                if (auto light = std::dynamic_pointer_cast<LightObject>(go))
                {
                    const auto& lightData = light->getLightData();
                    goJson["lightPosition"] = { {"x", lightData.position.x}, {"y", lightData.position.y}, {"z", lightData.position.z} };
                    goJson["lightColor"] = { {"r", lightData.color.x}, {"g", lightData.color.y}, {"b", lightData.color.z} };
                    goJson["lightDirection"] = { {"x", lightData.direction.x}, {"y", lightData.direction.y}, {"z", lightData.direction.z} };
                    goJson["lightIntensity"] = lightData.intensity;
                    goJson["lightRange"] = lightData.radius;
                    goJson["lightAngle-Inner"] = lightData.spot_angle_inner;
                    goJson["lightAngle-Outer"] = lightData.spot_angle_outer;
                    goJson["lightSpotFalloff"] = lightData.spot_falloff;
                    goJson["lightPadding"] = lightData.padding;
                }
                sceneJson["gameObjects"].push_back(goJson);
            }

        }

        std::ofstream o(fullPath);
        o << std::setw(4) << sceneJson << std::endl;
        DX3DLogInfo(("Scene exported to " + fullPath.string()).c_str());
    }
    catch (const fs::filesystem_error& e)
    {
        DX3DLogError(("Filesystem error during export: " + std::string(e.what())).c_str());
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

void dx3d::Game::renderScene(SceneCamera& camera, RenderTexture* renderTarget)
{
    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto d3dContext = deviceContext.getDeviceContext();

    LightConstantBuffer lcb;
    //new
    memset(&lcb, 0, sizeof(LightConstantBuffer));

    Vector3 camPos = camera.getPosition();
    lcb.camera_position = Vector4(camPos.x, camPos.y, camPos.z, 1.0f);
    lcb.ambient_color = m_ambientColor;
    lcb.num_lights = static_cast<UINT>(std::min((size_t)m_lights.size(), (size_t)MAX_LIGHTS_SUPPORTED));
    lcb.shadow_casting_light_index = m_shadowCastingLightIndex;

    for (int i = 0; i < lcb.num_lights; ++i)
    {
        lcb.lights[i] = m_lights[i]->getLightData();
    }
    lcb.light_view = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(m_lightViewMatrix.toXMMatrix()));
    lcb.light_projection = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(m_lightProjectionMatrix.toXMMatrix()));

    m_lightConstantBuffer->update(deviceContext, &lcb);


    if (renderTarget)
    {
        renderTarget->clear(deviceContext, 0.204f, 0.204f, 0.204f, 1.0f);
        renderTarget->setAsRenderTarget(deviceContext);
    }
    else
    {
        auto& swapChain = m_display->getSwapChain();
        deviceContext.clearRenderTargetColor(swapChain, 0.204f, 0.204f, 0.204f, 1.0f);
        deviceContext.clearDepthBuffer(*m_depthBuffer);
        deviceContext.setRenderTargetsWithDepth(swapChain, *m_depthBuffer);
    }

    ui32 viewportWidth = renderTarget ? renderTarget->getWidth() : m_display->getSize().width;
    ui32 viewportHeight = renderTarget ? renderTarget->getHeight() : m_display->getSize().height;

    Matrix4x4 projMatrix;
    if (viewportHeight > 0)
    {
        float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
        if (&camera == m_sceneCamera.get())
        {
            projMatrix = Matrix4x4::CreatePerspectiveFovLH(1.0472f, aspectRatio, 0.1f, 100.0f);
        }
        else if (auto gameCamObject = std::dynamic_pointer_cast<CameraObject>(m_gameCamera))
        {
            projMatrix = gameCamObject->getProjectionMatrix(aspectRatio);
        }
    }
    deviceContext.setViewportSize(viewportWidth, viewportHeight);

    ID3D11Buffer* transformCb = m_transformConstantBuffer->getBuffer();
    d3dContext->VSSetConstantBuffers(0, 1, &transformCb);
    d3dContext->OMSetDepthStencilState(m_solidDepthState, 0);

    bool isSceneView = (&camera == m_sceneCamera.get());



    for (const auto& gameObject : m_gameObjects)
    {
        if (!gameObject->isEnabled())
            continue;

        bool isCamera = std::dynamic_pointer_cast<CameraObject>(gameObject) != nullptr;
        if (!isSceneView && isCamera)
            continue;

        deviceContext.setVertexShader(m_modelVertexShader->getShader());
        deviceContext.setPixelShader(m_modelPixelShader->getShader());
        deviceContext.setInputLayout(m_modelVertexShader->getInputLayout());

        ID3D11Buffer* modelMatCb = m_modelMaterialConstantBuffer->getBuffer();
        d3dContext->PSSetConstantBuffers(1, 1, &modelMatCb);

        ID3D11Buffer* lightCb = m_lightConstantBuffer->getBuffer();
        d3dContext->PSSetConstantBuffers(2, 1, &lightCb);

        ID3D11ShaderResourceView* shadowSRV = m_shadowMap->getShaderResourceView();
        d3dContext->PSSetShaderResources(1, 1, &shadowSRV);
        d3dContext->PSSetSamplers(1, 1, &m_shadowSamplerState);

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

        else if (auto model = std::dynamic_pointer_cast<Model>(gameObject))
        {
            // Handle Model rendering
            if (model->isReadyForRendering())
            {
                // Switch to model shaders
                deviceContext.setVertexShader(m_modelVertexShader->getShader());
                deviceContext.setPixelShader(m_modelPixelShader->getShader());
                deviceContext.setInputLayout(m_modelVertexShader->getInputLayout());

                // Set up model material constant buffer
                ModelMaterialConstants mmc = {};

                // Render each mesh in the model
                for (size_t meshIdx = 0; meshIdx < model->getMeshCount(); ++meshIdx)
                {
                    auto mesh = model->getMesh(meshIdx);
                    if (!mesh || !mesh->isReadyForRendering())
                        continue;

                    // Set vertex and index buffers for this mesh
                    deviceContext.setVertexBuffer(*mesh->getVertexBuffer());
                    deviceContext.setIndexBuffer(*mesh->getIndexBuffer());

                    // Set up material
                    auto material = mesh->getMaterial();
                    if (material)
                    {
                        mmc.diffuseColor = material->getDiffuseColor();
                        mmc.ambientColor = material->getAmbientColor();
                        mmc.specularColor = material->getSpecularColor();
                        mmc.emissiveColor = material->getEmissiveColor();
                        mmc.specularPower = material->getSpecularPower();
                        mmc.opacity = material->getOpacity();
                        mmc.hasTexture = material->hasDiffuseTexture();

                        // Set texture if available
                        if (material->hasDiffuseTexture())
                        {
                            auto texture = material->getDiffuseTexture();
                            ID3D11ShaderResourceView* srv = texture->getShaderResourceView();
                            ID3D11SamplerState* sampler = texture->getSamplerState();
                            d3dContext->PSSetShaderResources(0, 1, &srv);
                            d3dContext->PSSetSamplers(0, 1, &sampler);
                        }
                    }
                    else
                    {
                        // Default material
                        mmc.diffuseColor = Vector4(0.7f, 0.7f, 0.7f, 1.0f);
                        mmc.ambientColor = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
                        mmc.specularColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
                        mmc.emissiveColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
                        mmc.specularPower = 32.0f;
                        mmc.opacity = 1.0f;
                        mmc.hasTexture = false;
                    }

                    // Update material constant buffer
                    m_modelMaterialConstantBuffer->update(deviceContext, &mmc);
                    ID3D11Buffer* materialCb = m_modelMaterialConstantBuffer->getBuffer();
                    d3dContext->PSSetConstantBuffers(1, 1, &materialCb);

                    // Set transformation matrix
                    TransformationMatrices transformMatrices;
                    transformMatrices.world = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(gameObject->getWorldMatrix().toXMMatrix()));
                    transformMatrices.view = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(camera.getViewMatrix().toXMMatrix()));
                    transformMatrices.projection = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(projMatrix.toXMMatrix()));
                    m_transformConstantBuffer->update(deviceContext, &transformMatrices);

                    // Draw this mesh
                    deviceContext.drawIndexed(mesh->getIndexCount(), 0, 0);
                }

                // Reset to fog shaders for other objects
                deviceContext.setVertexShader(m_fogVertexShader->getShader());
                deviceContext.setPixelShader(m_fogPixelShader->getShader());
                deviceContext.setInputLayout(m_fogVertexShader->getInputLayout());
                continue; // Skip the normal primitive rendering path
            }
        }

        if (bufferSet)
        {
            // Setup material for this object (reads from ECS MaterialComponent)
            setupMaterialForObject(gameObject, deviceContext);

            // Set material constant buffer
            ID3D11Buffer* materialCb = m_modelMaterialConstantBuffer->getBuffer();
            d3dContext->PSSetConstantBuffers(1, 1, &materialCb);

            TransformationMatrices transformMatrices;
            transformMatrices.world = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(gameObject->getWorldMatrix().toXMMatrix()));
            transformMatrices.view = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(camera.getViewMatrix().toXMMatrix()));
            transformMatrices.projection = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(projMatrix.toXMMatrix())); // Use the new projMatrix
            m_transformConstantBuffer->update(deviceContext, &transformMatrices);

            deviceContext.drawIndexed(indexCount, 0, 0);
        }
    }

    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    d3dContext->PSSetShaderResources(1, 1, nullSRV);
}

void dx3d::Game::renderShadowMapPass()
{
    auto& deviceContext = m_graphicsEngine->getRenderSystem().getDeviceContext();
    auto d3dContext = deviceContext.getDeviceContext();

    m_shadowCastingLightIndex = -1;
    Light* shadowCastingLight = nullptr;
    std::shared_ptr<LightObject> shadowCastingObject = nullptr;

    // Prioritize a Directional Light for shadows if one exists.
    for (int i = 0; i < m_lights.size(); ++i) {
        if (m_lights[i] && m_lights[i]->getLightData().type == LIGHT_TYPE_DIRECTIONAL) {
            shadowCastingObject = m_lights[i];
            m_shadowCastingLightIndex = i;
            break;
        }
    }
    // If no Directional Light was found, find the first Spot Light.
    if (!shadowCastingObject) {
        for (int i = 0; i < m_lights.size(); ++i) {
            if (m_lights[i] && m_lights[i]->getLightData().type == LIGHT_TYPE_SPOT) {
                shadowCastingObject = m_lights[i];
                m_shadowCastingLightIndex = i;
                break;
            }
        }
    }

    if (!shadowCastingObject)
    {
        m_lightViewMatrix = Matrix4x4();
        m_lightProjectionMatrix = Matrix4x4();
        return;
    }

    shadowCastingLight = &shadowCastingObject->getLightData();

    m_shadowMap->clear(deviceContext);
    m_shadowMap->setAsRenderTarget(deviceContext);

    deviceContext.setVertexShader(m_depthVertexShader->getShader());
    d3dContext->PSSetShader(nullptr, nullptr, 0);
    deviceContext.setInputLayout(m_depthVertexShader->getInputLayout());

    Matrix4x4 lightView, lightProjection;

    if (shadowCastingLight->type == LIGHT_TYPE_DIRECTIONAL)
    {
        Vector3 lightPos = Vector3(0, 0, 0) - (shadowCastingLight->direction * 50.0f);
        Vector3 target = Vector3(0, 0, 0);
        lightView = Matrix4x4::CreateLookAtLH(lightPos, target, Vector3(0, 1, 0));
        lightProjection = Matrix4x4::fromXMMatrix(DirectX::XMMatrixOrthographicLH(40.0f, 40.0f, 1.0f, 100.0f));
    }
    else if (shadowCastingLight->type == LIGHT_TYPE_SPOT)
    {
        // Get the world matrix of the light source itself
        Matrix4x4 world = shadowCastingObject->getWorldMatrix();
        DirectX::XMMATRIX xmWorld = world.toXMMatrix(); // Convert to DirectX matrix

        // --- Calculate World-Space Direction ---
        // Define local forward vector (Z-axis) for DirectX
        DirectX::XMVECTOR localForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        // Transform and normalize using DirectXMath functions
        DirectX::XMVECTOR worldForward = DirectX::XMVector3TransformNormal(localForward, xmWorld);
        worldForward = DirectX::XMVector3Normalize(worldForward);
        // Convert back to your Vector3 class (assuming a constructor exists)
        Vector3 lightDir = Vector3(worldForward);

        // --- Calculate World-Space Up Vector ---
        // Define local up vector (Y-axis) for DirectX
        DirectX::XMVECTOR localUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        // Transform and normalize using DirectXMath functions
        DirectX::XMVECTOR worldUp = DirectX::XMVector3TransformNormal(localUp, xmWorld);
        worldUp = DirectX::XMVector3Normalize(worldUp);
        // Convert back to your Vector3 class
        Vector3 up = Vector3(worldUp);

        // Position and Target are calculated as before
        Vector3 lightPos(world.m[3][0], world.m[3][1], world.m[3][2]);
        Vector3 target = lightPos + lightDir;

        lightView = Matrix4x4::CreateLookAtLH(lightPos, target, up);

        float fov_degrees = shadowCastingLight->spot_angle_outer * 2.0f;
        float fov_radians = DirectX::XMConvertToRadians(fov_degrees);

        lightProjection = Matrix4x4::CreatePerspectiveFovLH(
            fov_radians, // Use the corrected value in radians
            1.0f,
            0.1f,
            shadowCastingLight->radius
        );
    }
    /*else if (shadowCastingLight->type == LIGHT_TYPE_SPOT)
    {
        Vector3 lightPos = shadowCastingLight->position;
        Vector3 lightDir = Vector3::Normalize(shadowCastingLight->direction);
        Vector3 target = lightPos + lightDir;

        // Get the 'up' vector directly from the game object's transform matrix.
        Matrix4x4 world = shadowCastingObject->getWorldMatrix();
        Vector3 up = Vector3::Normalize(Vector3(world.m[1][0], world.m[1][1], world.m[1][2]));

        lightView = Matrix4x4::CreateLookAtLH(lightPos, target, up);
        lightProjection = Matrix4x4::CreatePerspectiveFovLH(shadowCastingLight->spot_angle_outer * 2.0f, 1.0f, 0.1f, shadowCastingLight->radius);
    }*/

    // Store the calculated matrices for the main render pass
    m_lightViewMatrix = lightView;
    m_lightProjectionMatrix = lightProjection;

    // Render all shadow-casting objects
    for (const auto& gameObject : m_gameObjects)
    {
        if (!gameObject || std::dynamic_pointer_cast<LightObject>(gameObject) || std::dynamic_pointer_cast<CameraObject>(gameObject)) {
            continue;
        }

        LightTransformMatrices ltm;
        ltm.world = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(gameObject->getWorldMatrix().toXMMatrix()));
        ltm.light_view = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(lightView.toXMMatrix()));
        ltm.light_projection = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(lightProjection.toXMMatrix()));
        m_lightTransformConstantBuffer->update(deviceContext, &ltm);

        ID3D11Buffer* lightTransformCb = m_lightTransformConstantBuffer->getBuffer();
        d3dContext->VSSetConstantBuffers(0, 1, &lightTransformCb);

        bool bufferSet = false;
        ui32 indexCount = 0;
        if (std::dynamic_pointer_cast<Cube>(gameObject)) {
            deviceContext.setVertexBuffer(*m_cubeVertexBuffer);
            deviceContext.setIndexBuffer(*m_cubeIndexBuffer);
            indexCount = Cube::GetIndexCount();
            bufferSet = true;
        }
        else if (std::dynamic_pointer_cast<Plane>(gameObject)) {
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

        if (bufferSet) {
            deviceContext.drawIndexed(indexCount, 0, 0);
        }
    }
}

void dx3d::Game::PrintMatrix(const char* name, const Matrix4x4& mat) {
    printf("--- Matrix: %s ---\n", name);
    for (int i = 0; i < 4; ++i) {
        printf("  [%.2f, %.2f, %.2f, %.2f]\n", mat.m[i][0], mat.m[i][1], mat.m[i][2], mat.m[i][3]);
    }
    printf("-----------------------\n");
}

void dx3d::Game::render()
{
    renderShadowMapPass();

    auto& renderSystem = m_graphicsEngine->getRenderSystem();
    auto& deviceContext = renderSystem.getDeviceContext();
    auto& swapChain = m_display->getSwapChain();

    auto& sceneViewport = m_viewportManager->getViewport(ViewportType::Scene);
    auto& gameViewport = m_viewportManager->getViewport(ViewportType::Game);

    // --- REMOVED projection matrix calculation from here ---
    renderScene(*m_sceneCamera, sceneViewport.renderTexture.get());
    renderScene(m_gameCamera->getCamera(), gameViewport.renderTexture.get());
    // -----------------------------------------------------

    deviceContext.clearRenderTargetColor(swapChain, 0.204f, 0.204f, 0.204f, 1.0f);
    deviceContext.clearDepthBuffer(*m_depthBuffer);
    deviceContext.setRenderTargetsWithDepth(swapChain, *m_depthBuffer);
    deviceContext.setViewportSize(m_display->getSize().width, m_display->getSize().height);

    UIManager::SpawnCallbacks spawnCallbacks{
    [this]() { spawnCube(); },
    [this]() { spawnSphere(); },
    [this]() { spawnCapsule(); },
    [this]() { spawnCylinder(); },
    [this]() { spawnPlane(); },
    [this](const std::string& filename) { spawnModel(filename); },
    [this]() { spawnFiftyCubes(); },

    [this]() { spawnDirectionalLight(); },
    [this]() { spawnPointLight(); },
    [this]() { spawnSpotLight(); },
    [this]() { saveScene(); },
    [this](const std::string& filename) { loadScene(filename); }
    };

    m_uiManager->render(m_deltaTime, spawnCallbacks);

    //renderUI();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    deviceContext.present(swapChain);
}

void dx3d::Game::alignGameCameraWithView()
{
    if (m_gameCamera)
    {
        m_gameCamera->alignWithView(*m_sceneCamera);
        DX3DLogInfo("Game camera aligned with scene view");
    }
}

void dx3d::Game::spawnFiftyCubes()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posX(1.0f, 5.0f);
    std::uniform_real_distribution<float> posZ(1.0f, 5.0f);
    std::uniform_real_distribution<float> posY(13.0f, 15.0f);

    const int numCubes = 50;
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

            if (filename == "lucy.obj") {
                scale += Vector3(20.0f, 20.0f, 20.0f);
            }
            else if (filename == "bunnynew.obj") {
                scale += Vector3(10.0f, 10.0f, 10.0f);
            }

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

void dx3d::Game::spawnDirectionalLight()
{
    auto light = std::make_shared<DirectionalLight>();
    light->setPosition(Vector3(0, 5, 0));
    light->setRotation(Vector3(0.785f, 0.785f, 0.0f)); // ~45 degree angle

    m_gameObjects.push_back(light);
    m_lights.push_back(light);
    m_selectionSystem->setSelectedObject(light);
    DX3DLogInfo("Spawned Directional Light");
}

void dx3d::Game::spawnPointLight()
{
    auto light = std::make_shared<PointLight>();
    light->setPosition(Vector3(0, 3, 0));
    light->setRotation(Vector3(0.785f, 0.0f, 0.0f));

    m_gameObjects.push_back(light);
    m_lights.push_back(light);
    m_selectionSystem->setSelectedObject(light);
    DX3DLogInfo("Spawned Point Light");
}

void dx3d::Game::spawnSpotLight()
{
    auto light = std::make_shared<SpotLight>();
    light->setPosition(Vector3(0, 3, 0));
    light->setRotation(Vector3(-0.785f, 0.0f, 0.0f));

    m_gameObjects.push_back(light);
    m_lights.push_back(light);
    m_selectionSystem->setSelectedObject(light);
    DX3DLogInfo("Spawned Spot Light");
}

void dx3d::Game::setObjectTexture(std::shared_ptr<BaseGameObject> object, const std::string& textureFileName)
{
    if (!object)
    {
        DX3DLogError("Cannot set texture on null object");
        return;
    }

    object->setTexture(textureFileName);
}

std::shared_ptr<dx3d::Texture2D> dx3d::Game::loadTexture(const std::string& fileName)
{
    return ResourceManager::getInstance().loadTexture(fileName);
}

void dx3d::Game::setupMaterialForObject(std::shared_ptr<BaseGameObject> gameObject, DeviceContext& deviceContext)
{
    auto& componentManager = ComponentManager::getInstance();
    auto* materialComp = componentManager.getComponent<MaterialComponent>(gameObject->getEntity().getID());

    ModelMaterialConstants mmc;

    // Set default values first
    mmc.diffuseColor = Vector4(0.8f, 0.8f, 0.8f, 1.0f);
    mmc.ambientColor = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
    mmc.specularColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    mmc.emissiveColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
    mmc.specularPower = 32.0f;
    mmc.opacity = 1.0f;
    mmc.hasTexture = 0.0f;

    // If object has a material, use its properties
    if (materialComp && materialComp->material)
    {
        auto material = materialComp->material;

        mmc.diffuseColor = material->getDiffuseColor();
        mmc.ambientColor = material->getAmbientColor();
        mmc.specularColor = material->getSpecularColor();
        mmc.emissiveColor = material->getEmissiveColor();
        mmc.specularPower = material->getSpecularPower();
        mmc.opacity = material->getOpacity();

        // Handle texture
        if (material->hasDiffuseTexture())
        {
            auto texture = material->getDiffuseTexture();
            mmc.hasTexture = 1.0f;

            // Set texture in shader
            auto d3dContext = deviceContext.getDeviceContext();
            ID3D11ShaderResourceView* srv = texture->getShaderResourceView();
            ID3D11SamplerState* sampler = texture->getSamplerState();
            d3dContext->PSSetShaderResources(0, 1, &srv);
            d3dContext->PSSetSamplers(0, 1, &sampler);
        }
        else
        {
            mmc.hasTexture = 0.0f;

            // Clear texture binding
            auto d3dContext = deviceContext.getDeviceContext();
            ID3D11ShaderResourceView* nullSRV = nullptr;
            d3dContext->PSSetShaderResources(0, 1, &nullSRV);
        }
    }
    else
    {
        // Clear texture binding for objects without materials
        auto d3dContext = deviceContext.getDeviceContext();
        ID3D11ShaderResourceView* nullSRV = nullptr;
        d3dContext->PSSetShaderResources(0, 1, &nullSRV);
    }

    // Update the material constant buffer
    m_modelMaterialConstantBuffer->update(deviceContext, &mmc);
}


void dx3d::Game::clearTextureCache()
{
    ResourceManager::getInstance().clearTextureCache();
    DX3DLogInfo("Texture cache cleared");
}

std::vector<std::string> dx3d::Game::getLoadedTextures() const
{
    return ResourceManager::getInstance().getLoadedTextureNames();
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