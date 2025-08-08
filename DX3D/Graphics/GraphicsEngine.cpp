#include <../Graphics/GraphicsEngine.h>
#include <../Graphics/RenderSystem.h>
using namespace dx3d;

dx3d::GraphicsEngine::GraphicsEngine(const GraphicsEngineDesc& desc): Base(desc.base)
{
	m_renderSystem = std::make_shared<RenderSystem>(RenderSystemDesc{m_loggerInstance});
}

dx3d::GraphicsEngine::~GraphicsEngine()
{
}

RenderSystem& dx3d::GraphicsEngine::getRenderSystem() const noexcept
{
	return *m_renderSystem;
}
