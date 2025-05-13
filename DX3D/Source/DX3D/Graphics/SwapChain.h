#pragma once
#include <DX3D/Graphics/GraphicsResource.h>

namespace dx3d
{
    class SwapChain final : public GraphicsResource
    {
    public:
        SwapChain(const SwapChainDesc& desc, const GraphicsResourceDesc& gDesc);
        ~SwapChain();

        void present();
        ID3D11RenderTargetView* getRenderTargetView() const;
        ID3D11RenderTargetView** getRenderTargetViewAddress();

    private:
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView{};
        Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain{};
        void createRenderTargetView();
    };
}