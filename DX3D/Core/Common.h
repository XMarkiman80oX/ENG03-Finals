#pragma once
#include "../Core/Forward.h"
#include "../Core/Logger.h"
#include "../Math/Rect.h"

namespace dx3d
{
	struct BaseDesc
	{
		Logger& logger;
	};

	struct WindowDesc
	{
		BaseDesc base;
		Rect size{};
	};

	struct DisplayDesc
	{
		WindowDesc window;
		RenderSystem& renderSystem;
	};

	struct GraphicsEngineDesc
	{
		BaseDesc base;
	};

	struct RenderSystemDesc
	{
		BaseDesc base;
	};

	struct SwapChainDesc
	{
		void* winHandle{};
		Rect winSize{};
	};


	struct GameDesc
	{
		Rect windowSize{ 2560,720 };
		Logger::LogLevel logLevel = Logger::LogLevel::Error;
	};
}