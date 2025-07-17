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
#include <DX3D/Graphics/Primitives/Sphere.h>
#include <DX3D/Graphics/Primitives/Cylinder.h>
#include <DX3D/Graphics/Primitives/Capsule.h>
#include <DX3D/Graphics/Primitives/Model.h>
#include <DX3D/Graphics/Primitives/CameraObject.h>
#include <DX3D/Graphics/Primitives/CameraGizmo.h>
#include <DX3D/Graphics/Shaders/Rainbow3DShader.h>
#include <DX3D/Graphics/Shaders/WhiteShader.h>
#include <DX3D/Graphics/Shaders/FogShader.h>
#include <DX3D/Graphics/Shaders/ModelShader.h>
#include <DX3D/Graphics/Shaders/ModelVertexShader.h>
#include <DX3D/Math/Math.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Particles/ParticleSystem.h>
#include <DX3D/Particles/ParticleEffects/SnowParticle.h>
#include <DX3D/Game/ViewportManager.h>
#include <DX3D/Game/SelectionSystem.h>
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

	DX3DLogInfo("Game initialized with dual viewport system.");
}

dx3d::Game::~Game()
{
	DX3DLogInfo("Game deallocation started.");

	ParticleSystem::getInstance().shutdown();

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

	// Remove cube buffers, keep other primitives for potential use
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

	// Initialize snow configuration
	m_snowConfig.position = Vector3(0.0f, 10.0f, 0.0f);
	m_snowConfig.positionVariance = Vector3(20.0f, 0.0f, 20.0f);
	m_snowConfig.velocity = Vector3(0.0f, -2.0f, 0.0f);
	m_snowConfig.velocityVariance = Vector3(0.5f, 0.5f, 0.5f);
	m_snowConfig.acceleration = Vector3(0.0f, -0.5f, 0.0f);
	m_snowConfig.startColor = Vector4(1.0f, 1.0f, 1.0f, 0.8f);
	m_snowConfig.endColor = Vector4(0.9f, 0.9f, 1.0f, 0.0f);
	m_snowConfig.startSize = 0.2f;
	m_snowConfig.endSize = 0.1f;
	m_snowConfig.lifetime = 8.0f;
	m_snowConfig.lifetimeVariance = 2.0f;
	m_snowConfig.emissionRate = 50.0f;
	m_snowConfig.active = true;

	// Clear game objects and create the three models
	m_gameObjects.clear();
	m_gameObjects.reserve(5); // Plane + 3 models + camera

	// Load and create the three models
	try
	{
		// 1. Teapot with brick texture and scale 5.0
		auto teapotModel = Model::LoadFromFile("teapot.obj", resourceDesc);
		if (teapotModel && teapotModel->isReadyForRendering())
		{
			teapotModel->setPosition(Vector3(-8.0f, 3.0f, 0.0f)); // Left side of plane
			teapotModel->setScale(Vector3(5.0f, 5.0f, 5.0f));
			teapotModel->setRotation(Vector3(0.0f, 0.0f, 0.0f));
			teapotModel->setName("Teapot");

			// Try to load brick texture for the teapot
			try
			{
				auto brickTexture = std::make_shared<Texture2D>("DX3D/Assets/Textures/brick.png", resourceDesc);
				if (teapotModel->getMeshCount() > 0)
				{
					auto mesh = teapotModel->getMesh(0);
					if (mesh && mesh->getMaterial())
					{
						mesh->getMaterial()->setDiffuseTexture(brickTexture);
						mesh->getMaterial()->setDiffuseColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f)); // White to show texture properly
					}
				}
				DX3DLogInfo("Brick texture loaded for teapot");
			}
			catch (const std::exception& e)
			{
				DX3DLogError(("Failed to load brick texture: " + std::string(e.what())).c_str());
				// Set a brown color as fallback
				if (teapotModel->getMeshCount() > 0)
				{
					auto mesh = teapotModel->getMesh(0);
					if (mesh && mesh->getMaterial())
					{
						mesh->getMaterial()->setDiffuseColor(Vector4(0.8f, 0.4f, 0.2f, 1.0f)); // Brown color
					}
				}
			}

			m_gameObjects.push_back(teapotModel);
			DX3DLogInfo("Teapot model loaded successfully!");
		}
		else
		{
			DX3DLogError("Failed to load teapot model");
		}

		// 2. Bunny with scale 10.0
		auto bunnyModel = Model::LoadFromFile("bunnynew.obj", resourceDesc);
		if (bunnyModel && bunnyModel->isReadyForRendering())
		{
			bunnyModel->setPosition(Vector3(0.0f, 1.0f, 0.0f)); // Center of plane
			bunnyModel->setScale(Vector3(10.0f, 10.0f, 10.0f));
			bunnyModel->setRotation(Vector3(0.0f, 0.0f, 0.0f));
			bunnyModel->setName("Bunny");

			// Set a nice color for the bunny
			if (bunnyModel->getMeshCount() > 0)
			{
				auto mesh = bunnyModel->getMesh(0);
				if (mesh && mesh->getMaterial())
				{
					mesh->getMaterial()->setDiffuseColor(Vector4(0.9f, 0.9f, 0.9f, 1.0f)); // Light gray
				}
			}

			m_gameObjects.push_back(bunnyModel);
			DX3DLogInfo("Bunny model loaded successfully!");
		}
		else
		{
			DX3DLogError("Failed to load bunny model");
		}

		// 3. Armadillo with scale 0.01 - let it use its MTL file
		auto armadilloModel = Model::LoadFromFile("armadillo.obj", resourceDesc);
		if (armadilloModel && armadilloModel->isReadyForRendering())
		{
			armadilloModel->setPosition(Vector3(8.0f, 2.0f, 0.0f)); // Right side of plane
			armadilloModel->setScale(Vector3(0.01f, 0.01f, 0.01f));
			armadilloModel->setRotation(Vector3(0.0f, 0.0f, 0.0f));
			armadilloModel->setName("Armadillo");

			// Don't override material - let it use the MTL file materials
			// The enhanced ModelLoader will load the armadillo.mtl file automatically

			m_gameObjects.push_back(armadilloModel);
			DX3DLogInfo("Armadillo model loaded successfully!");
		}
		else
		{
			DX3DLogError("Failed to load armadillo model");
		}
	}
	catch (const std::exception& e)
	{
		DX3DLogError(("Failed to load one or more models: " + std::string(e.what())).c_str());
	}

	// Add the plane (ground)
	m_gameObjects.push_back(std::make_shared<Plane>(
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(-1.5708f, 0.0f, 0.0f), // 90 degrees rotation to make it horizontal
		Vector3(15.0f, 15.0f, 1.0f)
	));

	// Add game camera
	m_gameCamera = std::make_shared<CameraObject>(
		Vector3(12.0f, 8.0f, -12.0f),
		Vector3(0.0f, 0.0f, 0.0f)
	);
	m_gameCamera->getCamera().lookAt(Vector3(0.0f, 2.0f, 0.0f));
	m_gameObjects.push_back(m_gameCamera);

	// Set up scene camera
	m_sceneCamera = std::make_unique<Camera>(
		Vector3(15.0f, 10.0f, -15.0f),
		Vector3(0.0f, 2.0f, 0.0f)
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

	// Initialize particle system
	ParticleSystem::getInstance().initialize(*m_graphicsEngine);

	ParticleEmitter::EmitterConfig snowConfig;
	snowConfig.position = m_snowConfig.position;
	snowConfig.positionVariance = m_snowConfig.positionVariance;
	snowConfig.velocity = m_snowConfig.velocity;
	snowConfig.velocityVariance = m_snowConfig.velocityVariance;
	snowConfig.acceleration = m_snowConfig.acceleration;
	snowConfig.startColor = m_snowConfig.startColor;
	snowConfig.endColor = m_snowConfig.endColor;
	snowConfig.startSize = m_snowConfig.startSize;
	snowConfig.endSize = m_snowConfig.endSize;
	snowConfig.lifetime = m_snowConfig.lifetime;
	snowConfig.lifetimeVariance = m_snowConfig.lifetimeVariance;
	snowConfig.emissionRate = m_snowConfig.emissionRate;
	snowConfig.maxParticles = 2000;

	auto snowEmitter = ParticleSystem::getInstance().createEmitter(
		"snow",
		snowConfig,
		createSnowParticle
	);

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

	// Delete selected object with Delete key
	if (input.isKeyJustPressed(KeyCode::Delete))
	{
		auto selectedObject = m_selectionSystem->getSelectedObject();
		if (selectedObject)
		{
			// Find and remove the selected object
			auto it = std::find(m_gameObjects.begin(), m_gameObjects.end(), selectedObject);
			if (it != m_gameObjects.end())
			{
				m_gameObjects.erase(it);
				m_selectionSystem->setSelectedObject(nullptr);
			}
		}
	}

	// Duplicate selected object with Ctrl+D
	if (input.isKeyPressed(KeyCode::Control) && input.isKeyJustPressed(KeyCode::D))
	{
		auto selectedObject = m_selectionSystem->getSelectedObject();
		if (selectedObject)
		{
			auto newObject = createObjectCopy(selectedObject);
			if (newObject)
			{
				Vector3 pos = selectedObject->getPosition();
				pos.x += 1.0f; // Offset the duplicate
				newObject->setPosition(pos);
				m_gameObjects.push_back(newObject);
				m_selectionSystem->setSelectedObject(newObject); // Select the new object
			}
		}
	}

	// Reset scene camera
	if (input.isKeyJustPressed(KeyCode::R))
	{
		m_sceneCamera->setPosition(Vector3(15.0f, 10.0f, -15.0f));
		m_sceneCamera->lookAt(Vector3(0.0f, 2.0f, 0.0f));
	}

	// Exit application
	if (input.isKeyPressed(KeyCode::Escape))
	{
		m_isRunning = false;
	}
}

void dx3d::Game::debugRenderInfo()
{
	printf("=== DEBUG RENDER INFO ===\n");
	printf("Camera position: (%.2f, %.2f, %.2f)\n",
		m_sceneCamera->getPosition().x,
		m_sceneCamera->getPosition().y,
		m_sceneCamera->getPosition().z);

	printf("Number of game objects: %zu\n", m_gameObjects.size());

	for (size_t i = 0; i < m_gameObjects.size(); ++i)
	{
		auto& obj = m_gameObjects[i];
		printf("Object %zu position: (%.2f, %.2f, %.2f)\n",
			i, obj->getPosition().x, obj->getPosition().y, obj->getPosition().z);
	}

	printf("Fog enabled: %s\n", m_fogDesc.enabled ? "true" : "false");
	printf("Fog color: (%.2f, %.2f, %.2f, %.2f)\n",
		m_fogDesc.color.x, m_fogDesc.color.y, m_fogDesc.color.z, m_fogDesc.color.w);

	auto& sceneViewport = m_viewportManager->getViewport(ViewportType::Scene);
	printf("Scene viewport size: %dx%d\n", sceneViewport.width, sceneViewport.height);

	auto& gameViewport = m_viewportManager->getViewport(ViewportType::Game);
	printf("Game viewport size: %dx%d\n", gameViewport.width, gameViewport.height);

	printf("==========================\n");
}

void dx3d::Game::renderScene(Camera& camera, const Matrix4x4& projMatrix, RenderTexture* renderTarget)
{
	auto& renderSystem = m_graphicsEngine->getRenderSystem();
	auto& deviceContext = renderSystem.getDeviceContext();
	auto d3dContext = deviceContext.getDeviceContext();

	if (renderTarget)
	{
		// Clear with a neutral color for debugging
		renderTarget->clear(deviceContext, 0.2f, 0.2f, 0.2f, 1.0f); // Dark gray background
		renderTarget->setAsRenderTarget(deviceContext);
	}
	else
	{
		auto& swapChain = m_display->getSwapChain();
		deviceContext.clearRenderTargetColor(swapChain, 0.2f, 0.2f, 0.2f, 1.0f); // Dark gray background
		deviceContext.clearDepthBuffer(*m_depthBuffer);
		deviceContext.setRenderTargetsWithDepth(swapChain, *m_depthBuffer);
	}

	ui32 viewportWidth = renderTarget ? (renderTarget->getShaderResourceView() ? 640 : m_display->getSize().width) : m_display->getSize().width;
	ui32 viewportHeight = renderTarget ? (renderTarget->getShaderResourceView() ? 480 : m_display->getSize().height) : m_display->getSize().height;

	deviceContext.setViewportSize(viewportWidth, viewportHeight);

	// Set up common constant buffers
	ID3D11Buffer* transformCb = m_transformConstantBuffer->getBuffer();
	d3dContext->VSSetConstantBuffers(0, 1, &transformCb);

	d3dContext->OMSetDepthStencilState(m_solidDepthState, 0);

	bool isSceneView = (&camera == m_sceneCamera.get());
	int objectsRendered = 0;

	for (const auto& gameObject : m_gameObjects)
	{
		bool isCamera = std::dynamic_pointer_cast<CameraObject>(gameObject) != nullptr;
		if (!isSceneView && isCamera)
			continue;

		// Handle Model objects
		if (auto model = std::dynamic_pointer_cast<Model>(gameObject))
		{
			// Set model shaders
			deviceContext.setVertexShader(m_modelVertexShader->getShader());
			deviceContext.setPixelShader(m_modelPixelShader->getShader());
			deviceContext.setInputLayout(m_modelVertexShader->getInputLayout());

			// Set up transformation matrices
			TransformationMatrices transformMatrices;
			transformMatrices.world = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(gameObject->getWorldMatrix().toXMMatrix()));
			transformMatrices.view = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(camera.getViewMatrix().toXMMatrix()));
			transformMatrices.projection = Matrix4x4::fromXMMatrix(DirectX::XMMatrixTranspose(projMatrix.toXMMatrix()));
			m_transformConstantBuffer->update(deviceContext, &transformMatrices);

			// Render each mesh in the model
			for (size_t i = 0; i < model->getMeshCount(); ++i)
			{
				auto mesh = model->getMesh(i);
				if (mesh && mesh->isReadyForRendering())
				{
					// Set material constants
					ModelMaterialConstants materialConstants = {};
					if (mesh->getMaterial())
					{
						materialConstants.diffuseColor = mesh->getMaterial()->getDiffuseColor();
						materialConstants.ambientColor = mesh->getMaterial()->getAmbientColor();
						materialConstants.specularColor = mesh->getMaterial()->getSpecularColor();
						materialConstants.emissiveColor = mesh->getMaterial()->getEmissiveColor();
						materialConstants.specularPower = mesh->getMaterial()->getSpecularPower();
						materialConstants.opacity = mesh->getMaterial()->getOpacity();
						materialConstants.hasTexture = mesh->getMaterial()->hasDiffuseTexture();
					}
					else
					{
						// Default material
						materialConstants.diffuseColor = Vector4(0.7f, 0.7f, 0.7f, 1.0f);
						materialConstants.ambientColor = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
						materialConstants.specularColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
						materialConstants.emissiveColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
						materialConstants.specularPower = 32.0f;
						materialConstants.opacity = 1.0f;
						materialConstants.hasTexture = false;
					}

					m_modelMaterialConstantBuffer->update(deviceContext, &materialConstants);
					ID3D11Buffer* materialCb = m_modelMaterialConstantBuffer->getBuffer();
					d3dContext->PSSetConstantBuffers(1, 1, &materialCb);

					// Bind texture if available
					if (mesh->getMaterial() && mesh->getMaterial()->hasDiffuseTexture())
					{
						auto texture = mesh->getMaterial()->getDiffuseTexture();
						ID3D11ShaderResourceView* srv = texture->getShaderResourceView();
						ID3D11SamplerState* sampler = texture->getSamplerState();
						d3dContext->PSSetShaderResources(0, 1, &srv);
						d3dContext->PSSetSamplers(0, 1, &sampler);
					}
					else
					{
						// Clear texture binding if no texture
						ID3D11ShaderResourceView* nullSRV = nullptr;
						d3dContext->PSSetShaderResources(0, 1, &nullSRV);
					}

					// Set vertex and index buffers
					deviceContext.setVertexBuffer(*mesh->getVertexBuffer());
					deviceContext.setIndexBuffer(*mesh->getIndexBuffer());

					// Draw the mesh
					deviceContext.drawIndexed(mesh->getIndexCount(), 0, 0);
					objectsRendered++;
				}
			}
			continue; // Move to next game object
		}

		// Handle primitive objects (cubes, spheres, etc.)
		// Use fog shader for primitives
		deviceContext.setVertexShader(m_fogVertexShader->getShader());
		deviceContext.setPixelShader(m_fogPixelShader->getShader());
		deviceContext.setInputLayout(m_fogVertexShader->getInputLayout());

		FogShaderConstants fsc = {};
		fsc.fogColor = Vector4(0.2f, 0.3f, 0.4f, 1.0f);
		fsc.cameraPosition = camera.getPosition();
		fsc.fogStart = 5.0f;
		fsc.fogEnd = 25.0f;
		fsc.fogEnabled = false; // Disable fog for debugging
		m_fogConstantBuffer->update(deviceContext, &fsc);

		ID3D11Buffer* fogCb = m_fogConstantBuffer->getBuffer();
		d3dContext->PSSetConstantBuffers(1, 1, &fogCb);

		FogMaterialConstants fmc = {};
		fmc.useVertexColor = true; // Use vertex colors for primitives
		fmc.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // White base color
		m_materialConstantBuffer->update(deviceContext, &fmc);
		ID3D11Buffer* materialCb = m_materialConstantBuffer->getBuffer();
		d3dContext->PSSetConstantBuffers(2, 1, &materialCb);

		// Set vertex and index buffers
		bool bufferSet = false;
		ui32 indexCount = 0;

		if (auto sphere = std::dynamic_pointer_cast<Sphere>(gameObject)) {
			deviceContext.setVertexBuffer(*m_sphereVertexBuffer);
			deviceContext.setIndexBuffer(*m_sphereIndexBuffer);
			indexCount = Sphere::GetIndexCount();
			bufferSet = true;
		}
		else if (auto plane = std::dynamic_pointer_cast<Plane>(gameObject)) {
			deviceContext.setVertexBuffer(*m_planeVertexBuffer);
			deviceContext.setIndexBuffer(*m_planeIndexBuffer);
			indexCount = Plane::GetIndexCount();
			bufferSet = true;
		}
		else if (auto cylinder = std::dynamic_pointer_cast<Cylinder>(gameObject)) {
			deviceContext.setVertexBuffer(*m_cylinderVertexBuffer);
			deviceContext.setIndexBuffer(*m_cylinderIndexBuffer);
			indexCount = Cylinder::GetIndexCount();
			bufferSet = true;
		}
		else if (auto capsule = std::dynamic_pointer_cast<Capsule>(gameObject)) {
			deviceContext.setVertexBuffer(*m_capsuleVertexBuffer);
			deviceContext.setIndexBuffer(*m_capsuleIndexBuffer);
			indexCount = Capsule::GetIndexCount();
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

			// Draw the primitive
			deviceContext.drawIndexed(indexCount, 0, 0);
			objectsRendered++;
		}
	}
}

void dx3d::Game::update()
{
	auto currentTime = std::chrono::steady_clock::now();
	m_deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_previousTime).count() / 1000000.0f;
	m_previousTime = currentTime;

	// Add debug info (call only once)
	static bool debugPrinted = false;
	if (!debugPrinted)
	{
		debugRenderInfo();
		debugPrinted = true;
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	processInput(m_deltaTime);
	m_sceneCamera->update();

	for (auto& gameObject : m_gameObjects)
	{
		gameObject->update(m_deltaTime);
	}

	ParticleSystem::getInstance().update(m_deltaTime);

	if (auto snowEmitter = ParticleSystem::getInstance().getEmitter("snow"))
	{
		snowEmitter->setPosition(m_snowConfig.position);
	}
}

void dx3d::Game::updateSnowEmitter()
{
	auto snowEmitter = ParticleSystem::getInstance().getEmitter("snow");
	if (!snowEmitter)
		return;

	if (m_snowConfig.active)
	{
		snowEmitter->start();
	}
	else
	{
		snowEmitter->stop();
		return;
	}

	snowEmitter->setEmissionRate(m_snowConfig.emissionRate);

	ParticleEmitter::EmitterConfig newConfig;
	newConfig.position = m_snowConfig.position;
	newConfig.positionVariance = m_snowConfig.positionVariance;
	newConfig.velocity = m_snowConfig.velocity;
	newConfig.velocityVariance = m_snowConfig.velocityVariance;
	newConfig.acceleration = m_snowConfig.acceleration;
	newConfig.startColor = m_snowConfig.startColor;
	newConfig.endColor = m_snowConfig.endColor;
	newConfig.startSize = m_snowConfig.startSize;
	newConfig.endSize = m_snowConfig.endSize;
	newConfig.lifetime = m_snowConfig.lifetime;
	newConfig.lifetimeVariance = m_snowConfig.lifetimeVariance;
	newConfig.emissionRate = m_snowConfig.emissionRate;
	newConfig.maxParticles = 2000;
	newConfig.loop = true;

	ParticleSystem::getInstance().removeEmitter("snow");
	ParticleSystem::getInstance().createEmitter("snow", newConfig, createSnowParticle);
}

void dx3d::Game::renderUI()
{
	ImGuiIO& io = ImGui::GetIO();
	float windowWidth = io.DisplaySize.x;   // Should be 1280
	float windowHeight = io.DisplaySize.y;  // Should be 720

	float halfWidth = windowWidth * 0.5f;   // 640px each side
	float halfHeight = windowHeight * 0.5f; // 360px each half

	// =============================================================================
	// LEFT SIDE - Game View (Top) and Scene View (Bottom)
	// =============================================================================

	// Game View - Top Left
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight));
	ImGui::Begin("Game View", nullptr,
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImVec2 gameViewportSize = ImGui::GetContentRegionAvail();
	if (gameViewportSize.x > 0 && gameViewportSize.y > 0)
	{
		m_viewportManager->resize(ViewportType::Game,
			static_cast<ui32>(gameViewportSize.x),
			static_cast<ui32>(gameViewportSize.y));

		auto& gameViewport = m_viewportManager->getViewport(ViewportType::Game);
		ImGui::Image((void*)gameViewport.renderTexture->getShaderResourceView(), gameViewportSize);
	}
	ImGui::End();

	// Scene View - Bottom Left  
	ImGui::SetNextWindowPos(ImVec2(0, halfHeight));
	ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight));
	ImGui::Begin("Scene View", nullptr,
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImVec2 sceneViewportSize = ImGui::GetContentRegionAvail();
	if (sceneViewportSize.x > 0 && sceneViewportSize.y > 0)
	{
		m_viewportManager->resize(ViewportType::Scene,
			static_cast<ui32>(sceneViewportSize.x),
			static_cast<ui32>(sceneViewportSize.y));

		auto& sceneViewport = m_viewportManager->getViewport(ViewportType::Scene);
		ImGui::Image((void*)sceneViewport.renderTexture->getShaderResourceView(), sceneViewportSize);

		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 windowPos = ImGui::GetWindowPos();
		float localX = mousePos.x - windowPos.x - 8;
		float localY = mousePos.y - windowPos.y - ImGui::GetFrameHeight() - 4;

		m_viewportManager->updateViewportStates(
			ViewportType::Scene,
			ImGui::IsWindowHovered(),
			ImGui::IsWindowFocused(),
			localX,
			localY
		);
	}
	ImGui::End();

	// =============================================================================
	// RIGHT SIDE - Scene Hierarchy (Top) and Inspector (Bottom)  
	// =============================================================================

	// Scene Hierarchy - Top Right
	ImGui::SetNextWindowPos(ImVec2(halfWidth, 0));
	ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight));
	ImGui::Begin("Scene Hierarchy", nullptr,
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

	renderSceneHierarchy();

	ImGui::End();

	// Inspector - Bottom Right
	ImGui::SetNextWindowPos(ImVec2(halfWidth, halfHeight));
	ImGui::SetNextWindowSize(ImVec2(halfWidth, halfHeight));
	ImGui::Begin("Inspector", nullptr,
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

	renderInspector();

	ImGui::End();
}

void dx3d::Game::renderSceneHierarchy()
{
	ImGui::Text("Scene Objects (%zu)", m_gameObjects.size());
	ImGui::Separator();

	// Create a scrollable region for the hierarchy
	if (ImGui::BeginChild("HierarchyScroll", ImVec2(0, -60), true))
	{
		auto selectedObject = m_selectionSystem->getSelectedObject();
		int objectToDelete = -1; // Track which object to delete

		for (size_t i = 0; i < m_gameObjects.size(); ++i)
		{
			auto& gameObject = m_gameObjects[i];

			// Generate a display name for the object
			std::string objectName = getObjectDisplayName(gameObject, i);

			// Check if this object is currently selected
			bool isSelected = (selectedObject == gameObject);

			// Add icon based on object type
			std::string icon = getObjectIcon(gameObject);
			std::string displayText = icon + " " + objectName;

			// Create selectable item with highlighting for selection
			if (ImGui::Selectable(displayText.c_str(), isSelected))
			{
				m_selectionSystem->setSelectedObject(gameObject);
			}

			// Right-click context menu
			if (ImGui::BeginPopupContextItem())
			{
				ImGui::Text("Object: %s", objectName.c_str());
				ImGui::Separator();

				if (ImGui::MenuItem("Focus in Scene View"))
				{
					// Center scene camera on this object
					Vector3 objectPos = gameObject->getPosition();
					Vector3 offset = Vector3(5.0f, 5.0f, -5.0f);
					m_sceneCamera->setPosition(objectPos + offset);
					m_sceneCamera->lookAt(objectPos);
				}

				if (ImGui::MenuItem("Duplicate"))
				{
					// Create a copy of the object with slight offset
					auto newObject = createObjectCopy(gameObject);
					if (newObject)
					{
						Vector3 pos = gameObject->getPosition();
						pos.x += 1.0f; // Offset the duplicate
						newObject->setPosition(pos);
						m_gameObjects.push_back(newObject);
					}
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Delete", "Del"))
				{
					objectToDelete = static_cast<int>(i);
				}

				ImGui::EndPopup();
			}

			// Show object position and other info as secondary text
			Vector3 pos = gameObject->getPosition();
			ImGui::SameLine();
			ImGui::TextDisabled("(%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
		}

		// Handle deletion outside the loop to avoid iterator invalidation
		if (objectToDelete >= 0)
		{
			// If we're deleting the selected object, clear selection
			if (selectedObject == m_gameObjects[objectToDelete])
			{
				m_selectionSystem->setSelectedObject(nullptr);
			}

			m_gameObjects.erase(m_gameObjects.begin() + objectToDelete);
		}
	}
	ImGui::EndChild();

	// Add object creation buttons
	ImGui::Separator();
	ImGui::Text("Create Objects:");

	if (ImGui::Button("Cube"))
	{
		auto cube = std::make_shared<Cube>(Vector3(0, 1, 0));
		m_gameObjects.push_back(cube);
		m_selectionSystem->setSelectedObject(cube);
	}
	ImGui::SameLine();

	if (ImGui::Button("Sphere"))
	{
		auto sphere = std::make_shared<Sphere>(Vector3(2, 1, 0));
		m_gameObjects.push_back(sphere);
		m_selectionSystem->setSelectedObject(sphere);
	}
	ImGui::SameLine();

	if (ImGui::Button("Plane"))
	{
		auto plane = std::make_shared<Plane>(Vector3(0, 0, 2));
		m_gameObjects.push_back(plane);
		m_selectionSystem->setSelectedObject(plane);
	}

	if (ImGui::Button("Cylinder"))
	{
		auto cylinder = std::make_shared<Cylinder>(Vector3(-2, 1, 0));
		m_gameObjects.push_back(cylinder);
		m_selectionSystem->setSelectedObject(cylinder);
	}
	ImGui::SameLine();

	if (ImGui::Button("Capsule"))
	{
		auto capsule = std::make_shared<Capsule>(Vector3(0, 1, -2));
		m_gameObjects.push_back(capsule);
		m_selectionSystem->setSelectedObject(capsule);
	}
}

void dx3d::Game::renderInspector()
{
	auto selectedObject = m_selectionSystem->getSelectedObject();

	if (selectedObject)
	{
		// Object Inspector Section
		ImGui::Text("Selected Object");
		ImGui::Separator();

		std::string objectName = getObjectDisplayName(selectedObject, -1);
		ImGui::Text("Type: %s", objectName.c_str());

		ImGui::Spacing();

		// Transform manipulation
		ImGui::Text("Transform");
		ImGui::Separator();

		Vector3 pos = selectedObject->getPosition();
		if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
		{
			selectedObject->setPosition(pos);
		}

		Vector3 rot = selectedObject->getRotation();
		Vector3 rotDeg = Vector3(
			rot.x * 180.0f / 3.14159265f,
			rot.y * 180.0f / 3.14159265f,
			rot.z * 180.0f / 3.14159265f
		);
		if (ImGui::DragFloat3("Rotation", &rotDeg.x, 1.0f))
		{
			Vector3 newRotRad = Vector3(
				rotDeg.x * 3.14159265f / 180.0f,
				rotDeg.y * 3.14159265f / 180.0f,
				rotDeg.z * 3.14159265f / 180.0f
			);
			selectedObject->setRotation(newRotRad);
		}

		Vector3 scale = selectedObject->getScale();
		// FIXED: Remove scale limits - allow unlimited scaling
		if (ImGui::DragFloat3("Scale", &scale.x, 0.01f))
		{
			selectedObject->setScale(scale);
		}

		// Object-specific properties
		if (auto camera = std::dynamic_pointer_cast<CameraObject>(selectedObject))
		{
			ImGui::Spacing();
			ImGui::Text("Camera Settings");
			ImGui::Separator();

			float fov = camera->getFOV() * 180.0f / 3.14159265f;
			if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f))
			{
				camera->setFOV(fov * 3.14159265f / 180.0f);
			}

			float nearPlane = camera->getNearPlane();
			if (ImGui::DragFloat("Near", &nearPlane, 0.01f, 0.01f, 10.0f))
			{
				camera->setNearPlane(nearPlane);
			}

			float farPlane = camera->getFarPlane();
			if (ImGui::DragFloat("Far", &farPlane, 1.0f, 10.0f, 1000.0f))
			{
				camera->setFarPlane(farPlane);
			}

			if (ImGui::Button("Align with Scene View"))
			{
				camera->alignWithView(*m_sceneCamera);
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
	}
	else
	{
		ImGui::Text("No object selected");
		ImGui::Text("Select an object in the Scene Hierarchy");
		ImGui::Separator();
	}

	// Effects Settings Section
	ImGui::Text("Effects Settings");
	ImGui::Separator();

	// Fog Settings
	ImGui::Text("Fog");
	ImGui::Checkbox("Enable Fog", &m_fogDesc.enabled);
	ImGui::SliderFloat("Fog Start", &m_fogDesc.start, 0.1f, 50.0f);
	ImGui::SliderFloat("Fog End", &m_fogDesc.end, 1.0f, 100.0f);
	ImGui::ColorEdit3("Fog Color", &m_fogDesc.color.x);

	ImGui::Spacing();

	// Snow Particle Settings
	ImGui::Text("Snow Particles");
	bool snowSettingsChanged = false;

	if (ImGui::Checkbox("Active##Snow", &m_snowConfig.active))
		snowSettingsChanged = true;

	if (ImGui::DragFloat3("Position##Snow", &m_snowConfig.position.x, 0.5f))
		snowSettingsChanged = true;

	if (ImGui::ColorEdit4("Start Color##Snow", &m_snowConfig.startColor.x))
		snowSettingsChanged = true;

	if (ImGui::SliderFloat("Emission Rate##Snow", &m_snowConfig.emissionRate, 1.0f, 200.0f))
		snowSettingsChanged = true;

	if (snowSettingsChanged)
	{
		updateSnowEmitter();
	}

	ImGui::Spacing();

	// Camera Controls Section
	ImGui::Text("Scene Camera Controls");
	ImGui::Separator();

	ImGui::Text("Speed: %.1f", m_cameraSpeed);
	ImGui::SliderFloat("##CameraSpeed", &m_cameraSpeed, 1.0f, 20.0f);

	ImGui::Text("Mouse Sens: %.2f", m_mouseSensitivity);
	ImGui::SliderFloat("##MouseSens", &m_mouseSensitivity, 0.1f, 2.0f);

	if (ImGui::Button("Reset Scene Camera"))
	{
		m_sceneCamera->setPosition(Vector3(15.0f, 10.0f, -15.0f));
		m_sceneCamera->lookAt(Vector3(0.0f, 2.0f, 0.0f));
	}
}

std::string dx3d::Game::getObjectIcon(std::shared_ptr<AGameObject> object)
{
	if (!object) return "[?]";

	if (std::dynamic_pointer_cast<Model>(object))
		return "[M]";
	else if (std::dynamic_pointer_cast<CameraObject>(object))
		return "[C]";
	else if (std::dynamic_pointer_cast<Cube>(object))
		return "[■]";
	else if (std::dynamic_pointer_cast<Sphere>(object))
		return "[●]";
	else if (std::dynamic_pointer_cast<Plane>(object))
		return "[▬]";
	else if (std::dynamic_pointer_cast<Cylinder>(object))
		return "[⬢]";
	else if (std::dynamic_pointer_cast<Capsule>(object))
		return "[◉]";

	return "[O]";
}

std::shared_ptr<dx3d::AGameObject> dx3d::Game::createObjectCopy(std::shared_ptr<AGameObject> original)
{
	if (!original) return nullptr;

	Vector3 pos = original->getPosition();
	Vector3 rot = original->getRotation();
	Vector3 scale = original->getScale();

	if (auto cube = std::dynamic_pointer_cast<Cube>(original))
		return std::make_shared<Cube>(pos, rot, scale);
	else if (auto sphere = std::dynamic_pointer_cast<Sphere>(original))
		return std::make_shared<Sphere>(pos, rot, scale);
	else if (auto plane = std::dynamic_pointer_cast<Plane>(original))
		return std::make_shared<Plane>(pos, rot, scale);
	else if (auto cylinder = std::dynamic_pointer_cast<Cylinder>(original))
		return std::make_shared<Cylinder>(pos, rot, scale);
	else if (auto capsule = std::dynamic_pointer_cast<Capsule>(original))
		return std::make_shared<Capsule>(pos, rot, scale);
	else if (auto camera = std::dynamic_pointer_cast<CameraObject>(original))
		return std::make_shared<CameraObject>(pos, rot);

	// For models and other complex objects, we'd need more sophisticated copying
	return nullptr;
}

std::string dx3d::Game::getObjectDisplayName(std::shared_ptr<AGameObject> object, int index)
{
	if (!object) return "Null Object";

	// Try to identify the object type
	if (auto model = std::dynamic_pointer_cast<Model>(object))
	{
		if (!model->getName().empty())
			return model->getName();
		return "Model";
	}
	else if (auto camera = std::dynamic_pointer_cast<CameraObject>(object))
	{
		return "Camera";
	}
	else if (auto cube = std::dynamic_pointer_cast<Cube>(object))
	{
		return "Cube";
	}
	else if (auto sphere = std::dynamic_pointer_cast<Sphere>(object))
	{
		return "Sphere";
	}
	else if (auto plane = std::dynamic_pointer_cast<Plane>(object))
	{
		return "Plane";
	}
	else if (auto cylinder = std::dynamic_pointer_cast<Cylinder>(object))
	{
		return "Cylinder";
	}
	else if (auto capsule = std::dynamic_pointer_cast<Capsule>(object))
	{
		return "Capsule";
	}

	// Fallback to generic name with index
	if (index >= 0)
		return "GameObject_" + std::to_string(index);

	return "GameObject";
}

void dx3d::Game::render()
{
	auto& renderSystem = m_graphicsEngine->getRenderSystem();
	auto& deviceContext = renderSystem.getDeviceContext();
	auto& swapChain = m_display->getSwapChain();
	auto d3dContext = deviceContext.getDeviceContext();

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