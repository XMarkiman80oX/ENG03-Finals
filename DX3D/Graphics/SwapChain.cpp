#include <../Graphics/SwapChain.h>

dx3d::SwapChain::SwapChain(const SwapChainDesc& desc, const GraphicsResourceDesc& gDesc) :
    GraphicsResource(gDesc)
{
    DXGI_SWAP_CHAIN_DESC dxgiDesc{};

    dxgiDesc.BufferDesc.Width = std::max(1, desc.winSize.width);
    dxgiDesc.BufferDesc.Height = std::max(1, desc.winSize.height);
    dxgiDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxgiDesc.BufferCount = 2;
    dxgiDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    dxgiDesc.OutputWindow = static_cast<HWND>(desc.winHandle);
    dxgiDesc.SampleDesc.Count = 1;
    dxgiDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    dxgiDesc.Windowed = TRUE;

    DX3DGraphicsLogErrorAndThrow(m_factory.CreateSwapChain(&m_device, &dxgiDesc, &m_swapChain),
        "CreateSwapChain failed.");

    createRenderTargetView();
}

dx3d::SwapChain::~SwapChain()
{
}

void dx3d::SwapChain::createRenderTargetView()
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    DX3DGraphicsLogErrorAndThrow(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer),
        "GetBuffer failed for back buffer.");

    DX3DGraphicsLogErrorAndThrow(m_device.CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView),
        "CreateRenderTargetView failed.");
}

void dx3d::SwapChain::present()
{
    m_swapChain->Present(1, 0);
}

ID3D11RenderTargetView* dx3d::SwapChain::getRenderTargetView() const
{
    return m_renderTargetView.Get();
}

ID3D11RenderTargetView** dx3d::SwapChain::getRenderTargetViewAddress()
{
    return m_renderTargetView.GetAddressOf();
}