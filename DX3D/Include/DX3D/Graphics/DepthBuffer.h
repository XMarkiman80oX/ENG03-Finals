#pragma once
#include <DX3D/Graphics/GraphicsResource.h>

namespace dx3d
{
    class DepthBuffer final : public GraphicsResource
    {
    public:
        DepthBuffer(ui32 width, ui32 height, const GraphicsResourceDesc& desc);
        ~DepthBuffer();

        ID3D11DepthStencilView* getDepthStencilView() const;
        void resize(ui32 width, ui32 height);

    private:
        void createDepthBuffer(ui32 width, ui32 height);
        void releaseResources();

    private:
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthTexture{};
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView{};
        ui32 m_width{};
        ui32 m_height{};
    };
}