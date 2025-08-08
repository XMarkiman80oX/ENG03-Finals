#pragma once
#include "../Core/Forward.h"
#include "../Core/EventLog.h"
#include "../Math/Rect.h"

namespace dx3d
{
	struct BaseDesc
	{
		EventLog& logger;
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
		EventLog::LogStatus logLevel = EventLog::LogStatus::Error;
	};
}