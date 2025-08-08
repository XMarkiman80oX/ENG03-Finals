#include "../Core/Base.h"
using namespace dx3d;

dx3d::Base::Base(const BaseDesc& desc) : m_loggerInstance(desc.logger)
{
}

Logger& dx3d::Base::getLoggerInstance() const noexcept
{
	return m_loggerInstance;
}

dx3d::Base::~Base()
{
}