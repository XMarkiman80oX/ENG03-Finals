#include <DX3D/Graphics/VertexBuffer.h>

dx3d::VertexBuffer::VertexBuffer(const void* vertexList, ui32 vertexSize, ui32 vertexCount,
    const GraphicsResourceDesc& gDesc)
    : GraphicsResource(gDesc),
    m_vertexCount(vertexCount)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = vertexSize * vertexCount;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertexList;

    DX3DGraphicsLogErrorAndThrow(m_device.CreateBuffer(&bufferDesc, &initData, &m_buffer),
        "Failed to create vertex buffer.");
}

dx3d::ui32 dx3d::VertexBuffer::getVertexCount() const noexcept
{
    return m_vertexCount;
}

ID3D11Buffer* dx3d::VertexBuffer::getBuffer() const noexcept
{
    return m_buffer.Get();
}