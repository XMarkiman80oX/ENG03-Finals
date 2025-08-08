#include <../Game/Display.h>
#include <../Graphics/RenderSystem.h>

dx3d::Display::Display(const DisplayDesc& desc) : Window(desc.window)
{
    m_swapChain = desc.renderSystem.createSwapChain({ m_handle, m_size });
}

dx3d::SwapChain& dx3d::Display::getSwapChain() const
{
    return *m_swapChain;
}