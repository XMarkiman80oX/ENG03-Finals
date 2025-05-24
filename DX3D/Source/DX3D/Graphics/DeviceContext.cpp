#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/VertexBuffer.h>

dx3d::DeviceContext::DeviceContext(const GraphicsResourceDesc& desc, ID3D11DeviceContext* deviceContext)
    : GraphicsResource(desc)
{
    m_deviceContext = deviceContext;
}

dx3d::DeviceContext::~DeviceContext()
{

}

void dx3d::DeviceContext::clearRenderTargetColor(SwapChain& swapChain, float red, float green, float blue, float alpha)
{
    float clearColor[] = { red, green, blue, alpha };
    m_deviceContext->ClearRenderTargetView(swapChain.getRenderTargetView(), clearColor);
}

void dx3d::DeviceContext::setVertexBuffer(const VertexBuffer& vertexBuffer)
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    ID3D11Buffer* buffer = vertexBuffer.getBuffer();
    m_deviceContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void dx3d::DeviceContext::setViewportSize(ui32 width, ui32 height)
{
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_deviceContext->RSSetViewports(1, &viewport);
}

void dx3d::DeviceContext::setRenderTargets(SwapChain& swapChain)
{
    m_deviceContext->OMSetRenderTargets(1, swapChain.getRenderTargetViewAddress(), nullptr);
}

void dx3d::DeviceContext::setVertexShader(ID3D11VertexShader* vertexShader)
{
    if (vertexShader) {
        m_deviceContext->VSSetShader(vertexShader, nullptr, 0);
    }
}

void dx3d::DeviceContext::setPixelShader(ID3D11PixelShader* pixelShader)
{
    if (pixelShader) {
        m_deviceContext->PSSetShader(pixelShader, nullptr, 0);
    }
}

void dx3d::DeviceContext::setInputLayout(ID3D11InputLayout* inputLayout)
{
    if (inputLayout) {
        m_deviceContext->IASetInputLayout(inputLayout);
    }
}

void dx3d::DeviceContext::drawTriangleList(ui32 vertexCount, ui32 startVertexIndex)
{
    m_deviceContext->Draw(vertexCount, startVertexIndex);
}

void dx3d::DeviceContext::present(SwapChain& swapChain)
{
    swapChain.present();
}

ID3D11DeviceContext* dx3d::DeviceContext::getDeviceContext()
{
    return m_deviceContext.Get(); 
}