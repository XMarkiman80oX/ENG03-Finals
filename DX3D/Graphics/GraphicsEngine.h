
#pragma once
#include <../Core/Forward.h>
#include <../Core/Core.h>

namespace dx3d
{
	class GraphicsEngine final: public Core
	{
	public:
		explicit GraphicsEngine(const GraphicsEngineDesc& desc);
		virtual ~GraphicsEngine() override;


		RenderSystem& getRenderSystem() const noexcept;
	private:
		std::shared_ptr<RenderSystem> m_renderSystem{};
	};
}

