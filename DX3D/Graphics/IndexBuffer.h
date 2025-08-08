#pragma once
#include <../Graphics/GraphicsResource.h>

namespace dx3d
{
    class IndexBuffer final : public GraphicsResource
    {
    public:
        IndexBuffer(const void* indexList, ui32 indexCount, const GraphicsResourceDesc& desc);
        ~IndexBuffer();

        ui32 getIndexCount() const noexcept { return m_indexCount; }
        ID3D11Buffer* getBuffer() const noexcept { return m_buffer.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer{};
        ui32 m_indexCount{};
    };
}