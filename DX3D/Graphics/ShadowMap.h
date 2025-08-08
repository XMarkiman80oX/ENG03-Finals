#pragma once
#include <../Graphics/GraphicsResource.h>

namespace dx3d
{
    class DeviceContext;

    class ShadowMap : public GraphicsResource
    {
    public:
        ShadowMap(ui32 width, ui32 height, const GraphicsResourceDesc& desc);
        ~ShadowMap();

        void clear(DeviceContext& deviceContext);
        void setAsRenderTarget(DeviceContext& deviceContext);

        ID3D11DepthStencilView* getDepthView() const { return m_depthStencilView.Get(); }
        ID3D11ShaderResourceView* getShaderResourceView() const { return m_shaderResourceView.Get(); }

    private:
        void createResources(ui32 width, ui32 height);

    private:
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
        D3D11_VIEWPORT m_viewport;
        ui32 m_width;
        ui32 m_height;
    };
}