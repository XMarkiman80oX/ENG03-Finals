#include <DX3D/Graphics/IndexBuffer.h>

dx3d::IndexBuffer::IndexBuffer(const void* indexList, ui32 indexCount, const GraphicsResourceDesc& desc)
    : GraphicsResource(desc), m_indexCount(indexCount)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(ui32) * indexCount;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = indexList;

    DX3DGraphicsLogErrorAndThrow(
        m_device.CreateBuffer(&bufferDesc, &initData, &m_buffer),
        "Failed to create index buffer"
    );
}

dx3d::IndexBuffer::~IndexBuffer()
{
}