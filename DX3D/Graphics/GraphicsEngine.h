
#pragma once
#include <../Core/Base.h>
#include <../Core/Base.h>

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

