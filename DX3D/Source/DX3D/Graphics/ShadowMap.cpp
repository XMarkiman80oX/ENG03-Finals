#include <DX3D/Graphics/ShadowMap.h>
#include <DX3D/Graphics/DeviceContext.h>

dx3d::ShadowMap::ShadowMap(ui32 width, ui32 height, const GraphicsResourceDesc& desc)
    : GraphicsResource(desc), m_width(width), m_height(height)
{
    createResources(width, height);

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
}

dx3d::ShadowMap::~ShadowMap()
{
}

void dx3d::ShadowMap::createResources(ui32 width, ui32 height)
{
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    DX3DGraphicsLogErrorAndThrow(
        m_device.CreateTexture2D(&textureDesc, nullptr, &m_texture),
        "Failed to create shadow map texture"
    );

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    DX3DGraphicsLogErrorAndThrow(
        m_device.CreateDepthStencilView(m_texture.Get(), &dsvDesc, &m_depthStencilView),
        "Failed to create shadow map depth stencil view"
    );

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    DX3DGraphicsLogErrorAndThrow(
        m_device.CreateShaderResourceView(m_texture.Get(), &srvDesc, &m_shaderResourceView),
        "Failed to create shadow map shader resource view"
    );
}

void dx3d::ShadowMap::clear(DeviceContext& deviceContext)
{
    deviceContext.getDeviceContext()->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void dx3d::ShadowMap::setAsRenderTarget(DeviceContext& deviceContext)
{
    auto d3dContext = deviceContext.getDeviceContext();
    d3dContext->OMSetRenderTargets(0, nullptr, m_depthStencilView.Get());
    d3dContext->RSSetViewports(1, &m_viewport);
}