#pragma once
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/SwapChain.h>

namespace dx3d
{
    class Display final : public Window
    {
    public:
        explicit Display(const DisplayDesc& desc);

        SwapChain& getSwapChain() const;

    private:
        SwapChainPtr m_swapChain{};
    };
}