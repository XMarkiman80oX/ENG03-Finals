#pragma once
#include "../Window/Window.h"
#include "../Graphics/SwapChain.h"

namespace dx3d
{
    class Display final : public Window
    {
    public:
        explicit Display(const DisplayDesc& desc);
        HWND getWindowHandle() const { return static_cast<HWND>(m_handle); }

        SwapChain& getSwapChain() const;

    private:
        SwapChainPtr m_swapChain{};
    };
}