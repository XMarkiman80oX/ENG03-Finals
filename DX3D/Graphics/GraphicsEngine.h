
#pragma once
#include <../Core/Core.h>
#include <../Core/Base.h>

namespace dx3d
{
	class GraphicsEngine final: public Base
	{
	public:
		explicit GraphicsEngine(const GraphicsEngineDesc& desc);
		virtual ~GraphicsEngine() override;


		RenderSystem& getRenderSystem() const noexcept;
	private:
		std::shared_ptr<RenderSystem> m_renderSystem{};
	};
}

