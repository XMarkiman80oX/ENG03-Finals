#pragma once
#include <../Core/Common.h>
#include <../Core/Base.h>
#include <../Graphics/GraphicsLogUtils.h>

#include <d3d11.h>
#include <wrl.h>

namespace dx3d
{
	struct GraphicsResourceDesc
	{
		BaseDesc base;
		std::shared_ptr<const RenderSystem> renderSystem;
		ID3D11Device& device;
		IDXGIFactory& factory;
	};

	class GraphicsResource : public Core
	{
	public:
		explicit GraphicsResource(const GraphicsResourceDesc& desc) :
			Core(desc.base),
			m_renderSystem(desc.renderSystem),
			m_device(desc.device),
			m_factory(desc.factory) {
		}

	protected:
		std::shared_ptr<const RenderSystem> m_renderSystem;
		ID3D11Device& m_device;
		IDXGIFactory& m_factory;
	};
}