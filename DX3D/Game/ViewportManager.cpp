#include <../Game/ViewportManager.h>
#include <../Graphics/RenderTexture.h>
#include <../Graphics/GraphicsEngine.h>
#include <../Graphics/RenderSystem.h>

using namespace dx3d;

ViewportManager::ViewportManager()
{
    m_sceneViewport.type = ViewportType::Scene;
    m_gameViewport.type = ViewportType::Game;
}

ViewportManager::~ViewportManager()
{
}

void ViewportManager::initialize(GraphicsEngine& graphicsEngine, ui32 width, ui32 height)
{
    auto& renderSystem = graphicsEngine.getRenderSystem();
    auto resourceDesc = renderSystem.getGraphicsResourceDesc();

    m_sceneViewport.renderTexture = std::make_shared<RenderTexture>(width, height, resourceDesc);
    m_sceneViewport.width = width;
    m_sceneViewport.height = height;

    m_gameViewport.renderTexture = std::make_shared<RenderTexture>(width, height, resourceDesc);
    m_gameViewport.width = width;
    m_gameViewport.height = height;

    m_initialized = true;
}

void ViewportManager::resize(ViewportType type, ui32 width, ui32 height)
{
    if (!m_initialized || width == 0 || height == 0)
        return;

    Viewport& viewport = (type == ViewportType::Scene) ? m_sceneViewport : m_gameViewport;
    viewport.width = width;
    viewport.height = height;
    viewport.renderTexture->resize(width, height);
}

Viewport& ViewportManager::getViewport(ViewportType type)
{
    return (type == ViewportType::Scene) ? m_sceneViewport : m_gameViewport;
}

const Viewport& ViewportManager::getViewport(ViewportType type) const
{
    return (type == ViewportType::Scene) ? m_sceneViewport : m_gameViewport;
}

void ViewportManager::updateViewportStates(ViewportType type, bool hovered, bool focused, float mouseX, float mouseY)
{
    Viewport& viewport = (type == ViewportType::Scene) ? m_sceneViewport : m_gameViewport;
    viewport.isHovered = hovered;
    viewport.isFocused = focused;
    viewport.mousePos.x = mouseX;
    viewport.mousePos.y = mouseY;
}