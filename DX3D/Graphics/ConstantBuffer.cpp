#include <../Graphics/ConstantBuffer.h>
#include <../Graphics/DeviceContext.h>

dx3d::ConstantBuffer::ConstantBuffer(ui32 bufferSize, const GraphicsResourceDesc& desc)
    : GraphicsResource(desc), m_bufferSize(bufferSize)
{
    // Ensure buffer size is multiple of 16 bytes (D3D11 requirement)
    m_bufferSize = (bufferSize + 15) & ~15;

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = m_bufferSize;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;

    DX3DGraphicsLogErrorAndThrow(
        m_device.CreateBuffer(&bufferDesc, nullptr, &m_buffer),
        "Failed to create constant buffer"
    );
}

dx3d::ConstantBuffer::~ConstantBuffer()
{
}

void dx3d::ConstantBuffer::update(DeviceContext& deviceContext, const void* data)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource{};

    auto hr = deviceContext.getDeviceContext()->Map(
        m_buffer.Get(),
        0,
        D3D11_MAP_WRITE_DISCARD,
        0,
        &mappedResource
    );

    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, data, m_bufferSize);
        deviceContext.getDeviceContext()->Unmap(m_buffer.Get(), 0);
    }
    else
    {
        DX3DLogError("Failed to map constant buffer");
    }
}