#include "../Core/Base.h"
using namespace dx3d;

dx3d::Core::Core(const BaseDesc& desc) : m_loggerInstance(desc.logger)
{
}

Logger& dx3d::Core::getLoggerInstance() const noexcept
{
	return m_loggerInstance;
}

dx3d::Core::~Core()
{
}