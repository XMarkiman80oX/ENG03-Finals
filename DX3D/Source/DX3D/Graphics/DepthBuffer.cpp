#include <DX3D/Graphics/DepthBuffer.h>

dx3d::DepthBuffer::DepthBuffer(ui32 width, ui32 height, const GraphicsResourceDesc& desc)
    : GraphicsResource(desc), m_width(width), m_height(height)
{
    createDepthBuffer(width, height);
}

dx3d::DepthBuffer::~DepthBuffer()
{
}

void dx3d::DepthBuffer::createDepthBuffer(ui32 width, ui32 height)
{
    // Create depth texture
    D3D11_TEXTURE2D_DESC depthTextureDesc{};
    depthTextureDesc.Width = width;
    depthTextureDesc.Height = height;
    depthTextureDesc.MipLevels = 1;
    depthTextureDesc.ArraySize = 1;
    depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthTextureDesc.SampleDesc.Count = 1;
    depthTextureDesc.SampleDesc.Quality = 0;
    depthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthTextureDesc.CPUAccessFlags = 0;
    depthTextureDesc.MiscFlags = 0;

    DX3DGraphicsLogErrorAndThrow(
        m_device.CreateTexture2D(&depthTextureDesc, nullptr, &m_depthTexture),
        "Failed to create depth texture"
    );

    // Create depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
    depthStencilViewDesc.Format = depthTextureDesc.Format;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    DX3DGraphicsLogErrorAndThrow(
        m_device.CreateDepthStencilView(m_depthTexture.Get(), &depthStencilViewDesc, &m_depthStencilView),
        "Failed to create depth stencil view"
    );

    DX3DLogInfo("Depth buffer created successfully");
}

void dx3d::DepthBuffer::releaseResources()
{
    m_depthStencilView.Reset();
    m_depthTexture.Reset();
}

void dx3d::DepthBuffer::resize(ui32 width, ui32 height)
{
    if (m_width == width && m_height == height)
        return;

    m_width = width;
    m_height = height;

    releaseResources();
    createDepthBuffer(width, height);
}

ID3D11DepthStencilView* dx3d::DepthBuffer::getDepthStencilView() const
{
    return m_depthStencilView.Get();
}